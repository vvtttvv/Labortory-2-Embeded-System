#include "UartStdio.h"
#include <stdarg.h>

static FILE uart_out;
static FILE uart_in;

static int uart_putchar(char c, FILE *)
{
    if (c == '\n')
        Serial.write('\r');
    Serial.write(c);
    return 0;
}

static int uart_getchar(FILE *)
{
    while (!Serial.available())
        ;
    return Serial.read();
}

void UartStdio::init(unsigned long baud)
{
    Serial.begin(baud);

    // Brief startup "delay" for UART stabilization
    for (volatile uint16_t i = 0; i < 30000; i++) { ; }

    fdev_setup_stream(&uart_out, uart_putchar, NULL, _FDEV_SETUP_WRITE);
    fdev_setup_stream(&uart_in, NULL, uart_getchar, _FDEV_SETUP_READ);
    stdout = &uart_out;
    stdin  = &uart_in;
}

void UartStdio::print(const __FlashStringHelper *msg)
{
    fputs_P((PGM_P)msg, stdout);
}

void UartStdio::print(const char *msg)
{
    fputs(msg, stdout);
}

void UartStdio::print(uint16_t val)
{
    printf("%u", val);
}

void UartStdio::print(uint32_t val)
{
    printf("%lu", val);
}

void UartStdio::print(char c)
{
    putchar(c);
}

void UartStdio::println(const __FlashStringHelper *msg)
{
    fputs_P((PGM_P)msg, stdout);
    putchar('\n');
}

void UartStdio::println(const char *msg)
{
    puts(msg);
}

void UartStdio::println(uint16_t val)
{
    printf("%u\n", val);
}

void UartStdio::println()
{
    putchar('\n');
}

void UartStdio::printf_P(PGM_P fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf_P(stdout, fmt, ap);
    va_end(ap);
}
