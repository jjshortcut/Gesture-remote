
/*
  Jesper Hansen <jesperh@telia.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation, 
  Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.





  	Thermostat regulator with Dallas DS1621.
  	----------------------------------------	
    
  	2000-11-24  



	CPU: At90S2313

	Switches are at PD2 and PD3, common ground

 	LED segments a-g are at PB0 - PB7, and
 	the anodes are at PD0 and PD1
 	The relay are at PD6, active high

 	Dallas DS1621 I2C bus are connected at 
 	PD4 = SDA 
 	PD5 = SCL

*/


#include <io.h>
#include <io.h>
#include <progmem.h>
#include <eeprom.h>
#include <signal.h>
#include <interrupt.h>

#include "delay.h"
#include "i2c.h"



// PORT D
#define KEYUP	PD2
#define KEYDN	PD3

#define	LED1	PD0
#define LED2	PD1

#define SDA		PD4
#define SCL		PD5

#define RELAY	PD6


// PORT B

#define SEG_a	0x01
#define SEG_b	0x02
#define SEG_c	0x04
#define SEG_d	0x08
#define SEG_e	0x10
#define SEG_f	0x20
#define SEG_g	0x40
#define SEG_dot	0x80

// MISC

// eeprom storage positions
#define EEPROM_SETTEMP		0x11
#define EEPROM_HYSTERESIS	0x13
#define EEPROM_DUTYVALUE	0x15


#define BLINKCOUNT	80

#define KEY_UP		0x01
#define KEY_DOWN 	0x02
#define KEY_BOTH	(KEY_UP|KEY_DOWN)

//
// mode setting enums
//
typedef enum mode_e
{
	NORMAL,
	SET_HYST,
	SET_DUTY,
} mode_e;


mode_e next_mode[] = {SET_HYST, SET_DUTY, NORMAL};
mode_e mode = NORMAL;

unsigned char digits[] = {
	(SEG_a|SEG_b|SEG_c|SEG_d|SEG_e|SEG_f),			// 0
	(SEG_b|SEG_c),									// 1
	(SEG_a|SEG_b|SEG_d|SEG_e|SEG_g),				// 2
	(SEG_a|SEG_b|SEG_c|SEG_d|SEG_g),				// 3
	(SEG_b|SEG_c|SEG_c|SEG_f|SEG_g),				// 4
	(SEG_a|SEG_c|SEG_d|SEG_f|SEG_g),				// 5
	(SEG_a|SEG_c|SEG_d|SEG_e|SEG_f|SEG_g),			// 6
	(SEG_a|SEG_b|SEG_c),							// 7
	(SEG_a|SEG_b|SEG_c|SEG_d|SEG_e|SEG_f|SEG_g),	// 8
	(SEG_a|SEG_b|SEG_c|SEG_d|SEG_f|SEG_g),			// 9
	(SEG_a),										// mode 1 indicator
	(SEG_g),										// mode 2 indicator
	(SEG_d),										// mode 3 indicator
};



unsigned char set_temp = 20;		// initial set temperature
unsigned char set_flag = 0;			// no settings in progress

unsigned char hysteresis = 1;		// initial hysteresis
unsigned char dutyvalue = 3;		// initial duty-cycle
unsigned char relay_on = 0;			// initial relay state



// timer 0 handles multiplex and refresh of the displays

#define TI0_L		(256-64)	

volatile unsigned char flip = 0;
volatile unsigned char dutycount = 0;
volatile unsigned char temp = 20;
unsigned char dotcount = BLINKCOUNT;

SIGNAL(SIG_OVERFLOW0)	//timer 0 overflow
{
	unsigned char b = 0;

	// reload timer
	outp(TI0_L, TCNT0);

	// both displays off

	sbi(PORTD, LED1);
	sbi(PORTD, LED2);

	dutycount++;
	if (dutycount == dutyvalue)
	{
		dutycount = 0;

		switch (mode)
		{
			case NORMAL:
				// see which one we need to update
				if (flip)	// high ?
					b = (temp / 10);
				else
					b = temp % 10;
				break;

			case SET_HYST :
				// see which one we need to update
				if (flip)	// high ?
					b = 10;	// mode indicator
				else
					b = hysteresis;
				break;

			case SET_DUTY :
				// see which one we need to update
				if (flip)	// high ?
					b = 11;	// mode indicator
				else
					b = dutyvalue;
				break;
				
		}

		// set digit data on port
		outp(digits[b], PORTB);

		if (flip)	// left LED
		{
			if (dotcount < BLINKCOUNT/2)
				sbi(PORTB,7);
				
			if (dotcount)
				dotcount--;
			else
				dotcount = BLINKCOUNT;	
		
			cbi(PORTD, LED1);
		}
		else
		{
			if (relay_on)
				sbi(PORTB,7);
			
			cbi(PORTD, LED2);
		}
		
		flip = !flip;
	}
}




void start_timer_0(void)
{
	//setup timer 0

	outp(0x03, TCCR0);		// prescaler /64  tPeriod = 10.667 uS
	sbi(TIMSK, TOIE0);	 	// enable timer1 interrupt
	outp(TI0_L,TCNT0);		// load timer  

	sei();					// start timer by enabling interrupts
}



int getkey(void)
{
	int ret_val = 0;

	// if none of the keys are down,
	// return zero
	if (bit_is_set(PIND, KEYUP) && bit_is_set(PIND, KEYDN) )
		return 0;

	// debounce 80 mS
	delay(40000);	
	delay(40000);	
		
	// now read again and return status
	if (bit_is_clear(PIND, KEYUP))
		ret_val |= KEY_UP;

	if (bit_is_clear(PIND, KEYDN))
		ret_val |= KEY_DOWN;

	return ret_val;
}			



int main(void) 
{
	unsigned char i2cdata[8];

	// set port B as all outputs
	// and initially set all pins HI

	outp(0xff,PORTB);
	outp(0xff,DDRB);


	// set common LED pins as output
	// and initially set them HI
	// set RELAY pin as output, and initially set it LO (off)
	// also activate pullups on keyboard inputs
	outp((1<<LED1)|(1<<LED2)|(1<<RELAY),DDRD);
	outp((1<<LED1)|(1<<LED2)|(1<<KEYUP)|(1<<KEYDN),PORTD);

	i2c_init();

	// restore data from eeprom
	// and make sure values are sane
	set_temp = eeprom_rb(EEPROM_SETTEMP);
	if (set_temp > 40 || set_temp < 1)
		set_temp = 20;

	hysteresis = eeprom_rb(EEPROM_HYSTERESIS);
	if (hysteresis > 5)
		hysteresis = 1;

	dutyvalue = eeprom_rb(EEPROM_DUTYVALUE);
	if (dutyvalue > 10 || dutyvalue < 1)
		dutyvalue = 3;


	// Setup DS1621

	// Access Config Command 
	// Set Continuous transfer, polarity active lo
	i2c_send(0x90, 0xAC, 1, "\0x00");
	delay(1000);

	// Start conversion
	i2c_send(0x90, 0xEE, 0, 0);


	// start the timer
	start_timer_0();



	while (1)
	{
		// check switch status
		
		if (temp >= set_temp+hysteresis)
		{
			relay_on = 0;
			cbi(PORTD, RELAY);				// relay OFF
		}
			
		if (temp <= set_temp-hysteresis)
		{
			relay_on = 1;
			sbi(PORTD, RELAY);				// relay ON
		}	
			
			
		// check keys			
			
		switch (getkey())			
		{
			case KEY_UP :
				if (set_flag)		// don't change set_temp on first press
				{
					switch (mode)
					{
						case NORMAL :
							set_temp++;		// increment set temperature
							break;
						case SET_HYST :							
							hysteresis++;	// increment hysteresis
							break;
						case SET_DUTY :
							dutyvalue++;	// increment duty value
							break;
					}
				}	
				set_flag = 50;		// set timeout flag for display
				break;

			case KEY_DOWN :
				if (set_flag)		// don't change set_temp on first press
				{
					switch (mode)
					{
						case NORMAL :
							set_temp--;		// decrement set temperature
							break;
						case SET_HYST :							
							hysteresis--;	// decrement hysteresis
							break;
						case SET_DUTY :
							dutyvalue--;	// decrement duty value
							break;
					}

				}	
				set_flag = 50;		// set timeout flag for display
				break;
				
			case KEY_BOTH :	
				mode = next_mode[mode];
				set_flag = 50;
				break;
			
		}



		delay(10000);
			
		if (set_flag)
		{
			set_flag--;
			if (set_flag == 0)
			{
				switch (mode)
				{
					case NORMAL:
						eeprom_wb(EEPROM_SETTEMP,set_temp);
						break;
					case SET_HYST :
						eeprom_wb(EEPROM_HYSTERESIS,hysteresis);
						break;
					case SET_DUTY :
						eeprom_wb(EEPROM_DUTYVALUE,dutyvalue);
						break;
				}						
					
				mode = NORMAL;
			}
			temp = set_temp;
		}
		else
		{
			// read temp
			i2c_receive(0x90,0xAA, 2, i2cdata);
			temp = *i2cdata;
		}
	
	
	}




}



