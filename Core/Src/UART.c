/*
 * UART.c
 *
 *  Created on: Mar 11, 2024
 *      Author: minseok
 */

#include "UART.h"
#include <stdio.h>

UART_HandleTypeDef *myHuart;

#define rxBufferMax 255

int rxBufferGp;			// get pointer (read)
int rxBufferPp;			// put pointer (write)
uint8_t rxBuffer[rxBufferMax];
uint8_t rxChar;

uint8_t i = 0;

// Device init
void initUart(UART_HandleTypeDef *inHuart)		// Interrupt setting
{
	myHuart = inHuart;
	HAL_UART_Receive_IT(myHuart, &rxChar, 1);
}

// process received character
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	rxBuffer[rxBufferPp++] = rxChar;
	rxBufferPp %= rxBufferMax;
	HAL_UART_Receive_IT(myHuart, &rxChar, 1);
}

// get character form buffer
int16_t getChar()
{
	int16_t result;
	if(rxBufferGp == rxBufferPp) return -1;
	result = rxBuffer[rxBufferGp++];
	rxBufferGp %= rxBufferMax;
	return result;
}

int _write(int file, char *p, int len)
{
	HAL_UART_Transmit(myHuart, p, len, 10);
	return len;
}

// packet 송신
void transmitPacket(protocol_t data)
{
	/*
	 * 사전준비
	 * CRC 계산
	 * 데이터 전송
	 * 데이터 전송 완료 대기
	 */
	// 사전준비
	uint8_t txBuffer[] = {STX, 0, 0, 0, 0, ETX};
	txBuffer[1] = data.command;
	txBuffer[2] = (data.data >> 7) | 0x80; // big endian
	txBuffer[3] = (data.data & 0x7f) | 0x80;
	// CRC 계산
	txBuffer[4] = txBuffer[0] + txBuffer[1] + txBuffer[2] + txBuffer[3];
	// 데이터 전송
	HAL_UART_Transmit(myHuart, txBuffer, sizeof(txBuffer), 1);
	// 데이터 전송 완료 대기
	while(HAL_UART_GetState(myHuart) == HAL_UART_STATE_BUSY_TX ||
			HAL_UART_GetState(myHuart) == HAL_UART_STATE_BUSY_TX_RX);
	// 데이터 전송 중이면 HAL_UART_STATE_BUSY_TX 상태가 반환 됨
	// 상태가 HAL_UART_STATE_BUSY_TX 끝날 때까지 while문 반복
}

// packet 수신
protocol_t receivePacket()
{
	protocol_t result;
	uint8_t buffer[6];
	uint8_t count = 0;
	uint32_t timeout;

	int16_t ch = getChar();
	memset(&result, 0, sizeof(buffer)); // result 구조체를 0으로 초기화
	if(ch == STX)
	{
		buffer[count++] = ch;
		// STX 수신됐을 때 시간 저장
		timeout = HAL_GetTick();		// System Clock timeTick(32bit)
		while(ch != ETX)
		{
			ch = getChar();
			if(ch != -1)
				buffer[count++] = ch;
			//타임 아웃 처리(노이즈로 STX는 들어왔는데 ETX가 안들어왔을 때)
			if(HAL_GetTick() - timeout >= 2) return result;
			// 2ms 넘으면 result(0 저장되어 있음) 반환
		}

		// CRC 검사(ETX 정상적으로 수신 됐을 때)
		uint8_t crc = 0;
		for(int i = 0; i<4; i++)
			crc += buffer[i];
		if(crc != buffer[4]) return result;

		// 수신완료 후 데이터 파싱(parsing)
		result.command = buffer[1];
		result.data = buffer[3] & 0x7f;
		result.data |= (buffer[2] & 0x7f) << 7;
	}
	return result;
}
