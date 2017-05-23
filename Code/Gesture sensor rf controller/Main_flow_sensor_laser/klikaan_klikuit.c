/* 2011
TESTPROGRAM FOR DEMONSTRATING THE USE OF Klikaan_klikuit.c

Klikaan_klikuit is a program to control a wireless home remote set, with the use of a 433MHz transmitter.
(brand "Implus" from "Action" store)

The program works @8MHz, and the bittime is 0,0001713sec. so 1384 is the setting for OCR1A. (no exactly but this value is correct)
Make sure to check what the exact bittime and delaytime (between to bursts)!
You probably have to compensate some

The use of the program is simple, just init the timer and ports and use send_klikaan_klikuit
with the name and action, i.e. a_on = turn on adapter a which is set (with the dipswitches) to a.

AUTHOR: JJSHORTCUT

*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
 #include "dbg_putchar.h"

#define PORT		PORTB
#define PINNUMBER	PB0

volatile uint8_t bit_counter = 0;
volatile uint8_t byte_counter = 0;
volatile uint8_t code_to_send = 0;
volatile uint8_t sending = 0;
uint8_t init_klikaan_klikuit_flag = 0;

// Array for sending A_AAN	
const uint8_t codes[10][13] PROGMEM =	// Make 2d array, first array is for selecting the code, second is the code array itself
	{
		{	// Array [0][] = a_on = 0
		0b11101110,0b11101110,0b11101110,0b11101110,
		0b11101110,0b10001000,0b10001110,0b10001110,
		0b10001110,0b10001110,0b10001110,0b10001000,
		0b10000000
		},
		{	// a_off = 1
		0b11101110,0b11101110,0b11101110,0b11101110,
		0b11101110,0b10001000,0b10001110,0b10001110,
		0b10001110,0b10001110,0b10001000,0b10001110,
		0b10000000
		},
		{	// b_on = 2
		0b11101110,0b11101110,0b11101110,0b11101110,
		0b11101110,0b10001110,0b10001000,0b10001110,
		0b10001110,0b10001110,0b10001110,0b10001000,
		0b10000000
		},
		{	// b_off = 3
		0b11101110,0b11101110,0b11101110,0b11101110,
		0b11101110,0b10001110,0b10001000,0b10001110,
		0b10001110,0b10001110,0b10001000,0b10001110,
		0b10000000},
		{	// c_on = 4
		0b11101110,0b11101110,0b11101110,0b11101110,
		0b11101110,0b10001110,0b10001110,0b10001000,
		0b10001110,0b10001110,0b10001110,0b10001000,
		0b10000000
		},
		{	// c_off = 5
		0b11101110,0b11101110,0b11101110,0b11101110,
		0b11101110,0b10001110,0b10001110,0b10001000,
		0b10001110,0b10001110,0b10001000,0b10001110,
		0b10000000},
		{	// d_on = 6
		0b11101110,0b11101110,0b11101110,0b11101110,
		0b11101110,0b10001110,0b10001110,0b10001110,
		0b10001000,0b10001110,0b10001110,0b10001000,
		0b10000000
		},
		{	// d_off = 7
		0b11101110,0b11101110,0b11101110,0b11101110,
		0b11101110,0b10001110,0b10001110,0b10001110,
		0b10001000,0b10001110,0b10001000,0b10001110,
		0b10000000},
		{	// e_on = 8
		0b11101110,0b11101110,0b11101110,0b11101110,
		0b11101110,0b10001110,0b10001110,0b10001110,
		0b10001110,0b10001000,0b10001110,0b10001000,
		0b10000000
		},
		{	// e_off = 9
		0b11101110,0b11101110,0b11101110,0b11101110,
		0b11101110,0b10001110,0b10001110,0b10001110,
		0b10001110,0b10001000,0b10001000,0b10001110,
		0b10000000
		}
	};


ISR(TIMER0_COMPA_vect)
{
	if(byte_counter<=12)	// 13 times a byte
	{
		if(bit_counter<=7)	// send 8 bits
		{
			uint8_t b = (pgm_read_byte(&codes[code_to_send][byte_counter]));	// Get byte from program memory 2d array
			b = ( b << bit_counter);	// Select the correct bit, arrange it to MSB place
			b = ( b >> 7);				// Arrange this bit to LSB (everything else is 0)

			if(b==1)
			{
				PORT |= ( 1 << PINNUMBER);	// Send 1 to pin
			}
			if(b==0)
			{
				PORT &= ~(1 << PINNUMBER);	// Send 0 to pin
			}
			
			bit_counter++;
		}	
		else
		{
			bit_counter = 0;
			byte_counter++;
		}
	}
	else
	{
		byte_counter = 0;
		sending = 1;			// Sending finished
		TCCR0B &= ~(1 << CS00);	// Stop timer1
	}
} 


void init_klikaan_klikuit(void)
{
	/* Attiny 45 uses timer0 (8bit)
	Timer0 8bit
	@1Mhz
	Bittime = 0,0001713 sec
	No prescaler = 1MHz = 0.000001 sec
	Compare value for 1 bit = 0,0001713/0.000001 = 171,3 we take 171
	// tweaked for this chip = 166
	*/
	TCCR0A |= (1<<WGM01);	// CTC mode 2 //TCCR1B |= (1 << WGM12); // Configure timer 1 for CTC mode
	TIMSK |= (1<<OCIE0A);	// Enable compare A interrupt //TIMSK1 |= (1 << OCIE1A); // Enable CTC interrupt
   	OCR0A   = 166;			// Set compare value // Set top timer/counter value, compare value of timer1 (@8MHz no prescaling)
							// 1/8MHz = 0,000000125 sec.
							// Bittime = 171,3 us = 0,0001713 sec.
							// Delaytime = 5328us
							// Exact Compare value = 0,0001713/0,000000125 = 1370 (timer 1 is a 16 bit timer so it fits)
							// Intern oscilator -> 1409 (171,35us)
							// Extern crystal -> 1364 (171,25us)
	TCCR0B |= (1<<CS00);	// No prescaler, start timer
	sei(); 					// Enable global interrupts						
	DDRB |= (1<<PINNUMBER);	// Set pin as output 
	PORTB |=(1<<PINNUMBER);	// Make output low							
	init_klikaan_klikuit_flag = 1;
}

void send_klikaan_klikuit(uint8_t a)
{
	if (!init_klikaan_klikuit_flag)
	{
		init_klikaan_klikuit();
	}

	code_to_send = a;
	for (int i=0;i<5;i++)		// Send burst at least 3 times
	{
		TCCR0B |= (1 << CS00); 	// Start timer0, no prescale
		while((sending) ==0){};	// While sending the burst
		sending = 0;			// Reset "sending"
		_delay_us(3740);		// Delay between 2 bursts = 5,328ms // was 5400 // 4712 = -4 bittimes
								// Internal oscilator -> 3920us (5286us)
								// External crystal -> 3740us (5280us)
	}
}

