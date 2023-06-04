#include "uart.h"
#include <avr/io.h>
#include <util/delay.h>


void USART_init(unsigned char baudios){
uint16_t ubrr = baudios;
UBRR0H = (uint8_t)(ubrr>>8);
UBRR0L = (uint8_t)(ubrr);
UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1 << RXCIE0);
UCSR0C = ((1<<UCSZ00)|(1<<UCSZ01));
DDRD |= (1<<1);                            //PD1    COMO SALIDA TXa
DDRD &= ~(1<<0);                        //PD0    COMO ENTRADA RX

}

void USART_send( unsigned char data){

	while(!(UCSR0A & (1<<UDRE0)));
	UDR0 = data;

}

unsigned char USART_receive(void){

	while(!(UCSR0A & (1<<RXC0)));
	return UDR0;

}

void USART_putstring(char* StringPtr){

	while(*StringPtr != 0x00){    //Aca se chequea si no hay mas caracteres para enviar, esto //se hace chequeando el caracter actual y viendo si es diferente de NULL
		USART_send(*StringPtr);    //usando esta simple funcion envio un caracter por vez
	StringPtr++;}        //incremento el puntero asi puedo leer el proximo caracter
}
