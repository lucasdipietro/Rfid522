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
#include <http.c>
#define TIMER_LEN 128
uint8_t SelfTestBuffer[64];
uint8_t timerStarted = 0;
uint8_t prenderled =0;

#define DOMAIN				"api.beebotte.com"
#define PORT				"80"


#define SSID				"Fibertel WiFi056 2.4GHz"
#define PASSWORD			"0161833386"
//#define SSID				"Barcala"
//#define PASSWORD			"barcala2023"



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
	char strchr[128];
	char strc;
	char * strcp=&strc;
	struct ds3231_clock_t clock = { 0 };
	
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

	while(!(ESPXX_Begin())); //ESPXX_BEGIN ES SOLO PARA ENVIAR UN COMANDO at Y COMPROBAR CONEXIÓN CON EL ESP
	ESPXX_WIFIMode(3);/* 3 = Both (AP and STA) */
	bool booleano = ESPXX_ConnectionMode(0);			/* 0 = Single; 1 = Multi */
	//if(booleano){
	//PORTB |= (1 << PORTB5);
	//}
	//ESPXX_ApplicationMode(NORMAL);		/* 0 = modo Normal; 1 = Transperant Mode */
	ESPXX_JoinAccessPoint(SSID, PASSWORD);
	//ESPXX_Start(0, DOMAIN, PORT);





	while(1){
		

		if(rfid_alive()){
			
			byte = mfrc522_request(PICC_REQALL,str);

			if(byte == CARD_FOUND)
			{	
				
				byte = mfrc522_get_card_serial(str);
				USART_putstring("CARD DETECTED\r\n");
				sprintf(strchr,"Card : %x %x %x %x \r\n",str[0],str[1],str[2],str[3]);
				USART_putstring(strchr);
				
	
				ESPXX_Start(0, DOMAIN, PORT);
				memset(_buffer, 0, 150);
				sprintf(_buffer, "GET /v1/data/read/test/res?limit=1 HTTP/1.1\r\nAccept: application/json\r\nContent-Type: application/json\r\nHost: api.beebotte.com\r\nX-Auth-Token: token_AoCqH6gCmwvFAnls\r\n\r\n");
				ESPXX_Send(_buffer);
				Read_Data(_buffer);
				_delay_ms(600);
				
				if(soniguales){
					
					PORTD |= (1 << 7);
					prenderled = 1;
					TCNT1 = (65535 - (16000000/1024)*50);
					
					byte = ds3231_read_clock ( &clock ); // 0 = success
					sprintf(strchr,"Clock = %d/%d/%d %d:%d:%d \r\n",clock.date,clock.month,clock.year,clock.hours,clock.minutes,clock.seconds);
					USART_putstring(strchr);
					byte = ds3231_read_clock ( &clock ); // 0 = success
					sprintf(strchr,"%d/%d/%d %d:%d:%d",clock.date,clock.month,clock.year,clock.hours,clock.minutes,clock.seconds);
					memset(_buffer, 0, 350);
					sprintf(_buffer, "POST /v1/data/write/test/res HTTP/1.1\r\nAccept: application/json\r\nContent-Type: application/json\r\nHost: api.beebotte.com\r\nX-Auth-Token: token_AoCqH6gCmwvFAnls\r\nContent-length: 12\r\n\r\n{\"data\": %s}",strchr);
					ESPXX_Send(_buffer);
					_delay_ms(10000);
					
				}
				
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
		
		//_delay_ms(2000);
		}
}
	

