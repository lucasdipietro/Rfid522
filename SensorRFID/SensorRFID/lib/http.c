/*
 * CFile1.c
 *
 * Created: 3/6/2023 18:27:05
 *  Author: lucas
 */ 

#include <http.h>

#define SREG    _SFR_IO8(0x3F)

//#define DEFAULT_BUFFER_SIZE		160
#define DEFAULT_TIMEOUT			10000

/* Connection Mode */
#define SINGLE					0
#define MULTIPLE				1

/* Application Mode */
#define NORMAL					0
#define TRANSPERANT				1

/* Application Mode */
#define STATION							1
#define ACCESSPOINT						2
#define BOTH_STATION_AND_ACCESPOINT		3

/* Seleccione si quiere enviar o recibir parámetros por http */
//#define RECEIVE_DEMO				/* Define RECEIVE demo */
#define SEND_DEMO					/* Define SEND demo */

/* defina los campos del servidor thinspeak */
#define DOMAIN				"api.beebotte.com"
#define PORT				"80"
//#define API_WRITE_KEY		"SQ4AV8JCUDOPNG8N"
//#define CHANNEL_ID			"1374723"

//#define SSID				"Fibertel WiFi056 2.4GHz"
//#define PASSWORD			"0161833386"
#define SSID				"Barcala"
#define PASSWORD			"barcala2023"

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

enum ESP8266_RESPONSE_STATUS{
	ESP8266_RESPONSE_WAITING,
	ESP8266_RESPONSE_FINISHED,
	ESP8266_RESPONSE_TIMEOUT,
	ESP8266_RESPONSE_BUFFER_FULL,
	ESP8266_RESPONSE_STARTING,
	ESP8266_RESPONSE_ERROR
};

enum ESP8266_CONNECT_STATUS {
	ESP8266_CONNECTED_TO_AP,
	ESP8266_CREATED_TRANSMISSION,
	ESP8266_TRANSMISSION_DISCONNECTED,
	ESP8266_NOT_CONNECTED_TO_AP,
	ESP8266_CONNECT_UNKNOWN_ERROR
};

enum ESP8266_JOINAP_STATUS {
	ESP8266_WIFI_CONNECTED,
	ESP8266_CONNECTION_TIMEOUT,
	ESP8266_WRONG_PASSWORD,
	ESP8266_NOT_FOUND_TARGET_AP,
	ESP8266_CONNECTION_FAILED,
	ESP8266_JOIN_UNKNOWN_ERROR
};

int8_t Response_Status;
volatile int16_t Counter = 0, pointer = 0;
uint32_t TimeOut = 0;
//char RESPONSE_BUFFER[DEFAULT_BUFFER_SIZE];

void Read_Response(char* _Expected_Response)
{
	uint8_t EXPECTED_RESPONSE_LENGTH = strlen(_Expected_Response);
	uint32_t TimeCount = 0, ResponseBufferLength;
	char RECEIVED_CRLF_BUF[EXPECTED_RESPONSE_LENGTH];
	
	while(1)
	{
		if(TimeCount >= (DEFAULT_TIMEOUT+TimeOut))
		{
			TimeOut = 0;
			Response_Status = ESP8266_RESPONSE_TIMEOUT;
			return;
		}

		if(Response_Status == ESP8266_RESPONSE_STARTING)
		{
			Response_Status = ESP8266_RESPONSE_WAITING;
		}
		ResponseBufferLength = strlen(RESPONSE_BUFFER);
		if (ResponseBufferLength)
		{
			_delay_ms(1);
			TimeCount++;
			if (ResponseBufferLength==strlen(RESPONSE_BUFFER))
			{
				for (uint16_t i=0;i<ResponseBufferLength;i++)
				{
					memmove(RECEIVED_CRLF_BUF, RECEIVED_CRLF_BUF + 1, EXPECTED_RESPONSE_LENGTH-1);
					RECEIVED_CRLF_BUF[EXPECTED_RESPONSE_LENGTH-1] = RESPONSE_BUFFER[i];
					if(!strncmp(RECEIVED_CRLF_BUF, _Expected_Response, EXPECTED_RESPONSE_LENGTH))
					{
						TimeOut = 0;
						Response_Status = ESP8266_RESPONSE_FINISHED;
						return;
					}
				}
			}
		}
		_delay_ms(1);
		TimeCount++;
	}
}

void ESPXX_Clear()
{
	memset(RESPONSE_BUFFER,0,DEFAULT_BUFFER_SIZE);
	Counter = 0;	pointer = 0;
}

void Start_Read_Response(char* _ExpectedResponse)
{
	Response_Status = ESP8266_RESPONSE_STARTING;
	do {
		Read_Response(_ExpectedResponse);
	} while(Response_Status == ESP8266_RESPONSE_WAITING);

}

void GetResponseBody(char* Response, uint16_t ResponseLength)
{

	uint16_t i = 12;
	char buffer[5];
	while(Response[i] != '\r')
	++i;

	strncpy(buffer, Response + 12, (i - 12));
	ResponseLength = atoi(buffer);

	i += 2;
	uint16_t tmp = strlen(Response) - i;
	memcpy(Response, Response + i, tmp);

	if(!strncmp(Response + tmp - 6, "\r\nOK\r\n", 6))
	memset(Response + tmp - 6, 0, i + 6);
}

bool WaitForExpectedResponse(char* ExpectedResponse)
{
	Start_Read_Response(ExpectedResponse);	/* primero se lee la respuesta */
	if((Response_Status != ESP8266_RESPONSE_TIMEOUT))
	return true;							/* retorna true si la respuesta es la esperada */
	return false;							/* en caso contrario devuelve false */
}

bool SendATandExpectResponse(char* ATCommand, char* ExpectedResponse)
{
	ESPXX_Clear();
	USART_putstring(ATCommand);			/* envia el comando AT para el ESPXX */
	USART_putstring("\r\n");
	return WaitForExpectedResponse(ExpectedResponse);
}

bool ESPXX_ApplicationMode(uint8_t Mode)
{
	char _atCommand[20];
	memset(_atCommand, 0, 20);
	sprintf(_atCommand, "AT+CIPMODE=%d", Mode);
	_atCommand[19] = 0;
	return SendATandExpectResponse(_atCommand, "\r\nOK\r\n");
}

bool ESPXX_ConnectionMode(uint8_t Mode)
{
	char _atCommand[20];
	memset(_atCommand, 0, 20);
	sprintf(_atCommand, "AT+CIPMUX=%d", Mode);
	_atCommand[19] = 0;
	return SendATandExpectResponse(_atCommand, "\r\nOK\r\n");
}

bool ESPXX_Begin()
{
	for (uint8_t i=0;i<1;i++)
	{
		if(SendATandExpectResponse("ATE0","\r\nOK\r\n")||SendATandExpectResponse("AT","\r\nOK\r\n"))
		return true;  //devuelve verdadero si al enviar AT se recibe OK
	}
	return true;
}

bool ESPXX_Close()
{
	return SendATandExpectResponse("AT+CIPCLOSE=1", "\r\nOK\r\n");
}

bool ESPXX_WIFIMode(uint8_t _mode)
{
	char _atCommand[20];
	memset(_atCommand, 0, 20);
	sprintf(_atCommand, "AT+CWMODE=%d", _mode);
	_atCommand[19] = 0;
	return SendATandExpectResponse(_atCommand, "\r\nOK\r\n");
}

uint8_t ESPXX_JoinAccessPoint(char* _SSID, char* _PASSWORD)
{
	char _atCommand[60];
	memset(_atCommand, 0, 60);
	sprintf(_atCommand, "AT+CWJAP=\"%s\",\"%s\"", _SSID, _PASSWORD);
	_atCommand[59] = 0;
	if(SendATandExpectResponse(_atCommand, "\r\nWIFI CONNECTED\r\n"))
	return ESP8266_WIFI_CONNECTED;
	else{
		if(strstr(RESPONSE_BUFFER, "+CWJAP:1"))
		return ESP8266_CONNECTION_TIMEOUT;
		else if(strstr(RESPONSE_BUFFER, "+CWJAP:2"))
		return ESP8266_WRONG_PASSWORD;
		else if(strstr(RESPONSE_BUFFER, "+CWJAP:3"))
		return ESP8266_NOT_FOUND_TARGET_AP;
		else if(strstr(RESPONSE_BUFFER, "+CWJAP:4"))
		return ESP8266_CONNECTION_FAILED;
		else
		return ESP8266_JOIN_UNKNOWN_ERROR;
	}
}

uint8_t ESPXX_connected()
{
	SendATandExpectResponse("AT+CIPSTATUS", "\r\nOK\r\n");
	if(strstr(RESPONSE_BUFFER, "STATUS:2"))
	return ESP8266_CONNECTED_TO_AP;
	else if(strstr(RESPONSE_BUFFER, "STATUS:3"))
	return ESP8266_CREATED_TRANSMISSION;
	else if(strstr(RESPONSE_BUFFER, "STATUS:4"))
	return ESP8266_TRANSMISSION_DISCONNECTED;
	else if(strstr(RESPONSE_BUFFER, "STATUS:5"))
	return ESP8266_NOT_CONNECTED_TO_AP;
	else
	return ESP8266_CONNECT_UNKNOWN_ERROR;
}

uint8_t ESPXX_Start(uint8_t _ConnectionNumber, char* Domain, char* Port)
{
	bool _startResponse;
	char _atCommand[60];
	memset(_atCommand, 0, 60);
	_atCommand[59] = 0;

	//comando de conexión al servidor de thinkspeak
	sprintf(_atCommand, "AT+CIPSTART=\"TCP\",\"%s\",%s", Domain, Port);
	//se envía el comando de conexión
	_startResponse = SendATandExpectResponse(_atCommand, "CONNECT\r\n");
	if(!_startResponse) // se verifica si la respuesta es correcta
	{
		if(Response_Status == ESP8266_RESPONSE_TIMEOUT)
		return ESP8266_RESPONSE_TIMEOUT;
		return ESP8266_RESPONSE_ERROR;
	}
	return ESP8266_RESPONSE_FINISHED;
}

uint8_t ESPXX_Send(char* Data)
{
	char _atCommand[20];
	memset(_atCommand, 0, 20);
	sprintf(_atCommand, "AT+CIPSEND=%d", (strlen(Data)+2));
	_atCommand[19] = 0;
	SendATandExpectResponse(_atCommand, "\r\nOK\r\n>");
	if(!SendATandExpectResponse(Data, "\r\nSEND OK\r\n"))
	{
		if(Response_Status == ESP8266_RESPONSE_TIMEOUT)
		return ESP8266_RESPONSE_TIMEOUT;
		return ESP8266_RESPONSE_ERROR;
	}
	return ESP8266_RESPONSE_FINISHED;
}

int16_t ESPXX_DataAvailable()
{
	return (Counter - pointer);
}

uint8_t ESPXX_DataRead()
{
	if(pointer < Counter)
	return RESPONSE_BUFFER[pointer++];
	else{
		ESPXX_Clear();
		return 0;
	}
}

uint16_t Read_Data(char* _buffer)
{
	uint16_t len = 0;
	_delay_ms(100);
	while(ESPXX_DataAvailable() > 0)
	_buffer[len++] = ESPXX_DataRead();
	return len;
}

ISR (USART_RX_vect)
{
	uint8_t oldsrg = SREG;
	cli();
	RESPONSE_BUFFER[Counter] = UDR0;
	Counter++;
	if(Counter == DEFAULT_BUFFER_SIZE){
		Counter = 0; pointer = 0;
	}
	SREG = oldsrg;
}
void extract_data(const char* packet, char** extracted_data) {
	const char* start = strstr(packet, "data") + strlen("data") + 2;
	const char* end = strstr(packet, "wts") - 2;

	int len = end - start;

	*extracted_data = malloc(len + 1);
	strncpy(*extracted_data, start, len);
	(*extracted_data)[len] = '\0';
}
void Post_f(char * channel, char * resource, char * token, char * data) {
	ESPXX_Start(0, DOMAIN, PORT);
	char paquete[350]="POST /v1/data/write/";
	char longi[20]="";
	strcat(paquete,channel);
	strcat(paquete,"/");
	strcat(paquete,resource);
	strcat(paquete," HTTP/1.1\r\nAccept: application/json\r\nContent-Type: application/json\r\nHost: api.beebotte.com\r\nX-Auth-Token: ");
	strcat(paquete,token);
	strcat(paquete,"\r\nContent-length: ");
	sprintf(longi,"%ld",strlen(data));
	strcat(paquete,longi);
	strcat(paquete,"\r\n\r\n{\"data\": ");
		strcat(paquete,data);
	strcat(paquete,"}");
	ESPXX_Send(paquete);
	return;
}
void Get_f(char * channel, char * resource, char * token){
	ESPXX_Start(0, DOMAIN, PORT);
	char paquete[350]="GET /v1/data/read/";
	strcat(paquete,channel);
	strcat(paquete,"/");
	strcat(paquete,resource);
	strcat(paquete,"?limit=1 HTTP/1.1\r\nAccept: application/json\r\nContent-Type: application/json\r\nHost: api.beebotte.com\r\nX-Auth-Token: ");
	strcat(paquete,token);
	strcat(paquete,"\r\n\r\n");
	ESPXX_Send(paquete);
	return;
}
/*
ds3231_to_str(ds3231_clock_t clock, char * clock_string){
	sprintf(clock_string, "%d/%d/%d %d:%d:%d", clock.day, clock.month, clock.year, clock.hours, clock.minutes, clock.seconds);
	return;
}

ds3231_clock_t time_diff(ds3231_clock_t clock1, char * clock2){
	struct ds3231_clock_t duration;
	uint32_t time1 = clock1.hours * 3600 + clock1.minutes * 60 + clock1.seconds;
	uint32_t clock2_dia, clock2_mes, clock2_anio, clock2_hora, clock2_minuto, clock2_segundo;
	sscanf(clock2, "%d/%d/%d %d:%d:%d", &clock2_dia, &clock2_mes, &clock2_anio, &clock2_hora, &clock2_minuto, &clock2_segundo);
	uint32_t time2 = clock2_hora * 3600 + clock2_minuto * 60 + clock2_segundo;
	int32_t diff = time2 - time1;
	uint8_t hours = diff / 3600;
	diff -= hours * 3600;
	uint8_t minutes = diff / 60;
	uint8_t seconds = diff % 60;
	duration.hours = hours;
	duration.minutes = minutes;
	duration.seconds = seconds;
	return duration;
}
*/