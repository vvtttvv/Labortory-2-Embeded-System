#include "UartStdio.h"

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

    fdev_setup_stream(&uart_out, uart_putchar, NULL, _FDEV_SETUP_WRITE);
    fdev_setup_stream(&uart_in, NULL, uart_getchar, _FDEV_SETUP_READ);
    stdout = &uart_out;
    stdin = &uart_in;
}
