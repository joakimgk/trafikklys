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

int i = 0;
unsigned char program[256];
unsigned char ID = -1;

int main(void){
	
	UCSRB = (1 << RXEN ) | (1 << TXEN ); // Turn on the transmission and reception circuitry
	UCSRC = (1 << URSEL ) | (1 << UCSZ0 ) | (1 << UCSZ1 ); // Use 8- bit character sizes - URSEL bit set to select the UCRSC register

	// Load upper 8- bits of the baud rate value into the high byte of the UBRR register
	UBRRH = ( BAUD_PRESCALE >> 8);
	// Load lower 8- bits of the baud rate value into the low byte of the UBRR register
	UBRRL = BAUD_PRESCALE ;

	DDRB = 0xFF; // set PORTB for output
	PORTB = 0x00; // turn ON all LEDs initially (to indicate ready)

	DDRA = 0x00; // set PORTA for input
	
	// INIT:
	char initmelding[] = "Press SW0 to start initialization...";
	send_string(initmelding);
		
	// blink leds until SW0 is pressed
	int state = 0;
	unsigned char input = -1;
	while (input != 0b11111110) {  // NB! inverted logic, also on inputs!
		input = PINA;
		
		PORTB = (state++ % 2 == 0 ? 0xFF : 0x00);
		_delay_ms(SLOW); 
		send_char(input);
	}
	
	// 
	char initmelding2[] = "Tast ID, 1-16";
	send_string(initmelding2);
	
	// then blink rapidly until byte (ID) is received
	state = 0;
	while (ID < 0) {
		PORTB = (state++ % 2 == 0 ? 0xFF : 0x00);
		_delay_ms(RAPID); 
	}
	send_char(ID);  // echo
	
	// after that, discard all bytes that don't start with ID (in 3 first bits)
			
	PORTB = 0xFF; // turn OFF all LEDs
	
	int step = 0;
	while (1) {
		if (step < i) {
			PORTB = program[step++];
			_delay_ms(SLOW);
		}
	}
	return 1;
}



ISR ( USART_RXC_vect , ISR_BLOCK )
{
	char ReceivedByte;
	char payload;
	ReceivedByte = UDR;
	
	if (ID < 0) {
		ID = (ReceivedByte & 0b11100000);
		char initmelding[] = "--ID received!--";
		send_string(initmelding);
	} else {
	
		if ((ReceivedByte & 0b11100000) == ID) {
			
			if ((ReceivedByte & 0b00011111) == 31) { 
				i = 0; // RESET: Prepare for new block of data (program)
			} else {
				payload = (ReceivedByte & 0b00011100) << 3;
				program[i] = payload;
			}
		}
	}
}
