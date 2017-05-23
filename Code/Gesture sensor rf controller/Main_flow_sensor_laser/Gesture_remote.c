/*
 * Main_flow_sensor_laser.c
 *
 * Created: 5/20/2014 9:16:13 PM
 *  Author: Jan-Jaap
 *
 *	@4.8MHZ/8 - 600Khz (now @ 1Mhz with attiny85)
 *  Brownout @4.3v
 *	Pinout:
 *	PB0 = RF433 MHz sender
 *	PB1 = TX debug 9600 8N1, LED
 *	PB2 = INT0, for gesture chip ADPS-9960
 *	PB3 = SCL I2C ADPS-9960
 *	PB4 = SDA I2C ADPS-9960
 *	PB5 = Reset
 *
 */ 

#include <stdlib.h> 
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/sleep.h>

#include <avr/pgmspace.h>
#include "dbg_putchar.h"
#include "Klikaan_klikuit.h"
#include "APDS9960.h"

#define APDS9960_INT	PB2 // Needs to be an interrupt pin

#define FLOW_THRESHOLD 4	// Threshold of output vs. flow
#define EXT_AS_LED_SWITCH 1

#define BLINK_DELAY_MS 250
#define LONG_DELAY 500
#define SHORT_DELAY 150
#define DEBUG_MSG 1

/* Pinouts */
/* PB1 = serial debug TX defined in dbg_putchar.c */

// Use jumper to enable led (no TX then)
#define LED_PIN PB1
#define LED_ON	(PORTB |= (1 << LED_PIN))		// LED on
#define LED_OFF	(PORTB &= ~(1 << LED_PIN))		// LED off
#define LED_TOGGLE (PORTB ^= (1 << LED_PIN))		// LED toggle

// Prototypes
void init_IO(void);
void init_interrupt(void);
void send_start_settings(void);
void uart_puts(const char *s);				// Print strings from RAM 
void uart_puts_p(const char *progmem_s);	// Print strings from PROGMEM (FLASH)
void blink(uint8_t nr_of_blinks, uint16_t delay_ms);
void LED_alarm(void);
void my_delay_ms(uint8_t n);
void handleGesture(void); 
void disable_gesture_int(uint8_t interrupt);
void klikaan_klikuit_off(void);
void do_sleep(void);

volatile uint8_t int_flag = 0;
uint8_t a_flag = 0, b_flag = 0, c_flag = 0, d_flag = 0, e_flag = 0;

ISR (INT0_vect)
{
	// Do something?
	int_flag = 1;
	//uart_puts("int!\n");
	//LED_TOGGLE;
}

int main(void)
{
	//unsigned char i2cdata[8];
	init_IO();
	#if(DEBUG_MSG)
	{
		dbg_tx_init();
	}
	#else
	{
		blink(4, SHORT_DELAY);
	}
	#endif
	
	init_klikaan_klikuit();
	klikaan_klikuit_off();	// all klikaan off
	
	#if DEBUG_MSG
	send_start_settings();
	#endif
	init_interrupt();// first enable interrupt RUN FIRST??
	
	if (init_apds()) 
	{
		#if DEBUG_MSG
		uart_puts_p(PSTR("APDS-9960 init complete!\n"));
		#endif
	} 
	else 
	{
		#if DEBUG_MSG
		uart_puts_p(PSTR("APDS-9960 init failed!\n"));
		#endif
	}
	
	if (enableGestureSensor(1))
	{
		#if DEBUG_MSG
		uart_puts_p(PSTR("Gesture sensor now running!\n"));
		#endif
	}
	else
	{
		#if DEBUG_MSG
		uart_puts_p(PSTR("Something went wrong during gesture sensor init!"));
		#endif
	}

	while(1) 
	{
		if (int_flag)
		{
			disable_gesture_int(true);	// disable
			uart_puts_p(PSTR("Int trig!\n"));
			handleGesture();
			int_flag = 0;
			disable_gesture_int(false);	// enable
		}
		//uart_puts("go to sleep\n");
		//do_sleep();	// sleep until int0 trigger
		
		//send_klikaan_klikuit(c_off_1);
	}	
}

void init_IO(void)
{
	DDRB |= (1<<LED_PIN);					// Output pin for LED
}

void init_interrupt(void)	// int of apds9960
{
	GIMSK |= (1<<INT0);		// Enable INT0
	MCUCR |= (1<<ISC01);	// INT0 on Falling edge
	//MCUCR |= (1<<ISC01);	// Any change
	sei();					// Enable global interrupts
}	

void klikaan_klikuit_off(void)
{
	a_flag=0;
	b_flag=0;
	c_flag=0;
	d_flag=0;
	e_flag=0;
	send_klikaan_klikuit(a_off_1);
	send_klikaan_klikuit(b_off_1);
	send_klikaan_klikuit(c_off_1);
	send_klikaan_klikuit(d_off_1);
	send_klikaan_klikuit(e_off_1);
}

void disable_gesture_int(uint8_t interrupt)
{
	if (!interrupt)	// enable
	{
		GIMSK |= (1<<INT0);		// Enable INT0	
	}
	else  // Disable
	{
		GIMSK &= ~(1<<INT0);		// Disable INT0
	}
}

void send_start_settings(void)
{
	uart_puts_p(PSTR("Gesture recognition APDS-9960 chip test v1.0\n"));
}

void my_delay_ms(uint8_t n) 
{
	while(n--) {
		_delay_us(1);
	}
}

void blink(uint8_t nr_of_blinks, uint16_t delay_ms)
{
	uint16_t delay_time_ms = BLINK_DELAY_MS;
	if ((delay_ms > 0) && (delay_ms <= 5000))	// delay should not be more than 5 sec..
	{
		delay_time_ms = delay_ms;
		if (nr_of_blinks<2)	// if 1 blink
		{
			LED_ON;
			for (int16_t i=0;i<(delay_time_ms);i++)
			{
				_delay_ms(1);
			}
			LED_OFF;
		}
		else
		{
			for (int i=0;i<nr_of_blinks;i++)
			{
				LED_ON;
				for (int16_t i=0;i<(delay_time_ms/2);i++)
				{
					_delay_ms(1);
				}
				//_delay_ms(delay_time_ms);
				LED_OFF;
				for (int16_t i=0;i<(delay_time_ms/2);i++)
				{
					_delay_ms(1);
				}
				//_delay_ms(delay_time_ms);
			}
		}
	}
}

void handleGesture(void) 
{
	if ( isGestureAvailable() ) 
	{
		switch ( readGesture() ) 
		{
			case DIR_UP:
				#if DEBUG_MSG
				uart_puts("UP:");
				#endif
				if (a_flag) {
					//send_klikaan_klikuit(a_off_1);
					#if DEBUG_MSG
					uart_puts("A OFF\n");
					#else
					blink(1, SHORT_DELAY);
					#endif
					a_flag=0;
				}
				else {
					//send_klikaan_klikuit(a_on_1);
					#if DEBUG_MSG
					uart_puts("A ON\n");
					#else
					blink(1, SHORT_DELAY);
					#endif
					a_flag=1;
				}
			break;
			case DIR_DOWN:
				#if DEBUG_MSG
				uart_puts("DOWN:");
				#endif
				if (c_flag) {
					//send_klikaan_klikuit(c_off_1);
					#if DEBUG_MSG
					uart_puts("C OFF\n");
					#else
					blink(3, SHORT_DELAY);
					#endif
					c_flag=0;
				}
				else {
					//send_klikaan_klikuit(c_on_1);
					#if DEBUG_MSG
					uart_puts("C ON\n");
					#else
					blink(3, SHORT_DELAY);
					#endif
					c_flag=1;
				}
			break;
			case DIR_LEFT:
				#if DEBUG_MSG
				uart_puts("LEFT: ");
				#endif
				if (d_flag) {
					//send_klikaan_klikuit(d_off_1);
					#if DEBUG_MSG
					uart_puts("D OFF\n");
					#else
					blink(4, SHORT_DELAY);
					#endif
					d_flag=0;
				}
				else {
					//send_klikaan_klikuit(d_on_1);
					#if DEBUG_MSG
					uart_puts("D ON\n");
					#else
					blink(4, SHORT_DELAY);
					#endif
					d_flag=1;
				}
			break;
			case DIR_RIGHT:
				#if DEBUG_MSG
				uart_puts("RIGHT:");
				#endif
				if (b_flag) {
					//send_klikaan_klikuit(b_off_1);
					#if DEBUG_MSG
					uart_puts("B OFF\n");
					#else
					blink(3, SHORT_DELAY);
					#endif
					b_flag=0;
				}
				else {
					//send_klikaan_klikuit(b_on_1);
					#if DEBUG_MSG
					uart_puts("B ON\n");
					#else
					blink(3, SHORT_DELAY);
					#endif
					b_flag=1;
				}
			break;
			case DIR_NEAR:
				#if DEBUG_MSG
				uart_puts("NEAR:");
				#endif
			break;
			case DIR_FAR:
				#if DEBUG_MSG
				uart_puts("FAR:");
				#endif
				//klikaan_klikuit_off();
				#if DEBUG_MSG
				uart_puts("ALL OFF\n");
				#else
				blink(5 ,SHORT_DELAY);
				#endif
			break;
			case ERROR:
				#if DEBUG_MSG
				uart_puts("ERROR!\n");
				#else
				blink(1, LONG_DELAY);
				#endif
			break;
			default:
				#if DEBUG_MSG
				uart_puts("NONE\n");
				#else
				blink(1, LONG_DELAY);
				#endif
		}
	}
}

void do_sleep()
{
	ADCSRA	&= ~(1 << ADEN);					// Disable ADC, save power
	ACSR |= _BV(ACD);							//disable the analog comparator
	//PRR |= (1 << PRTIM1) | (1 << PRTIM0) | (1 << PRUSI) | (1 << PRADC); // Power down all peripherals ALSO TIMERS!
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);		// MCU into power-down mode
	sleep_enable();								// Set sleep enable (SE) bit
	//GIMSK |= (1<<INT0);							// enable INT0
	init_interrupt();
	//sleep_bod_disable();						// Put the device to sleep
	//sei();
	sleep_cpu();			// Put the device to sleep
	cli();
	uart_puts("wakeup!\n");
	sleep_disable();		// Upon waking up, sketch continues from this point
	sei();					// Keep int0 enabled!
	init_klikaan_klikuit();
}
