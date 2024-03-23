/*
 * UART.h
 *
 *  Created on: Mar 11, 2024
 *      Author: minseok
 */

#ifndef SRC_UART_H_
#define SRC_UART_H_

#include "main.h"

#define STX		0x02
#define ETX		0x03

typedef struct {
	uint8_t command;
	uint16_t data;
}protocol_t;

void initUart(UART_HandleTypeDef *inHuart);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
int16_t getChar();
int _write(int file, char *p, int len);

void transmitPacket(protocol_t data);
protocol_t receivePacket();


#endif /* SRC_UART_H_ */
