/*
    bspSpi.c

    Board support for controlling UART interfaces on NUCLEO-F401RE MCU

    Developed for University of Washington embedded systems programming certificate
    
    2016/2 Nick Strathy wrote/arranged it
*/

#include "bsp.h"

/**
  * @brief  Print a character on the HyperTerminal
  * @param  c: The character to be printed
  * @retval None
  */
void PrintByte(char c)
{
  USART_SendData(COMM, c);
  while (USART_GetFlagStatus(COMM, USART_FLAG_TXE) == RESET);
}