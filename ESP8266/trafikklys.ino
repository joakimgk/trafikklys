#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <ESP8266WiFi.h>
#include <Ticker.h>

#define PORT 10000
#define HOST "192.168.2.200"
#define MAX_LENGTH 256
#define TIMER_DELAY 25000
#define TICKS_MAX 65000

const char* ssid     = "Xperia z";         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = "fleskeeske";     // The password of the Wi-Fi network

char buffer[MAX_LENGTH];
const short int GREEN = 2;
const short int YELLOW = 4;
const short int RED = 5;
int state = LOW;

volatile unsigned int tempo = 50;
char program[MAX_LENGTH];
char rec_program[MAX_LENGTH];
volatile int length = 4;
int rec_length;
volatile int step = 0;

bool offline = false;

WiFiClient client;
Ticker timer;

volatile int ticks;

// ISR to Fire when Timer is triggered
void ICACHE_RAM_ATTR onTime() {
  if (ticks++ >= tempo) {
    blink();
    ticks = 0;
    // tempo = newTempo;  // reset tempo only when current interval is complete
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(GREEN, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(RED, OUTPUT);

  Serial.println("\nTrafikklys 3.2\n");

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
  }

  program[0] = 0b00000001;
  program[1] = 0b00000010;
  program[2] = 0b00000100;
  program[3] = 0b00000010;

  timer1_attachInterrupt(onTime); // Add ISR Function
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);  //NB!! Crystal is 26MHz!! Not 80!

  // Arm the Timer for our 0.2s Interval
  timer1_write(TIMER_DELAY); // 25000000 / 5 ticks per us from TIM_DIV16 == 200,000 us interval 
}


void loop() {
  // put your main code here, to run repeatedly:
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
  digitalWrite(GREEN, getBit(program[step], 0));
  digitalWrite(YELLOW, getBit(program[step], 1));
  digitalWrite(RED, getBit(program[step], 2));
  
  if (++step >= length) {
    step = 0;
  }
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
      
    default:
      break;
  }

}
