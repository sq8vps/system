#ifndef PORT_H_INCLUDED
#define PORT_H_INCLUDED

#include <stdint.h>

//odczytuje bajt z portu
uint8_t port_readByte(uint16_t port);


//wpisuje bajt na port
void port_writeByte(uint16_t port, uint8_t d);


//odczytuje slowo z portu
uint16_t port_readWord(uint16_t port);


//wpisuje slowo na port
void port_writeWord(uint16_t port, uint16_t d);

uint32_t port_readDword(uint16_t port);

void port_writeDword(uint16_t port, uint32_t d);

#endif // PORT_H_INCLUDED
