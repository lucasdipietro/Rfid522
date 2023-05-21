#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

#define BAUDRATE 9600
#define BAUD_PRESCALLER_9600 (((F_CPU / (BAUDRATE * 16UL))) - 1)
#define  BAUD_PRESCALLER_19200   (((F_CPU / (BAUDRATE * 16U))) - 1)  // 19.2k bps
#define  BAUD_PRESCALLER_38400  25  // 38.4k bps
#define  BAUD_PRESCALLER_57600   16  // 57.6k bps
#define  BAUD_PRESCALLER_115200    8  // 115.2k bps

//Declaración de funciones
void USART_init(unsigned char baudios);
unsigned char USART_receive(void);
void USART_send( unsigned char data);
void USART_putstring(char* StringPtr);
