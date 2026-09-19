#ifndef UART_H
#define UART_H

void UART0_Init(void);
void UART0_WriteChar(char c);
void UART0_WriteString(const char *string);
void UART0_WriteFloat(float number);

#endif