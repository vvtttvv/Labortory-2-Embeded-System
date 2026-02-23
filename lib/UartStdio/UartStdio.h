#ifndef UART_STDIO_H
#define UART_STDIO_H

#include <Arduino.h>
#include <stdio.h>

namespace UartStdio
{
    void init(unsigned long baud = 9600);
}

#endif
