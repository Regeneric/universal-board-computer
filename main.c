#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "includes/rtos/FreeRTOS.h"
#include "includes/rtos/task.h"


#define ON 	1
#define OFF 0


#define UART_BAUD 9600
#define __UBRR ((F_CPU+UART_BAUD*8UL) / (16UL*UART_BAUD)-1)

void initUSART(uint16_t ubrr);
void sendUSART(uint8_t *data);
char recvUSART();


struct LED {
	int state;	// If LED is ON or OFF
	int delay;	// How long it stays ON (ms)
}; inline void initLED(struct LED *l, int ls, int d) {l->state = ls; l->delay = d;}

void debugLED(void *params);


void sendTask(void *params);
void recvTask(void *params);

static TaskHandle_t ledControl  = NULL;
static TaskHandle_t usartRecv 	= NULL;
static TaskHandle_t usartSend 	= NULL;

int main() {
	struct LED ledConfig; 
	initLED(&ledConfig, ON, 500);

	initUSART(__UBRR);

	xTaskCreate(debugLED,
		(const char*)"Toogle LED",
		256,
		(void*)&ledConfig,
		1,
		NULL
	);

	xTaskCreate(recvTask,
		(const char*)"Recieve data from terminal",
		512,
		(void*)&ledConfig,
		5,
		NULL
	);

	// xTaskCreate(sendTask,
	// 	(const char*)"USART data transmit",
	// 	512,
	// 	(void*)&ledConfig,
	// 	5,
	// 	&usartSend
	// );

	
	vTaskStartScheduler();
	return 0;
}


void sendTask(void *params) {
	struct LED *led = params;

	char *data = malloc(32 * sizeof *data);
	itoa(led->delay, data, 10);

	while(1) sendUSART(data);
	free((void*)data);
}
void recvTask(void *params) {
	struct LED *led = params;
	
	char *data = malloc(128 * sizeof *data);
	int delayTime = 0;

	while(1) {
		data = recvUSART();

		sscanf(data, "%d", &delayTime);
		led->delay = delayTime;

		vTaskDelay(100 / portTICK_PERIOD_MS);
	}

	free((void*)data);
}


void debugLED(void *params) {
	struct LED *led = params;

	DDRB = led->state ? 0x20 : 0x00;	// Built-in LED (PB5)
	if(led->delay > 0) {
		while(1) {
			PORTB ^= 0x20;
			vTaskDelay(led->delay / portTICK_PERIOD_MS);
		}
	} else PORTB |= 0x20;
}












void initUSART(uint16_t ubrr) {
	UBRR0H = (uint8_t)(ubrr>>8);	// High byte
	UBRR0L = (uint8_t)ubrr;			// Low byte

	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	UCSR0C = (1<<USBS0)|(3<<UCSZ00);	// 8N1
}
void sendUSART(uint8_t *data) {
	for(int i = 0; i <= strlen(data); i++) {
		while(!(UCSR0A & (1<<UDRE0)));
		UDR0 = data[i];
	} return;
}
char recvUSART() {
	DDRB  = 0x20;
	PORTB = 0x20;

	char *data = malloc(128 * sizeof *data);
	memset(data, 0, (128 * sizeof *data));

	for(int i = 0; i < strlen((const char*)UDR0); ++i) {
		while(!(UCSR0A & (1<<UDRE0)));
		data[i] = UDR0;
	}

	PORTB = 0x00;
	free((void*)data);

	return data;
}