/*
* ATmega16_WIFI
* http://www.electronicwings.com
*
*/


#define F_CPU 8000000UL			/* Define CPU Frequency e.g. here its Ext. 12MHz */
#include <avr/io.h>					/* Include AVR std. library file */
#include <util/delay.h>				/* Include Delay header file */
#include <stdbool.h>				/* Include standard boolean library */
#include <string.h>					/* Include string library */
#include <stdio.h>					/* Include standard IO library */
#include <stdlib.h>					/* Include standard library */
#include <avr/interrupt.h>			/* Include avr interrupt header file */
#include "USART_RS232_H_file.h"		/* Include USART header file */

#define SREG    _SFR_IO8(0x3F)

#define DEFAULT_BUFFER_SIZE		160
#define DEFAULT_TIMEOUT			10000

/* Connection Mode */
#define SINGLE					0
#define MULTIPLE				1

/* Application Mode */
#define NORMAL					0
#define TRANSPERANT				1

/* Application Mode */
#define STATION							1
#define ACCESSPOINT						2
#define BOTH_STATION_AND_ACCESPOINT		3

/* Define Required fields shown below */
#define DOMAIN				"192.168.43.254"
#define PORT				"10000"
#define API_WRITE_KEY		"C7JFHZY54GLCJY38"
#define CHANNEL_ID			"119922"

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

void echoTest(char *whatIgot) {
	char smthing[150];
	memset(smthing, 0, 150);
	sprintf(smthing, "whatIgot=%s", whatIgot);
	USART_SendString(smthing);
}

bool IsATCommand(char *_buffer) {
	return (!(_buffer[0] == 'A' && _buffer[1] == 'T' && _buffer[2] == '+'));
}


bool ESP8266_ApplicationMode(uint8_t Mode)
{
	char _atCommand[20];
	memset(_atCommand, 0, 20);
	sprintf(_atCommand, "AT+CIPMODE=%d", Mode);
	_atCommand[19] = 0;
	return SendATandExpectResponse(_atCommand, "\r\nOK\r\n");
}

bool ESP8266_ConnectionMode(uint8_t Mode)
{
	char _atCommand[20];
	memset(_atCommand, 0, 20);
	sprintf(_atCommand, "AT+CIPMUX=%d", Mode);
	_atCommand[19] = 0;
	return SendATandExpectResponse(_atCommand, "\r\nOK\r\n");
}

bool ESP8266_Begin()
{
	for (uint8_t i=0;i<5;i++)
	{
		if(SendATandExpectResponse("ATE0","\r\nOK\r\n")||SendATandExpectResponse("AT","\r\nOK\r\n"))
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
	char _atCommand[20];
	memset(_atCommand, 0, 20);
	sprintf(_atCommand, "AT+CWMODE=%d", _mode);
	_atCommand[19] = 0;
	return SendATandExpectResponse(_atCommand, "\r\nOK\r\n");
}

uint8_t ESP8266_JoinAccessPoint(char* _SSID, char* _PASSWORD)
{
	char _atCommand[60];
	memset(_atCommand, 0, 60);
	sprintf(_atCommand, "AT+CWJAP=\"%s\",\"%s\"", _SSID, _PASSWORD);
	_atCommand[59] = 0;
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
	char _atCommand[60];
	memset(_atCommand, 0, 60);
	_atCommand[59] = 0;

	if(SendATandExpectResponse("AT+CIPMUX?", "CIPMUX:0"))
		sprintf(_atCommand, "AT+CIPSTART=\"TCP\",\"%s\",%s", Domain, Port);
	else
		sprintf(_atCommand, "AT+CIPSTART=\"%d\",\"TCP\",\"%s\",%s", _ConnectionNumber, Domain, Port);

	_startResponse = SendATandExpectResponse(_atCommand, "CONNECT\r\n");
	if(!_startResponse)
	{
		if(Response_Status == ESP8266_RESPONSE_TIMEOUT)
		return ESP8266_RESPONSE_TIMEOUT;
		return ESP8266_RESPONSE_ERROR;
	}
	return ESP8266_RESPONSE_FINISHED;
}

uint8_t ESP8266_Send(char* Data)
{
	char _atCommand[20];
	memset(_atCommand, 0, 20);
	sprintf(_atCommand, "AT+CIPSEND=%d", (strlen(Data)+2));
	_atCommand[19] = 0;
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

uint16_t Read_Data(char* _buffer)
{
	uint16_t len = 0;
	_delay_ms(100);
	while(ESP8266_DataAvailable() > 0)
	_buffer[len++] = ESP8266_DataRead();
	return len;
}

# define BUFFER_LENGTH 125
# define SLOW 3
# define RAPID 5

// program buffers
volatile uint8_t program[BUFFER_LENGTH];
volatile uint8_t rec_program[BUFFER_LENGTH];

// length of current program
volatile int length = 0;
// length of received program
volatile int rec_length = 0;
// playback position
volatile int step = 0;
// playback delay (ms)
volatile uint16_t tempo = 2;

void swapArrays(char **a, char **b){
	char *temp = *a;
	*a = *b;
	*b = temp;
}

uint8_t tempos[5] = {
	10000,
	20000,
	30000,
	40000,
	50000
};

void handlePayload(char command, int len, char payload[]) {
	uint8_t i;
	switch (command) {
		case 0x01:  // TEMPO
		/*
			i = payload[1];
		
		
			char smthing[150];
			memset(smthing, 0, 150);
			sprintf(smthing, "tempo=%d", i);
			USART_SendString(smthing);
			*/
			/*if (i >= 0 && i < 5) {
				OCR1A = tempos[i];
			}*/
			//OCR1A = i;
			break;
			
		case 0x02:  // RESET (restart nåværende program)
			step = 0;
			break;
	
		case 0x03:  // MOTTA PROGRAM  (dump payload inn i *rec_program)
			memcpy(rec_program, payload, len);	
			rec_length = len;
			break;
		
		case 0x04: // BYTT PROGRAM (bytt om referanser og bruk av *program og *rec_program)
			swapArrays(program, rec_program);
			length = rec_length;
			step = 0;  // og RESET!
			break;
			
		default:
			break;
	}
}

ISR (USART_RXC_vect)
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


ISR (TIMER1_COMPA_vect)
{
	if ((step >= length) || (step == BUFFER_LENGTH)) step = 0;
	
	PORTB = program[step++];  // *(program + step++);
}

void setupTimerISR()
{
	TCCR1A = 0; // Reset control registers timer1 /not needed, safety
	TCCR1B = 0; // Reset control registers timer1 // not needed, safety
	TIMSK |= (1 << OCIE1A); // | (1 << TOIE1); //timer1 output compare match and overflow interrupt enable
	OCR1A = 50000; // Set TOP/compared value (your value) (maximum is 65535 i think)
	TCCR1B |= (1 << WGM12)|(1 << CS11);  // prescaling=8 CTC-mode (two counts per microsecond)
	TCNT1 = 0;
	//TCCR1A |= (1 << OCR1A);  //set OCR1A on compare match (output high level level)
}

int main(void)
{
	char _buffer[150];
	uint8_t Connect_Status;
	uint8_t Sample = 0;
	
	DDRB = 0xFF; // set PORTB for output
	PORTB = 0b01111111; // crash indicator (LED 7)
	
	// create an initial program to keep the loop busy until first program is received (and started)
	program[0] = 0b01111111;
	program[1] = 0b10111111;
	program[2] = 0b11011111;
	program[3] = 0b11101111;
	program[4] = 0b11110111;
	program[5] = 0b11111011;
	program[6] = 0b11111101;
	program[7] = 0b01111110;

	length = 8;
	step = 0;
	
	cli();
	//setupTimerISR();

	
	USART_Init(115200);						/* Initiate USART with 115200 baud rate */
	sei();									/* Start global interrupt */

	USART_SendString("HEI VELKOMMEN VERDEN");
	
	while(!ESP8266_Begin());
	ESP8266_WIFIMode(BOTH_STATION_AND_ACCESPOINT);/* 3 = Both (AP and STA) */
	ESP8266_ConnectionMode(SINGLE);			/* 0 = Single; 1 = Multi */
	ESP8266_ApplicationMode(NORMAL);		/* 0 = Normal Mode; 1 = Transperant Mode */
	if(ESP8266_connected() == ESP8266_NOT_CONNECTED_TO_AP)
	ESP8266_JoinAccessPoint(SSID, PASSWORD);
	ESP8266_Start(0, DOMAIN, PORT);
	
	
	PORTB = 0xFF; // All leds off
	unsigned char payload[10];
	
	while(1)
	{
		/*
		Connect_Status = ESP8266_connected();
		if(Connect_Status == ESP8266_NOT_CONNECTED_TO_AP)
		ESP8266_JoinAccessPoint(SSID, PASSWORD);
		if(Connect_Status == ESP8266_TRANSMISSION_DISCONNECTED)
		ESP8266_Start(0, DOMAIN, PORT);
		*/
		
		/*
		if (!tempoSent && Sample++ > 5) {
			memset(_buffer, 0, 150);
			sprintf(_buffer, "KLAR FOR TEMPO");
			ESP8266_Send(_buffer);
			
			tempoSent = true;
			_delay_ms(500);
		}
		*/
	
		int len = 0;
		memset(_buffer, 0, 150);
		len = Read_Data(_buffer);
		
		char * pbuffer_cmd = strstr(_buffer, "+IPD");
		if (len > 0 && pbuffer_cmd != NULL) {
			
			//PORTB = 0b00111111;

			while (pbuffer_cmd != NULL) {

				char *pbuffer_len = strstr(pbuffer_cmd, ":");

				int startpos = (int)(pbuffer_cmd - _buffer);
				int endpos = (int)(pbuffer_len - _buffer);

				// hent ut lengde av pakke (IPD)
				strncpy(payload, pbuffer_len-1, endpos-(startpos+5));
				int len = 0;
				
				sscanf(payload, "%d", &len);
				strncpy(payload, pbuffer_len+1, len);
				
				unsigned int dKommando = payload[0];
				unsigned int dLengde = payload[1];
				
				// +1 for å klarere :, +2 for å gå forbi kommando- og lengde-bytes
				strncpy(payload, pbuffer_len+1 +2, dLengde);
				payload[dLengde] = 0;
				
				unsigned int ttt = payload[0];
				
				PORTB = ttt; // (0xFF ^ dKommando);
				//handlePayload(dKommando, dLengde, payload);

				pbuffer_cmd = strstr(pbuffer_len, "+IPD");
			}
			
		}
		_delay_ms(600);

	}
}
