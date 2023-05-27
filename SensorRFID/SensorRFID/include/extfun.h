/*
 * IncFile1.h
 *
 * Created: 24/5/2023 18:58:44
 *  Author: lucas
 */ 


#ifndef extfun_H_
#define extfun_H_
#define ERROR10 10
#define ERROR20 20
uint8_t rfid_card_found(char);
void rfid_config();
uint8_t rfid_alive();
void errorhandler(uint8_t errorcode);

#endif /* INCFILE1_H_ */