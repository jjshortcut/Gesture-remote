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

// 	a_on_1	= 0
//	a_off_1	= 1
//	b_on_1 	= 2
//	b_off_1	= 3
//	c_on_1	= 4
//	c_off_1	= 5
//	d_on_1	= 6
//	d_off_1	= 7
//	e_on_1	= 8
//	e_off_1	= 9

#define a_on_1	0	// defines to send to send_klikaan_klikuit
#define a_off_1	1
#define b_on_1	2
#define b_off_1	3
#define c_on_1	4
#define c_off_1	5
#define d_on_1	6
#define d_off_1	7
#define e_on_1	8
#define e_off_1	9

void send_klikaan_klikuit(unsigned int);
void init_klikaan_klikuit(void);
		
