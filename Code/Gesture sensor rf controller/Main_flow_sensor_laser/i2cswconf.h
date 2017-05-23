/*! \file i2cswconf.h \brief Software-driven I2C interface configuration. */
//*****************************************************************************
//
// File Name	: 'i2cswconf.h'
// Title		: software-driven I2C interface using port pins
// Author		: Pascal Stang - Copyright (C) 2000-2002
// Created		: 11/22/2000
// Revised		: 5/2/2002
// Version		: 1.1
// Target MCU	: Atmel AVR series
// Editor Tabs	: 4
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#ifndef I2CSWCONF_H
#define I2CSWCONF_H

// clock line port
#define SCLPORT	PORTB	// i2c clock port
#define SCLDDR	DDRB	// i2c clock port direction
// data line port
#define SDAPORT	PORTB	// i2c data port
#define SDADDR	DDRB	// i2c data port direction
#define SDAPIN	PINB	// i2c data port input
// pin assignments
#define SCL		PB3		// i2c clock pin
#define SDA		PB4		// i2c data pin

#endif
