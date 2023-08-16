#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Ticker.h>

#define PORT 10000
#define HOST "192.168.227.234"

#define UDP_PORT 4210
#define UDP_HOST "0.0.0.0"

#define MAX_LENGTH 256
#define MAX_CLIENTS 10

#define TIMER_DELAY 25000
#define TICKS_MAX 3000
#define PING_TIME 2000
#define RESET_TIME 500

char buffer[MAX_LENGTH];
const short int GREEN = 2;
const short int YELLOW = 4;
const short int RED = 5;

const short int MASTER_LED = 15;
int state = LOW;

const uint32 clientID = system_get_chip_id();
char clientIDstring[9];

volatile unsigned int tempo = 50;
char program[MAX_LENGTH];
char rec_program[MAX_LENGTH];
volatile int length = 4;
int rec_length;
volatile int step = 0;

bool offline = false;
bool master = false;

WiFiClient client;
WiFiUDP UDP;
Ticker timer;

volatile int ticks = 0;
volatile int timeSincePing = 0;
int j = 0;
int jitter[MAX_CLIENTS];
int resetTick = 0;
volatile int timeSinceReset = 0;

volatile bool resetFlag = false;

// ISR to Fire when Timer is triggered
void ICACHE_RAM_ATTR onTime() {
  if (resetFlag || ticks++ >= tempo) {
    blink();
    ticks = 0;
    resetFlag = false;
    // tempo = newTempo;  // reset tempo only when current interval is complete
  }
  
  timeSincePing++;
  timeSinceReset++;

/*
  if (timeSincePing % 1000 == 0) {
    Serial.println("\tMASTER timeSincePing = " + (String)timeSincePing);
  }
  if (timeSinceReset % 1000 == 0) {
    Serial.println("\tMASTER timeSinceReset = " + (String)timeSinceReset);
  }
  */

}

void setup() {
  Serial.begin(115200);
  pinMode(GREEN, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(RED, OUTPUT);

  pinMode(MASTER_LED, OUTPUT);
  digitalWrite(MASTER_LED, LOW);

  sprintf(clientIDstring, "%08x", clientID);
  Serial.println("\nTrafikklys 3.2\n");
  Serial.println("ClientID: " + clientID);

  //WiFi.begin(ssid, password);             // Connect to the network
  WiFiManager wifiManager;
  wifiManager.setConnectTimeout(10);
  wifiManager.setConnectRetries(6);
  wifiManager.setConfigPortalTimeout(60);

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect();
  
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  if (!client.connect(HOST, PORT)) {
    Serial.println("Connection to trafikklys host failed -- running offline");
    offline = true;
  } else {
    Serial.println("Connected to server at " + (String)HOST + ":" + (String)PORT);
  }

  // Begin listening to UDP port
  UDP.begin(UDP_PORT);
  Serial.print("Listening on UDP port ");
  Serial.println(UDP_PORT);

  program[0] = 0b00000001;
  program[1] = 0b00000010;
  program[2] = 0b00000100;
  program[3] = 0b00000010;

  timer1_attachInterrupt(onTime); // Add ISR Function
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);  //NB!! Crystal is 26MHz!! Not 80!

  // Arm the Timer for our 0.2s Interval
  timer1_write(TIMER_DELAY); // 25000000 / 5 ticks per us from TIM_DIV16 == 200,000 us interval 
}

char packet[255];
char pingId = '\0';
bool pingUnsync = false;

void loop() {

  // TCP
  if (client.connected()) {
    int b = client.available();
    if (b > 0) {
      if (b > MAX_LENGTH) {
        Serial.println("Packet size " + (String)b + " larger than buffer " + (String)MAX_LENGTH);
      } else {
        client.readBytes(buffer, b);
        buffer[b] = '\0';
        Serial.println((String)buffer);

        handlePayload(buffer);
      }
    }
  }

  // UDP
  int packetSize = UDP.parsePacket();
  if (packetSize) {
    Serial.print("Received packet! Size: ");
    Serial.println(packetSize); 
    int len = UDP.read(packet, 255);
    if (len > 0)
    {
      packet[len] = '\0';
    }
    Serial.print("Packet received: ");
    Serial.println(packet);

    if (packet[0] == 0x06) {  // ping
      timeSincePing = 0;
      if (master) {
        master = false;  // some other master out there! yield
        digitalWrite(MASTER_LED, LOW);
      }
      char pingResponse[] = { 0x07, 0x04, packet[2], clientIDstring[0], clientIDstring[1], clientIDstring[2], '\0' };  // reply with same pingId, plus my ID
      UDP.beginPacket(UDP.remoteIP(), UDP.remotePort());
      UDP.write(pingResponse);
      UDP.endPacket();
    }

    if (packet[0] == 0x07) {  // ping response (master only receives this...)
      Serial.print("MASTER\tReceived ping response with ID: ");
      Serial.print(packet[2]);
      Serial.print(" (current pingId is ");
      Serial.print(pingId);
      Serial.println(")");

      if (packet[2] == pingId) {
        if (j < MAX_CLIENTS) {
          jitter[j++] = timeSincePing;  // record time of arrival
        }
      } else {
        pingUnsync = true;
      }
    }
  }

  if (timeSincePing > TICKS_MAX) {
    Serial.println("ping timeout: claiming master...");
    // become master
    master = true;
    digitalWrite(MASTER_LED, HIGH);
  }
  if (master) {
    if (timeSincePing > PING_TIME) {
      if (!pingUnsync && j > 0) {
        Serial.print("MASTER\tping time: computing jitter, j = " + (String)j);

        // calculate jitter based on previous times...
        int sumJitter = 0;
        for (int i = 0; i < j; i++) {
          sumJitter += jitter[i];
        }
        Serial.print("MASTER\tping time: sum jitter = ");
        Serial.println(sumJitter);
        double avgJitter = sumJitter/j;
        Serial.print("MASTER\tping time: avg jitter = ");
        Serial.println(avgJitter);
        resetTick = tempo - avgJitter/2;  // if fast tempo (low value) then resetTick is negative, and we don't reset :) No need at high tempo!
        Serial.println("MASTER\tping time: resetTick = " + (String)resetTick);
      }

      Serial.println("MASTER\tping time: sending ping broadcast");
      // send ping! (broadcast)
      pingId = pingId == 0xFF ? 0x01 : pingId + 1;
      char ping[] = { 0x06, 0x01, pingId, '\0' };  // each ping has an ID (so we can identify responses)
      UDP.beginPacket(UDP_HOST, UDP_PORT);
      UDP.write(ping);
      UDP.endPacket();

      // reset
      timeSincePing = 0;
      pingUnsync = false;
      j = 0;
    }

    if (timeSinceReset > RESET_TIME && ticks == resetTick) {
      Serial.println("MASTER\treset time (ticks, tempo = " + (String)ticks + ", " + (String)tempo + "): sending reset broadcast");
      // send reset! (broadcast)
      UDP.beginPacket(UDP_HOST, UDP_PORT);
      char resetMessage[] = { 0x05, 0x01, step, '\0' };
      UDP.write(resetMessage);
      UDP.endPacket();
      timeSinceReset = 0;
    }
  }

  delay(100);
}

void printByte(unsigned char b) {
  for (volatile int i = 0; i < 8; i++) {
    Serial.print((String)((b >> i) & 0b00000001));
  }
}

ICACHE_RAM_ATTR int getBit(unsigned char b, int i) {
  return ((b >> i) & 0b00000001) == 1 ? HIGH : LOW;
}

ICACHE_RAM_ATTR void blink() {
  if (++step >= length) {
    step = 0;
  }

  digitalWrite(GREEN, getBit(program[step], 0));
  digitalWrite(YELLOW, getBit(program[step], 1));
  digitalWrite(RED, getBit(program[step], 2));
  
  //Serial.print("\n" + (String)step + ": ");
  //printByte(program[step]);
}

void handlePayload(char payload[]) {
  
  char command = payload[0];
  int len = (int)payload[1];
  uint16_t test;
  uint8_t i;
  
  switch (command) {
    case 0x01:  // TEMPO
      test = payload[2];

      // safety....
      if (test < 1) test = 1;
      else if (test > 255) test = 256;
      
      tempo = test;
      if (tempo > ticks) {
        ticks = 0;
      }
      break;
      
    case 0x02:  // RESET (restart nåværende program)
      step = 0;
      break;
  
    case 0x03:  // MOTTA PROGRAM  (dump payload inn i *rec_program)
      //memcpy(rec_program, payload, len);	
      for (i = 0; i < len; i++) {
        rec_program[i] = payload[i + 2];
      }
      rec_length = len;
      break;
    
    case 0x04: // BYTT PROGRAM (bytt om referanser og bruk av *program og *rec_program)
      //swapArrays(program, rec_program);
      for (i = 0; i < rec_length; i++) {
        program[i] = rec_program[i];
      }
      length = rec_length;
      step = 0;  // og RESET!
      break;

    case 0x05: // RESET / SYNC 
      step = payload[2] + 1;
      resetFlag = true;
      break;
      
    default:
      break;
  }

}
