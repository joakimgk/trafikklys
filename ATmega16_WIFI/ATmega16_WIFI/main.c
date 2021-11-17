/*
* ATmega16_WIFI
* http://www.electronicwings.com
*
*/


#define F_CPU 3686400UL			/* Define CPU Frequency e.g. here its Ext. 12MHz */
#include <avr/io.h>					/* Include AVR std. library file */
#include <util/delay.h>				/* Include Delay header file */
#include <stdbool.h>				/* Include standard boolean library */
#include <string.h>					/* Include string library */
#include <stdio.h>					/* Include standard IO library */
#include <stdlib.h>					/* Include standard library */
#include <avr/interrupt.h>			/* Include avr interrupt header file */
#include "USART_RS232_H_file.h"		/* Include USART header file */

#define SREG    _SFR_IO8(0x3F)

#define DEFAULT_BUFFER_SIZE		60
#define DEFAULT_TIMEOUT			10000

/* Connection Mode */
#define SINGLE					0
#define MULTIPLE				1

/* Message Mode */
#define HIDE_REMOTE_ADDR		0
#define SHOW_REMOTE_ADDR		1

/* Application Mode */
#define NORMAL					0
#define TRANSPERANT				1

/* Application Mode */
#define STATION							1
#define ACCESSPOINT						2
#define BOTH_STATION_AND_ACCESPOINT		3

/* Define Required fields shown below */
#define DOMAIN				"192.168.135.234"   //43.86"  //"192.168.43.254"
#define PORT				"10000"
#define CHANNEL_ID			"119922"

/* Define UDP setup */
#define UDP_DOMAIN			"0.0.0.0"
#define UDP_PORT			"4445"

#define SSID				"Xperia z"
#define PASSWORD			"fleskeeske"


enum ESP8266_RESPONSE_STATUS{
	ESP8266_RESPONSE_WAITING,
	ESP8266_RESPONSE_FINISHED,
	ESP8266_RESPONSE_TIMEOUT,
	ESP8266_RESPONSE_BUFFER_FULL,
	ESP8266_RESPONSE_STARTING,
	ESP8266_RESPONSE_ERROR
};

enum ESP8266_CONNECT_STATUS {
	ESP8266_CONNECTED_TO_AP,
	ESP8266_CREATED_TRANSMISSION,
	ESP8266_TRANSMISSION_DISCONNECTED,
	ESP8266_NOT_CONNECTED_TO_AP,
	ESP8266_CONNECT_UNKNOWN_ERROR
};

enum ESP8266_JOINAP_STATUS {
	ESP8266_WIFI_CONNECTED,
	ESP8266_CONNECTION_TIMEOUT,
	ESP8266_WRONG_PASSWORD,
	ESP8266_NOT_FOUND_TARGET_AP,
	ESP8266_CONNECTION_FAILED,
	ESP8266_JOIN_UNKNOWN_ERROR
};

int8_t Response_Status;
volatile int16_t Counter = 0, pointer = 0;
uint32_t TimeOut = 0;
char RESPONSE_BUFFER[DEFAULT_BUFFER_SIZE];
char _atCommand[60];

void Read_Response(char* _Expected_Response)
{
	uint8_t EXPECTED_RESPONSE_LENGTH = strlen(_Expected_Response);
	uint32_t TimeCount = 0, ResponseBufferLength;
	char RECEIVED_CRLF_BUF[EXPECTED_RESPONSE_LENGTH];
	
	while(1)
	{
		if(TimeCount >= (DEFAULT_TIMEOUT+TimeOut))
		{
			TimeOut = 0;
			Response_Status = ESP8266_RESPONSE_TIMEOUT;
			return;
		}

		if(Response_Status == ESP8266_RESPONSE_STARTING)
		{
			Response_Status = ESP8266_RESPONSE_WAITING;
		}
		ResponseBufferLength = strlen(RESPONSE_BUFFER);
		if (ResponseBufferLength)
		{
			_delay_ms(1);
			TimeCount++;
			if (ResponseBufferLength==strlen(RESPONSE_BUFFER))
			{
				for (uint16_t i=0;i<ResponseBufferLength;i++)
				{
					memmove(RECEIVED_CRLF_BUF, RECEIVED_CRLF_BUF + 1, EXPECTED_RESPONSE_LENGTH-1);
					RECEIVED_CRLF_BUF[EXPECTED_RESPONSE_LENGTH-1] = RESPONSE_BUFFER[i];
					if(!strncmp(RECEIVED_CRLF_BUF, _Expected_Response, EXPECTED_RESPONSE_LENGTH))
					{
						TimeOut = 0;
						Response_Status = ESP8266_RESPONSE_FINISHED;
						return;
					}
				}
			}
		}
		_delay_ms(1);
		TimeCount++;
	}
}

void ESP8266_Clear()
{
	memset(RESPONSE_BUFFER,0,DEFAULT_BUFFER_SIZE);
	Counter = 0;	pointer = 0;
}

void Start_Read_Response(char* _ExpectedResponse)
{
	Response_Status = ESP8266_RESPONSE_STARTING;
	do {
		Read_Response(_ExpectedResponse);
	} while(Response_Status == ESP8266_RESPONSE_WAITING);

}

void GetResponseBody(char* Response, uint16_t ResponseLength)
{

	uint16_t i = 12;
	char buffer[5];
	while(Response[i] != '\r')
	++i;

	strncpy(buffer, Response + 12, (i - 12));
	ResponseLength = atoi(buffer);

	i += 2;
	uint16_t tmp = strlen(Response) - i;
	memcpy(Response, Response + i, tmp);

	if(!strncmp(Response + tmp - 6, "\r\nOK\r\n", 6))
	memset(Response + tmp - 6, 0, i + 6);
}

bool WaitForExpectedResponse(char* ExpectedResponse)
{
	Start_Read_Response(ExpectedResponse);	/* First read response */
	if((Response_Status != ESP8266_RESPONSE_TIMEOUT))
	return true;							/* Return true for success */
	return false;							/* Else return false */
}

bool SendATandExpectResponse(char* ATCommand, char* ExpectedResponse)
{
	ESP8266_Clear();
	USART_SendString(ATCommand);			/* Send AT command to ESP8266 */
	USART_SendString("\r\n");
	return WaitForExpectedResponse(ExpectedResponse);
}

bool ESP8266_MessageMode(uint8_t Mode)
{
	sprintf(_atCommand, "AT+CIPDINFO=%d", Mode);
	return SendATandExpectResponse(_atCommand, "\r\nOK\r\n");
}

bool ESP8266_ApplicationMode(uint8_t Mode)
{
	sprintf(_atCommand, "AT+CIPMODE=%d", Mode);
	return SendATandExpectResponse(_atCommand, "\r\nOK\r\n");
}

bool ESP8266_ConnectionMode(uint8_t Mode)
{
	sprintf(_atCommand, "AT+CIPMUX=%d", Mode);
	return SendATandExpectResponse(_atCommand, "\r\nOK\r\n");
}

bool ESP8266_Begin()
{
	for (uint8_t i=0;i<5;i++)
	{
		if(SendATandExpectResponse("AT","\r\nOK\r\n")||SendATandExpectResponse("AT","\r\nOK\r\n"))
		return true;
	}
	return false;
}

bool ESP8266_Close()
{
	return SendATandExpectResponse("AT+CIPCLOSE=1", "\r\nOK\r\n");
}

bool ESP8266_WIFIMode(uint8_t _mode)
{
	sprintf(_atCommand, "AT+CWMODE=%d", _mode);
	return SendATandExpectResponse(_atCommand, "\r\nOK\r\n");
}

uint8_t ESP8266_JoinAccessPoint(char* _SSID, char* _PASSWORD)
{
	sprintf(_atCommand, "AT+CWJAP=\"%s\",\"%s\"", _SSID, _PASSWORD);
	if(SendATandExpectResponse(_atCommand, "\r\nWIFI CONNECTED\r\n"))
	return ESP8266_WIFI_CONNECTED;
	else{
		if(strstr(RESPONSE_BUFFER, "+CWJAP:1"))
		return ESP8266_CONNECTION_TIMEOUT;
		else if(strstr(RESPONSE_BUFFER, "+CWJAP:2"))
		return ESP8266_WRONG_PASSWORD;
		else if(strstr(RESPONSE_BUFFER, "+CWJAP:3"))
		return ESP8266_NOT_FOUND_TARGET_AP;
		else if(strstr(RESPONSE_BUFFER, "+CWJAP:4"))
		return ESP8266_CONNECTION_FAILED;
		else
		return ESP8266_JOIN_UNKNOWN_ERROR;
	}
}

void ESP8266_CloseAllConnections() {
	SendATandExpectResponse("AT+CIPCLOSE=5", "\r\nOK\r\n");
}

void ESP8266_QueryIPAddress() {
	SendATandExpectResponse("AT+CIFSR", "\r\nOK\r\n");
}

void ESP8266_DisableServerMode() {
	SendATandExpectResponse("AT+CIPSERVER=0", "\r\nOK\r\n");
}

uint8_t ESP8266_connected() 
{
	SendATandExpectResponse("AT+CIPSTATUS", "\r\nOK\r\n");
	if(strstr(RESPONSE_BUFFER, "STATUS:2"))
	return ESP8266_CONNECTED_TO_AP;
	else if(strstr(RESPONSE_BUFFER, "STATUS:3"))
	return ESP8266_CREATED_TRANSMISSION;
	else if(strstr(RESPONSE_BUFFER, "STATUS:4"))
	return ESP8266_TRANSMISSION_DISCONNECTED;
	else if(strstr(RESPONSE_BUFFER, "STATUS:5"))
	return ESP8266_NOT_CONNECTED_TO_AP;
	else
	return ESP8266_CONNECT_UNKNOWN_ERROR;
}

uint8_t ESP8266_Start(uint8_t _ConnectionNumber, char* Domain, char* Port)
{
	bool _startResponse;

	if(SendATandExpectResponse("AT+CIPMUX?", "CIPMUX:0"))
		sprintf(_atCommand, "AT+CIPSTART=\"TCP\",\"%s\",%s", Domain, Port);
	else
		sprintf(_atCommand, "AT+CIPSTART=%d,\"TCP\",\"%s\",%s", _ConnectionNumber, Domain, Port);

	_startResponse = SendATandExpectResponse(_atCommand, "CONNECT\r\n");
	if(!_startResponse)
	{
		if(Response_Status == ESP8266_RESPONSE_TIMEOUT)
		return ESP8266_RESPONSE_TIMEOUT;
		return ESP8266_RESPONSE_ERROR;
	}
	return ESP8266_RESPONSE_FINISHED;
}

uint8_t ESP8266_StartUDP(uint8_t _ConnectionNumber, char* Domain, char* Port, char* LocalPort, uint8_t _Mode)
{
	bool _startResponse;

	if(SendATandExpectResponse("AT+CIPMUX?", "CIPMUX:0"))
		sprintf(_atCommand, "AT+CIPSTART=\"UDP\",\"%s\",%s", Domain, Port);
	else
		sprintf(_atCommand, "AT+CIPSTART=%d,\"UDP\",\"%s\",%s,%s,%d", _ConnectionNumber, Domain, Port, LocalPort, _Mode);

	_startResponse = SendATandExpectResponse(_atCommand, "CONNECT\r\n");
	if(!_startResponse)
	{
		if(Response_Status == ESP8266_RESPONSE_TIMEOUT)
		return ESP8266_RESPONSE_TIMEOUT;
		return ESP8266_RESPONSE_ERROR;
	}
	return ESP8266_RESPONSE_FINISHED;
}

// If you want to send data to any other UDP terminals, please set the IP and port of this terminal.
uint8_t ESP8266_Send_UDP(char* Data, uint8_t IP1, uint8_t IP2, uint8_t IP3, uint8_t IP4, uint8_t Port)
{
	sprintf(_atCommand, "AT+CIPSEND=%d, \"%d.%d.%d.%d\", %d", (strlen(Data)+2), IP1, IP2, IP3, IP4, Port);
	SendATandExpectResponse(_atCommand, "\r\nOK\r\n>");
	if(!SendATandExpectResponse(Data, "\r\nSEND OK\r\n"))
	{
		if(Response_Status == ESP8266_RESPONSE_TIMEOUT)
		return ESP8266_RESPONSE_TIMEOUT;
		return ESP8266_RESPONSE_ERROR;
	}
	return ESP8266_RESPONSE_FINISHED;
}

// Otherwise, set connection number of existing connection (TCP or UDP)
uint8_t ESP8266_Send(uint8_t _ConnectionNumber, char* Data)
{
	sprintf(_atCommand, "AT+CIPSEND=%d,%d", _ConnectionNumber, (strlen(Data)+2));
	SendATandExpectResponse(_atCommand, "\r\nOK\r\n>");
	if(!SendATandExpectResponse(Data, "\r\nSEND OK\r\n"))
	{
		if(Response_Status == ESP8266_RESPONSE_TIMEOUT)
		return ESP8266_RESPONSE_TIMEOUT;
		return ESP8266_RESPONSE_ERROR;
	}
	return ESP8266_RESPONSE_FINISHED;
}

int16_t ESP8266_DataAvailable()
{
	return (Counter - pointer);
}

uint8_t ESP8266_DataRead()
{
	if(pointer < Counter)
	return RESPONSE_BUFFER[pointer++];
	else{
		ESP8266_Clear();
		return 0;
	}
}

uint16_t Read_Data(char* _buffer, uint8_t buffer_length)
{
	uint16_t len = 0;
	//_delay_ms(100);
	uint16_t timeout = 1000;
	uint16_t timer = 0;
	if (ESP8266_DataAvailable() > 0) {
		// wait until \r\n
		uint8_t x = ESP8266_DataRead();
		while (x != '\n' && len < buffer_length - 2) {
			_buffer[len++] = x;
			while (ESP8266_DataAvailable() == 0 && timer++ < timeout) {
				_delay_ms(1);
			}
			if (timer >= timeout) PORTC = 0b01000000;  // ALARM! Read timeout! PC6
			x = ESP8266_DataRead();
		}
		if (len == buffer_length - 2) PORTC = 0b10000000;  // ALARM! Data being truncated! PC7
		_buffer[len++] = x;
		_buffer[len++] = '\0';
	}
	//while(ESP8266_DataAvailable() > 0)
	//_buffer[len++] = ESP8266_DataRead();
	return len;
}

# define BUFFER_LENGTH 40
# define SLOW 3
# define RAPID 5

uint8_t Running_Status = 0;
uint8_t Connect_Status = 0;

// program buffers
volatile uint8_t program[BUFFER_LENGTH];
volatile uint8_t rec_program[BUFFER_LENGTH];

// length of current program
volatile uint8_t length = 0;
// length of received program
volatile uint8_t rec_length = 0;
// playback position
volatile uint8_t step = 0;
// tempo multiplier
volatile uint8_t tempo = 25;
volatile uint8_t ticks = 0;

volatile uint16_t ticks2 = 0;

// status indicator rate
uint8_t tempoS = 120;
uint8_t ticksS = 0;
uint8_t blinkState = 0;

uint16_t sync = 255 * 10;  // sync hvert 10. sekund (NB! uavhengig av `tempo`)
uint16_t syncTimeout = 100;
volatile bool doSync = false;
volatile bool doResponse = false;

volatile uint16_t ticksSinceSync = 0;
volatile uint8_t measureJitter = 0;  // 0: expired, 1: start, 2: sending, 3: done (for a while)
volatile uint16_t jitterTicks = 0;
volatile uint16_t jitter = 0;

volatile bool master = false;

void handlePayload(char command, int len, char payload[]) {
	
	uint16_t test;
	uint8_t i;
	
	switch (command) {
		case 0x01:  // TEMPO
			test = payload[0];

			// safety....
			if (test < 1) test = 1;
			else if (test > 255) test = 256;
			
			tempo = test;			
			break;
			
		case 0x02:  // RESET (restart nåværende program)
			step = 0;
			break;
	
		case 0x03:  // MOTTA PROGRAM  (dump payload inn i *rec_program)
			//memcpy(rec_program, payload, len);	
			for (i = 0; i < len; i++) {
				rec_program[i] = payload[i];
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
			

			// +IPD,0,4:[05][01]	
			// +IPD,1,4,192.168.160.7,4445:[05][01] <-- STAIP of master	
		case 0x05: // SYNC	
			ticksSinceSync = 0; // reset counter	
			if (master) master = false; // master (sender) does NOT receive own UPD broadcasts	
			
			if (measureJitter == 0) {	
				jitterTicks = 0;	
				measureJitter = 1;	
			}	
			if (measureJitter == 3) {	
				TCNT1 = jitter;	
				measureJitter = 0;  // get ready for next round (next sync packet)	
			}	
			break;	

			/*	
		case 0x06:  // PING REQUEST	
			if (master) {	
				// determine sender IP  // TODO! Master needs to support MULTIPLE (a queue?) of IPs (pings) to respond to!!	
				// send ping response (0x07)	
				////doResponse = true;  // remove when we have actual queue of IPs/pings: Non-empty queue is handled in main loop	
			}	
			break;	
			*/	

			/*	
				
		case 0x07: // PING RESPONSE 	
			if (!master) {	
				jitter = (jitterTicks / 2 * 225) % 225;	
				measureJitter = 3; // done	
				// NB! We don't SET the jitter adjustment here/now; we wait until the NEXT sync packet arrives	
			}	
			break;	
			*/
			
		default:
			break;
	}

}

ISR (USART_RXC_vect, ISR_BLOCK)
{ 
	uint8_t oldsrg = SREG;
	cli();
	RESPONSE_BUFFER[Counter] = UDR;
	Counter++;
	if(Counter == DEFAULT_BUFFER_SIZE){
		Counter = 0; pointer = 0;
	}
	SREG = oldsrg;
}

uint8_t invert(uint8_t bit) {
	return (bit == 1 ? 0 : 1);
}

ISR (TIMER1_COMPA_vect)
{
	uint8_t oldsrg = SREG;
	cli();
	
	uint8_t newState = PORTB;
	
	// fixed-interval update of status LEDs (for steady-rate blinking)
	if (ticksS++ >= tempoS) {
		ticksS = 0;
		
		if (blinkState == 1) blinkState = 0;
		else blinkState = 1;
		
		uint8_t masterState = master;
		if (!master && ticksSinceSync > syncTimeout) masterState = blinkState;
		
		newState = (newState & ~(1 << 3) & ~(1 << 4)) | (Running_Status << 3) | (masterState << 4);  // Running_Status 0 is OFF
		//newState = (newState & ~(1 << 3) & ~(1 << 4) & ~(1 << 5)) | (Connect_Status << 3) | (Running_Status << 4) | (masterState << 5);  // Running_Status 0 is OFF
	}
	
	if (ticks++ >= tempo) {
		ticks = 0;
		
		if ((step >= length) || (step == BUFFER_LENGTH)) step = 0;
	
		//uint8_t mem = (~PORTB & 0b11111000);
		//PORTB = (~(program[step++]) | mem);  // *(program + step++);
		newState = (newState & ~0b00000111) | (program[step++] ^ 0b00000111);
	}
	PORTB = newState;

	if (Running_Status == 1) {
		if (!master) {	

			if (measureJitter == 2) jitterTicks++;	
			if (jitterTicks > 65530) {	
				//USART_SendString("jitterTicks maxed out");	
				jitterTicks = 65530;	
			}	
			
			if (measureJitter == 1) {	
				jitterTicks = 0;	
				measureJitter = 2;  // send ping ASAP	
			} 
		
			if (measureJitter == 0) {	
				if (ticksSinceSync > syncTimeout) {	
					//if (rand() < 0.1) { // don't think we actually need rand, since each client will start up at different times	
					master = true;  // try becoming master. If someone beats you to it, receiving 0x05 (sync) will let you know	
					//}	
				} else ticksSinceSync++;	
			}

		} else {
			
			if (ticks2++ >= sync) {	
				ticks2 = 0;	// reset counter
				doSync = true;	// set flag
				PORTC = 0b00000011;  // PC1
			}
		}
	}
	SREG = oldsrg;
}


void setupTimerISR()
{
	cli();
	
	// set up timer1
	TCCR1A = 0; // Reset control registers timer1 /not needed, safety
	TCCR1B = 0; // Reset control registers timer1 // not needed, safety
	TIMSK |= (1 << OCIE1A); // | (1 << TOIE1); //timer1 output compare match and overflow interrupt enable
	OCR1A = 225; // 1/256 sec "base rate"
	TCNT1 = 0;
	//TCCR1B |= (1 << WGM12)|(1 << CS11);  // prescaling=8 CTC-mode (two counts per microsecond)
	TCCR1B |= (1 << WGM12)|(1 << CS11)|(1 << CS10);  // prescaling=64 CTC-mode (two counts per microsecond)
	//TCCR1B |= (1 << WGM12)|(1 << CS10)|(1 << CS12);  // prescaling=1024 CTC-mode (two counts per microsecond)
	sei();
}

int main(void)
{	
	_delay_ms(200);
	
	//char _buffer[40];
	//uint8_t Connect_Status;
	//uint8_t Sample = 0;
	Running_Status = 0;
	
	DDRB = 0xFF; // set PORTB for output
	//DDRD = 0xFF;
	//PORTB = 0b11011111; // crash indicator (LED 5)
	//PORTD = 0b11111111; // network setup indicator (LED 2) OFF
	
	DDRC = 0xFF; // set PORTC for output
	PORTC = 0b00000001;  // PC0  (test) 
	//PORTC = 0xFF;

	program[0] = 0b00000100;
	program[1] = 0b00000010;
	program[2] = 0b00000001;
	program[3] = 0b00000010;
	length = 4;
	
	step = 0;
	
	cli();
	//PORTB = 0b10000000; // setup indicator (LED 6)
	setupTimerISR();

	USART_Init(115200);						/* Initiate USART with 115200 baud rate */
	sei();									/* Start global interrupt */

	if (master) USART_SendString("Master node");
	else USART_SendString("Normal node");
	
	//PORTB = 0b11000000; // setup indicator (LED 6)
	while(!ESP8266_Begin());
	
	//PORTB = 0b11100000; // setup indicator (LED 6)
	ESP8266_CloseAllConnections();
	ESP8266_WIFIMode(BOTH_STATION_AND_ACCESPOINT);/* 3 = Both (AP and STA) */
	ESP8266_ApplicationMode(NORMAL);		/* 0 = Normal Mode; 1 = Transperant Mode */

    uint8_t connectionMode = MULTIPLE;
	ESP8266_ConnectionMode(connectionMode); //SINGLE);			/* 0 = Single; 1 = Multi */
	//ESP8266_DisableServerMode(); // test
	if(ESP8266_connected() == ESP8266_NOT_CONNECTED_TO_AP) {
		ESP8266_JoinAccessPoint(SSID, PASSWORD);
	}
	
	uint8_t messageMode = SHOW_REMOTE_ADDR;
	if (messageMode == SHOW_REMOTE_ADDR) ESP8266_MessageMode(messageMode);		/* 0 = Normal Mode; 1 = Show Remote Host/Port */
	
	ESP8266_Start(0, DOMAIN, PORT);
	//PORTB = 0b11110000; // setup indicator (LED 6)
	
	// TRENGER VI GJØRE DETTE?? BRUKER VI EGENTLIG UDP "MOT SERVER"??
	if (connectionMode == MULTIPLE) {
		ESP8266_QueryIPAddress();
		// UDP init på port 4445 (on all addresses)
		ESP8266_StartUDP(1, UDP_DOMAIN, UDP_PORT, UDP_PORT, 2);
		//AT+CIPSTART=0,"UDP","0.0.0.0",4445,4445,2
	}
	
	//PORTB = 0xFF; // All leds off
	//PORTD = 0b11111011; // network setup indicator (LED 2) ON
	unsigned char payload[40];
	unsigned char readme[40];
	unsigned char tmp[40];
	//unsigned char remoteAddress[32];
	
	uint8_t dKanal;
	uint8_t dKommando;
	uint8_t dLengde;
	uint8_t IP1 = 0, IP2 = 0, IP3 = 0, IP4 = 0;
	uint8_t remotePort;
	
	char syncMessage[] = { 0x05, 0x01, 0xFF, '\0' }; // payload 0xFF unused (should support empty payload, length = 0, but don't yet)
	char pingMessage[] = { 0x06, 0x01, 0xFF, '\0' };  // all null terminated
	char pingResponse[] = { 0x07, 0x01, 0xFF, '\0' };

	//if (master) doSync = true; // test!!
	Running_Status = 1;
	//Connect_Status = ESP8266_connected();
	
	while(1)
	{
		//if (programCounter++ > 65000) {	
			//programCounter = 0;	
			//	
			////Connect_Status = ESP8266_connected();	
			////if(Connect_Status == ESP8266_NOT_CONNECTED_TO_AP) {	
				////ESP8266_JoinAccessPoint(SSID, PASSWORD);	
			////}	
			////if(Connect_Status == ESP8266_CONNECTED_TO_AP || Connect_Status == ESP8266_TRANSMISSION_DISCONNECTED) {	
				////ESP8266_Start(0, DOMAIN, PORT);	
				////ESP8266_StartUDP(1, UDP_DOMAIN, UDP_PORT, UDP_PORT, 2);	
			////}	
		//}	


		if (master && connectionMode == MULTIPLE && doSync) {	
			doSync = false;  // clear flag
			//PORTC = 0b00000001;  // PC1
			ESP8266_Send(1, syncMessage); 	// send UDP sync message (is this UDP broadcast?)	
		}
		
		/*
		if (!master && measureJitter == 2) {	
			// here, we HAVE received a 0x05 sync packet (from master)	
			// send UDP sync packet in return. This is !master, so we only expect to get ONE source of these messages (not many, at the same time)	
			// thus, we can rely on the "last read" IP to be the recipient (we also only read the address off UDP packets, not those from server/TCP)	
			if (IP1 != 0) {	
				//memset(remoteAddress, 0, 32);	
				//sprintf(remoteAddress, "%d.%d.%d.%d", IP1, IP2, IP3, IP4);	
				ESP8266_Send_UDP(pingMessage, IP1, IP2, IP3, IP4, remotePort);	
			}	
		}
		*/
		
		uint8_t len = 0;
		memset(readme, 0, 40);
		len = Read_Data(readme, 40);
		
		unsigned char * pbuffer_cmd = strstr(readme, "+IPD");
		if (len > 0) {
			if (pbuffer_cmd != NULL) {

				while (pbuffer_cmd != NULL) {  // +IPD,1,42:0x01 0x02 0xAA 0x23+IPD,1,3 ... ...

					unsigned char *pbuffer_len = strstr(pbuffer_cmd, ":");

					uint8_t startpos = (uint8_t)(pbuffer_cmd - readme);
					uint8_t endpos = (uint8_t)(pbuffer_len - readme);
					
					// make scratch space (tmp) for tokenizer
					if (endpos > 39) break;  //endpos = 49;
					strncpy(tmp, pbuffer_cmd, endpos);
					tmp[endpos] = 0;
					
					char * token = strtok(tmp, ",");    // +IPD "header" (ignored)
					
					if (connectionMode == MULTIPLE) {
						token = strtok(NULL, ",");
						dKanal = atoi(token);  // channel ID
					}
					
					if (messageMode == SHOW_REMOTE_ADDR) token = strtok(NULL, ",");
					else token = strtok(NULL, ":");
					dLengde = atoi(token);     // data length
					if (dLengde > 39) break;   // safety; else just abort
					
					if (messageMode == SHOW_REMOTE_ADDR && dKanal == 1) {  // only read address from UDP packets (other devices, not server!)
						token = strtok(NULL, ".");
						IP1 = atoi(token);
						token = strtok(NULL, ".");
						IP2 = atoi(token);
						token = strtok(NULL, ".");
						IP3 = atoi(token);
						token = strtok(NULL, ".");
						IP4 = atoi(token);
						
						token = strtok(NULL, ":");
						remotePort = atoi(token);  // remote port
					}
					
					memcpy(payload, pbuffer_len+1, dLengde);
					
					dKommando = payload[0];
					dLengde = payload[1];
					
					// +1 for å klarere :, +2 for å gå forbi kommando- og lengde-bytes
					strncpy(payload, pbuffer_len+1 +2, dLengde);
					payload[dLengde] = 0;  // 000 111 000 111 
					
					if (master && dKanal == 1 && dKommando == 0x06) {
						//memset(remoteAddress, 0, 32);
						//sprintf(remoteAddress, "%d.%d.%d.%d", IP1, IP2, IP3, IP4);
						// send back response on ping, to same device (address)...
						ESP8266_Send_UDP(pingResponse, IP1, IP2, IP3, IP4, remotePort);
					} else {
						handlePayload(dKommando, dLengde, payload);
					}
					//handlePayload(dKommando, dLengde, payload);

					pbuffer_cmd = strstr(pbuffer_len, "+IPD");

				}
			}
		}
	}
}
