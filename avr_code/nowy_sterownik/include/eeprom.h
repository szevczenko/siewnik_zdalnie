/*
 * eeprom.h
 *
 * Created: 13.03.2019 15:51:23
 *  Author: Demetriusz
 */ 


#ifndef EEPROM_H_
#define EEPROM_H_

void EEPROM_write(unsigned int ucAddress, unsigned char ucData);
unsigned char EEPROM_read(unsigned int ucAddress);




#endif /* EEPROM_H_ */