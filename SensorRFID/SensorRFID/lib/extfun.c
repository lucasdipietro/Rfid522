/*
 * CFile1.c
 *
 * Created: 24/5/2023 18:53:20
 *  Author: lucas
 */ 
#include <mfrc522.h>
#include <spi.h>
#include <util/delay.h>
#include <extfun.h>
uint8_t byte;



uint8_t rfid_alive(){
	
	byte = mfrc522_read(VersionReg);
	if(byte == 0x92 || byte == 0x91 || byte==0x90){
		
		return 1;
	}
	else{
		errorhandler(ERROR10);

		return 0;
	}
	
}


void rfid_config(){
	byte = mfrc522_read(ComIEnReg);
	mfrc522_write(ComIEnReg,byte|0x20);
	byte = mfrc522_read(DivIEnReg);
	mfrc522_write(DivIEnReg,byte|0x80);
	};
	
uint8_t  rfid_card_found(char str){

	byte = mfrc522_request(PICC_REQALL,str);
	if(byte == CARD_FOUND)
	{
		return CARD_FOUND;
	}
	else if(byte == ERROR){
	//	errorhandler(ERROR20);
		return 0;
	}
	else{
		return 0;
	}
	
}

void errorhandler(uint8_t errorcode){
	if(errorcode == ERROR10){
		USART_putstring("No sensor detected\r\n");
	}
	if(errorcode == ERROR20){
		USART_putstring("Error reading card");
	}
	
}
