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
}


volatile int length = 0;
volatile int rec = 0;
volatile int step = 0;
volatile int tempo = RAPID;
volatile unsigned char program[256];
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
	send_char((ID >> 5) ^ 0b11000000);  // echo

	char initmelding3[] = "Starting...";
	send_string(initmelding3);
	
	
	// after that, discard all bytes that don't start with ID (in 3 first bits)
			
	PORTB = 0xFF; // turn OFF all LEDs
	
	step = 0;
	while (1) {
		if ((step >= length) || (step == 256)) step = 0;
		
		PORTB = program[step++];
		
		my_delay_ms(tempo);
	}
	return 1;
}

void my_delay_ms(int n) {
	int mem = tempo;
	while(n--) {
		if (mem != tempo) break;
		_delay_ms(1);
	}
}

ISR ( USART_RXC_vect , ISR_BLOCK )
{
	char ReceivedByte;
	char payload;
	ReceivedByte = UDR;
	
	if (ID_mottatt == 0) {
		//char initmelding[] = "--ID received!--";
		//send_string(initmelding);
		
		ID = (ReceivedByte & 0b11100000);
		ID_mottatt = 1;
	} else {
		
		//char initmelding[] = "her";
		//send_string(initmelding);
	
		if ((ReceivedByte & 0b11100000) == ID) {
			//send_char('j');
			
			//char resetPattern = (0b00011111 & 0b00011111);
			
			if ((ReceivedByte & 0b00000011) == 0b00000011) { 
				length = 0; // RESET: Prepare for new block of data (program)
				step = 0;
				rec = 0;
				//send_char('r');

			} else {
				//send_char('b');
				payload = (ReceivedByte & 0b00011100) >> 2;
				
				char a = (payload & 0b00000001 == 0b00000001 ? '1' : '0');
				char b = (payload & 0b00000010 == 0b00000010 ? '1' : '0');
				char c = (payload & 0b00000100 == 0b00000100 ? '1' : '0');
				char tmp[6] = {rec+48,':',a,b,c, 0x00};
				send_string(tmp);
				
				program[rec++] = (payload ^ 0b11111111);  // inverted logic
				
				if (rec >= 256) rec = 0;
				else ++length;
			}
		} else {
			// common reception stuff:
			if ((ReceivedByte & 0b00000011) == 0b00000001) {
				payload = (ReceivedByte & 0b11111100);
				tempo = payload * 10;	
			}
		}
	}
}
