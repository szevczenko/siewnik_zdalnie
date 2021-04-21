/*
 * eeprom.c
 *
 * Created: 13.03.2019 15:51:37
 *  Author: Demetriusz
 */ 

#include "eeprom.h"
#include <avr/io.h>

void EEPROM_write(unsigned int ucAddress, unsigned char ucData)
{
	
	//cli();
	/* Wait for completion of previous write */
	
	while(EECR & (1<<EEWE));
	
	/* Set up address and data registers */
	EEAR = ucAddress;
	EEDR = ucData;
	/* Write logical one to EEMPE */
	EECR |= (1<<EEMWE);
	/* Start eeprom write by setting EEPE */
	EECR |= (1<<EEWE);
	//sei();
	
}

unsigned char EEPROM_read(unsigned int ucAddress)
{
	/* Wait for completion of previous write */
	while(EECR & (1<<EEWE));
	/* Set up address register */
	EEAR = ucAddress;
	/* Start eeprom read by writing EERE */
	EECR |= (1<<EERE);
	/* Return data from data register */
	return EEDR;
}