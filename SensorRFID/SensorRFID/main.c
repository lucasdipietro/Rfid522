/*
 * main.c
 
 
 */



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
#include <interrupt.h>
#include <stdbool.h>
#include <http.h>
#include <stdlib.h>

#define TIMER_LEN 128
uint8_t SelfTestBuffer[64];
uint8_t timerStarted = 0;
uint8_t prenderled =0;

#define DOMAIN				"api.beebotte.com"
#define PORT				"80"


#define SSID				"Fibertel WiFi056 2.4GHz"
#define PASSWORD			"0141833386"
//#define SSID				"Barcala"
//#define PASSWORD			"barcala2023"
#define channel "Tarjetas"
#define token "token_WlqwfZqzycNiPoZK"


ISR(PCINT2_vect){
	_delay_ms(200);
	if(!(PIND & (1 << 3))){
	 PORTD ^= (1<<7); 
	USART_putstring("Boton apretado\r\n");
	}
	else
	{
		PORTD ^= (1<<7); 
	}

}


void timer_init()
{
	
	TCCR1B = (1<< CS10) | (1<<CS12);
	TCCR1A = 0x00;
	TIMSK1 = (1<<TOIE1);
	
	
}


ISR(TIMER1_OVF_vect)
{
	if(prenderled){
	 PORTD &= ~(1<<7); // apaga pin 7
	 prenderled=0;
	 USART_putstring("Timer activo");
	}
}


int main()
{
	uint8_t str[MAX_LEN];
	uint8_t byte;
	uint8_t FOUND;
	uint8_t ENTER;
	char ID[MAX_LEN];
	char TIME[TIMER_LEN];
	char strchrs[128];
	char cardchrs[128];
	char clockstr[100];
	char strc;
	char * strcp;
	uint8_t card_match = 0;
	struct ds3231_clock_t clock = { 0 };
		struct ds3231_clock_t duration = {0}; 
		char* extracted_data ;
		uint8_t entrando;
		uint8_t saliendo;
	
	spi_init();
		_delay_ms(1000);
	mfrc522_init();
		_delay_ms(500);
	//USART_init(9600);
		//_delay_ms(500);
		
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
	if (rtc_alive())
	{
		USART_putstring("RTC Conectado");
	}
	
	DDRD |= (1<<7); // PD7 como output 
	PORTD &= ~(1<<7); // inicia el pin 7 en 0
	
	DDRD &= ~(1<<3); // PD2 como interrupcion
	PORTD |= (1<<3); // inicia el pin 3 en 1
	
	PCICR |= (1 << PCIE2);    // Habilita la interrupción por cambio de estado en PCINT[23:16]
	PCMSK2 |= (1 << PCINT19); // Habilita la interrupción en el pin PD3 (PCINT19)
	 timer_init();
	sei(); 
	
	///conexion ESP
	char _buffer[350];
	uint8_t Connect_Status;
	///#ifdef SEND_DEMO
	uint8_t Sample = 0;
	///#endif
	///DDRB |= (1 << DDB5);

	USART_init(BAUD_PRESCALLER_115200);						/* Inicia la USART a 115200 baudios */
	//sei();									/* inicia interrupciones globales */

	ESPXX_Begin(); //ESPXX_BEGIN ES SOLO PARA ENVIAR UN COMANDO at Y COMPROBAR CONEXIÓN CON EL ESP
	ESPXX_WIFIMode(3);/* 3 = Both (AP and STA) */
	ESPXX_ConnectionMode(0);			/* 0 = Single; 1 = Multi */
	ESPXX_JoinAccessPoint(SSID, PASSWORD);





	while(1){
		

		if(rfid_alive()){
			
			byte = mfrc522_request(PICC_REQALL,str);

			if(byte == CARD_FOUND)
			{	
				//////////////////////////////////////////////////////////////////////////
				///Aca detecta la tarjeta
				byte = mfrc522_get_card_serial(str);
				USART_putstring("CARD DETECTED\r\n");
				sprintf(cardchrs,"%x:%x:%x:%x",str[0],str[1],str[2],str[3]);
				//USART_putstring(strchr);
				//////////////////////////////////////////////////////////////////////////
				ds3231_read_clock ( &clock ); // 0 = success
				sprintf(clockstr,"%d/%d/%d %d:%d:%d",clock.date,clock.month,clock.year,clock.hours,clock.minutes,clock.seconds);
				
				//////////////////////////////////////////////////////////////////////////
				///Aca hace el GET a la ID 
				 Get_f(channel,strchrs,token);
				//Read_Data(_buffer); //aca hay q ver q hacemo 
				 extracted_data = extract_data(RESPONSE_BUFFER);
				 sprintf(strchrs,"%s",extracted_data);
				 if(!strcmp(strchrs,"true")){
					 card_match = 1;
				 }
				 free(extracted_data);
				//////////////////////////////////////////////////////////////////////////
				
				if(card_match)
					if(!strcmp(strchrs,"aca va la ide de tarjeta")){
						#define tarjeta_entrada "tarjeta_1_entrada"
						#define tarjeta_salida  "tarjeta_1_salida"
						#define tarjeta_diferencia  "tarjeta_1_diferencia"
						#define tarjeta_bool  "tarjeta_1_bool"
					}
					if(!strcmp(strchrs,"aca va la ide de tarjeta")){
						#define tarjeta_entrada  "tarjeta_2_entrada"
						#define tarjeta_salida  "tarjeta_2_salida"
						#define tarjeta_diferencia  "tarjeta_2_diferencia"
						#define tarjeta_bool  "tarjeta_2_bool"
					}
				
					//////////////////////////////////////////////////////////////////////////
					//abre el rele
					PORTD |= (1 << 7);
					prenderled = 1;
					TCNT1 = (65535 - (16000000/1024)*50);
					//////////////////////////////////////////////////////////////////////////
					
					//////////////////////////////////////////////////////////////////////////
					///GET para saber si esta adentro o afuera 
					Get_f(channel,tarjeta_bool,token);
					//uint8_t entrando = strcmp(extract_data(RESPONSE_BUFFER),"true");
					 extracted_data = extract_data(RESPONSE_BUFFER);
					 sprintf(strchrs,"%s",extracted_data);
					 if(!strcmp(strchrs,"true")){
						 entrando = 1;
					 }
					 free(extracted_data);
					//bool saliendo = strcmp(extract_data(RESPONSE_BUFFER),"false");
					 extracted_data = extract_data(RESPONSE_BUFFER);
					 sprintf(strchrs,"%s",extracted_data);
					 if(!strcmp(strchrs,"false")){
						 saliendo = 1;
					 }
					 free(extracted_data);
					//////////////////////////////////////////////////////////////////////////
					
					if(entrando){
					//////////////////////////////////////////////////////////////////////////
					///registra el tiempo y lo envia a tiempo de entrada
					Get_f(channel,tarjeta_entrada,token);
					//_buffer = extract_data(RESPONSE_BUFFER);
					//ACA MOVEMOS EL VECTOR DE TIEMPO
					
					
					extracted_data = extract_data(RESPONSE_BUFFER);
					sprintf(_buffer,"%s",extracted_data);
					memmove(_buffer, _buffer + 17, strlen(_buffer) - 17 + 1);
					sprintf(strchrs,",%s",clockstr);
					strcat(_buffer,strchrs);
					//ACA LO PUSHEAMOS 
					Post_f(channel,tarjeta_entrada,token,_buffer);
					//////////////////////////////////////////////////////////////////////////
					free(extracted_data);
					
					//////////////////////////////////////////////////////////////////////////
					/// cambia el valor del bool 
					Post_f(channel,tarjeta_bool,token,"false");
					}
					//////////////////////////////////////////////////////////////////////////
					if(saliendo){
					//////////////////////////////////////////////////////////////////////////
					///registra el tiempo 
					Get_f(channel,tarjeta_salida,token);
					//_buffer = extract_data(RESPONSE_BUFFER);
					extracted_data = extract_data(RESPONSE_BUFFER);
					sprintf(_buffer,"%s",extracted_data);
					//ACA MOVEMOS EL VECTOR DE TIEMPO
					memmove(_buffer, _buffer + 17, strlen(_buffer) - 17 + 1);
					sprintf(strchrs,",%s",clockstr);
					strcat(_buffer,strchrs);
					//ACA LO PUSHEAMOS
					Post_f(channel,tarjeta_salida,token,_buffer);
					free(extracted_data);
					//////////////////////////////////////////////////////////////////////////
					
					//////////////////////////////////////////////////////////////////////////
					///resta los tiempos
					Get_f(channel,tarjeta_entrada,token);
					extracted_data = extract_data(RESPONSE_BUFFER);
					sprintf(_buffer,"%s",extracted_data);
					memmove(_buffer,_buffer + (17*4),strlen(_buffer) - (17*4)+1);
					
					time_diff(clock,_buffer);	
					sprintf(_buffer,"%d/%d/%d %d:%d:%d",clock.date,clock.month,clock.year,clock.hours,clock.minutes,clock.seconds);
					//////////////////////////////////////////////////////////////////////////
					free(extracted_data);
					//////////////////////////////////////////////////////////////////////////
					///cambia el bool
					Post_f(channel,tarjeta_bool,token,"false");
					
					//////////////////////////////////////////////////////////////////////////			
					}
			
				
				

			}
			_delay_ms(2000);
		}
		else{
		
		_delay_ms(1000);
		} 
		
		//_delay_ms(2000);
		}
}
	

