#ifndef UART_STDIO_H
#define UART_STDIO_H

#include <Arduino.h>
#include <stdio.h>
#include <avr/pgmspace.h>

namespace UartStdio
{
    void init(unsigned long baud = 9600);

    void print(const __FlashStringHelper *msg);
    void print(const char *msg);
    void print(uint16_t val);
    void print(uint32_t val);
    void print(char c);

    void println(const __FlashStringHelper *msg);
    void println(const char *msg);
    void println(uint16_t val);
    void println();

    // Formatted output (flash format string)
    void printf_P(PGM_P fmt, ...);
}

#endif
