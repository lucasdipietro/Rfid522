/*
 * main.c
 
 
 */


//#define BAUDRATE 9600
#define F_CPU 16000000UL
#define BAUD_PRESCALE ((F_CPU / (BAUDRATE * 16UL)) - 1)
#include <uart.h> 
#include <avr/io.h>
#include <util/delay.h>
#include <spi.h>
#include <mfrc522.h>
#include <stdio.h>
#include <string.h>
#include <extfun.h>
#include <ds3231.h>
#include <i2c.h>
#include <avr/interrupt.h>
#define TIMER_LEN 128
uint8_t SelfTestBuffer[64];


ISR(INT0_vect){
	PORTD ^= (1<<7); // inicia el pin 7 en 0
	USART_putstring("Boton apretado\r\n");
}


int main()
{
	uint8_t str[MAX_LEN];
	uint8_t byte;
	uint8_t FOUND;
	uint8_t ENTER;
	char ID[MAX_LEN];
	char TIME[TIMER_LEN];
	char strchr[128];
	char strc;
	char * strcp=&strc;
	struct ds3231_clock_t clock = { 0 };
	
	spi_init();
		_delay_ms(1000);
	mfrc522_init();
		_delay_ms(500);
	USART_init(9600);
		_delay_ms(500);
	if(rfid_alive()){
		USART_putstring("Sensor detected\r\n");
		_delay_ms(500);
	rfid_config();
	}
	/*
	clock.minutes = 49;
	clock.hours = 22;
	clock.seconds= 0;
	clock.day= 5;
	clock.month=5;
	clock.year=23;
	clock.century=1;
	clock.date = 25;
	i2c_init();
	byte = ds3231_write_clock ( &clock ); // 0 = success
	sprintf(strchr,"Write = %d\r\n",byte);
	USART_putstring(strchr);
	*/
	i2c_init();
	cli();
	DDRD |= (1<<7); // PD7 pin 7 del arduino UNO como output 
	PORTD &= ~(1<<7); // inicia el pin 7 en 0
	
	DDRD &= ~(1<<2); // PD2 pin 2 del arduino UNO como interrupcion
	PORTD |= (1<<2); // inicia el pin 2 en 1
	
	EICRA = 0b00000011; //interrupcion externa por flanco de bajada en INT0 
	EIMSK = 0b00000001; //habilita interrupciones externas INT0
	EIFR = 0b00000000;
	sei(); 


	while(1){
		
		
		if(rfid_alive()){
			
			byte = mfrc522_request(PICC_REQALL,str);
		
		
			if(byte == CARD_FOUND)
			{	
				
				byte = mfrc522_get_card_serial(str);
				USART_putstring("CARD DETECTED\r\n");
				sprintf(strchr,"Card : %x %x %x %x \r\n",str[0],str[1],str[2],str[3]);
				USART_putstring(strchr);
				//llamar al esp preguntando las IDs 
				//comparar
				//si son iguales mandas horario de apertura 
				//abrir el rele y subir el clock 
				
				//ID
				//adentro ?? 
				//horario de entrada
				//horario de salida 
				//string = { " "," "}  
				
				
				byte = ds3231_read_clock ( &clock ); // 0 = success
				sprintf(strchr,"Clock = %d/%d/%d %d:%d:%d \r\n",clock.date,clock.month,clock.year,clock.hours,clock.minutes,clock.seconds);
				USART_putstring(strchr);
			}
			_delay_ms(2000);
		}
		else{
		
		_delay_ms(1000);
		} 
		
		
		}
}
	

