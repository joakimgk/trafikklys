#include <avr/io.h> // avr header file for IO ports
#include <avr/interrupt.h>

#define F_CPU 8000000UL
#include <util/delay.h>

# define USART_BAUDRATE 9600
// Naive Version - has rounding bugs !
//# define BAUD_PRESCALE ((( F_CPU / ( USART_BAUDRATE * 16 UL))) - 1)

// Better Version - see text below
# define BAUD_PRESCALE (((( F_CPU / 16) + ( USART_BAUDRATE / 2)) / ( USART_BAUDRATE )) - 1)


# define SLOW 1000
# define RAPID 250
# define BUFFER_LENGTH 125
# define TEMPO_SCALE 10

void send_char(char c)
{
	while ((UCSRA & (1 << UDRE)) == 0) {};
	UDR = c;
}

void send_string(char *s)
{
	while (*s != 0x00)
	{
		send_char(*s);
		s++;
	}
	send_char('\r');
	send_char('\n');
}

// program to play back
volatile unsigned char program[BUFFER_LENGTH];
// receive buffer
volatile unsigned char buffer[BUFFER_LENGTH];
volatile int buf_length = 0;

// length of current program
volatile int length = 0;
// record position
volatile int rec = 0;
// playback position
volatile int step = 0;
// playback delay (ms)
volatile int tempo = RAPID;

volatile unsigned char ID;
volatile unsigned int ID_mottatt;

int main(void){
	
	UCSRB = (1 << RXEN ) | (1 << TXEN ); // Turn on the transmission and reception circuitry
	UCSRC = (1 << URSEL ) | (1 << UCSZ0 ) | (1 << UCSZ1 ); // Use 8- bit character sizes - URSEL bit set to select the UCRSC register

	// Load upper 8- bits of the baud rate value into the high byte of the UBRR register
	UBRRH = ( BAUD_PRESCALE >> 8);
	// Load lower 8- bits of the baud rate value into the low byte of the UBRR register
	UBRRL = BAUD_PRESCALE ;

	UCSRB |= (1 << RXCIE );
	sei();

	DDRB = 0xFF; // set PORTB for output
	PORTB = 0x00; // turn ON all LEDs initially (to indicate ready)

	DDRA = 0x00; // set PORTA for input
	
	
	// INIT:
	char initmelding[] = "Press SW0 to start initialization...";
	send_string(initmelding);
		
	// blink leds until SW0 is pressed
	int state = 0;
	unsigned char input = 0;
	while (input != 0b11111110) {  // NB! inverted logic, also on inputs!
		input = PINA;
		
		PORTB = (state++ % 2 == 0 ? 0xFF : 0x00);
		_delay_ms(SLOW);
	}
	
	// 
	char initmelding2[] = "Tast ID, 1-16";
	send_string(initmelding2);
	
	// then blink rapidly until byte (ID) is received
	state = 0;
	ID = 0x00;
	ID_mottatt = 0;
	while (ID_mottatt == 0) {
		PORTB = (state++ % 2 == 0 ? 0xFF : 0x00);
		_delay_ms(RAPID); 
	}
	
	// echo
	char c = ((ID & 0b00100000) == 0b00100000 ? '1' : '0');
	char b = ((ID & 0b01000000) == 0b01000000 ? '1' : '0');
	char a = ((ID & 0b10000000) == 0b10000000 ? '1' : '0');
	char echo[7] = {'I','D','=',a,b,c,0x00};
	send_string(echo);

	char initmelding3[] = "Starting...";
	send_string(initmelding3);
			
	// create an initial program to keep the loop busy until first program is received (and started)
	program[0] = 0xFF;
	length = 1;
	step = 0;
	
	while (1) {
		if ((step >= length) || (step == BUFFER_LENGTH)) step = 0;
		
		PORTB = program[step++];
		
		my_delay_ms(tempo);
	}
	return 1;
}

void my_delay_ms(int n) {
	int mem = tempo;
	while(n--) {
		// interrupt delay if tempo changes
		if (mem != tempo) break;
		_delay_ms(1);
	}
}

ISR ( USART_RXC_vect , ISR_BLOCK )
{
	char ReceivedByte;
	char payload;
	ReceivedByte = UDR;
	char _ID_mask = 0b11100000;
	char _instruction_mask = 0b00000011;
	char _payload_mask = 0b00011100;
	char _tempo_mask = 0b11111100;
	char _invert_mask = 0b11111111;
	
	char _reset_instruction = 0b00000011;
	char _tempo_instruction = 0b00000001;
	
	if (ID_mottatt == 0) {
		ID = (ReceivedByte & _ID_mask);
		ID_mottatt = 1;
		//send_char('i');
	} else {
	
		if ((ReceivedByte & _ID_mask) == ID) {
			// addressed bytes (to this receiver):
			
			if ((ReceivedByte & _instruction_mask) == _reset_instruction) { 
				buf_length = 0; // RESET: Prepare for new block of data (program)
				rec = 0;
				//send_char('r');

			} else {
				//send_char('b');
				payload = (ReceivedByte & _payload_mask) >> 2;
				
				/*
				char a = ((payload & 0b00000001) == 0b00000001 ? '1' : '0');
				char b = ((payload & 0b00000010) == 0b00000010 ? '1' : '0');
				char c = ((payload & 0b00000100) == 0b00000100 ? '1' : '0');
				char tmp[6] = {rec+48,':',a,b,c,0x00};
				send_string(tmp);
				*/
				
				buffer[rec++] = (payload ^ _invert_mask);  // inverted logic on LEDs
				
				if (rec >= BUFFER_LENGTH) rec = 0;
				else ++buf_length;
			}
		} else {
			// common reception stuff:
			
			// change tempo
			if ((ReceivedByte & _instruction_mask) == _tempo_instruction) {
				//send_char('T');
				payload = (ReceivedByte & _tempo_mask);
				tempo = payload * TEMPO_SCALE;
				
			// start new program
			} else if ((ReceivedByte & _instruction_mask) == _reset_instruction) {
				//send_char('S');
				// copy buffer to program
				for (int j = 0; j < buf_length; j++) 
					program[j] = buffer[j];
				
				length = buf_length;
				step = 0;
			}
		}
	}
}
