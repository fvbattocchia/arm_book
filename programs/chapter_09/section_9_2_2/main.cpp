#include <arm_book_lib.h>
#include <mbed.h>

#define ESP8266_BAUD_RATE      115200
#define UART_USB_BAUD_RATE     115200

Serial esp8266Uart( D42, D41 );
Serial uartUsb(USBTX, USBRX);

int main()
{
    char receivedCharUsb;
    char receivedCharEsp;
    bool prepareWeb = false;

    esp8266Uart.baud(ESP8266_BAUD_RATE);
    uartUsb.baud(UART_USB_BAUD_RATE);

    uartUsb.printf("AT COMMANDS test\r\n");

    while( true ) {

        if( uartUsb.readable() ) {
            receivedCharUsb = uartUsb.getc();
            if (prepareWeb) {
                if ( ( receivedCharUsb == 'h' ) || ( receivedCharUsb == 'H' ) )  {
                    esp8266Uart.printf("<!doctype html> <html> <body> Hello! </body> </html>");
                    prepareWeb = false;
                }
            } else if ( receivedCharUsb == '\r' ) {
                esp8266Uart.printf("\r\n");
            } else {
                esp8266Uart.putc( receivedCharUsb );
            }
        }

        if ( esp8266Uart.readable() ) {
            receivedCharEsp = esp8266Uart.getc();
            uartUsb.putc( receivedCharEsp );
            if (receivedCharEsp == '>') {
                prepareWeb = true;
            }
        }
    }
}