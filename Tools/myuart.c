/*
 * myserial.c
 *
 *  Created on: 2017¦~5¤ë25¤é
 *      Author: Meenchen
 */
#include <stdio.h>
#include <stdarg.h>
#include <Tools/myuart.h>
#include "driverlib.h"
#include <Tools/dvfs.h>

// The following structure will configure the EUSCI_A port to run at 9600 baud from an 1~16MHz ACLK
// The baud rate values were calculated at: http://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430BaudRateConverter/index.html
EUSCI_A_UART_initParam UartParams[8] = {
{//1MHz
    EUSCI_A_UART_CLOCKSOURCE_SMCLK,
    6,                                                                         // clockPrescalar
    8,                                                                         // firstModReg
    17,                                                                        // secondModReg
    EUSCI_A_UART_NO_PARITY,
    EUSCI_A_UART_LSB_FIRST,
    EUSCI_A_UART_ONE_STOP_BIT,
    EUSCI_A_UART_MODE,
    EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION
},{//2.66MHz
    EUSCI_A_UART_CLOCKSOURCE_SMCLK,
    17,                                                                        // clockPrescalar
    5,                                                                         // firstModReg
    2,                                                                         // secondModReg
    EUSCI_A_UART_NO_PARITY,
    EUSCI_A_UART_LSB_FIRST,
    EUSCI_A_UART_ONE_STOP_BIT,
    EUSCI_A_UART_MODE,
    EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION
},{//3.5MHz
   EUSCI_A_UART_CLOCKSOURCE_SMCLK,
   22,                                                                         // clockPrescalar
   12,                                                                         // firstModReg
   107,                                                                        // secondModReg
   EUSCI_A_UART_NO_PARITY,
   EUSCI_A_UART_LSB_FIRST,
   EUSCI_A_UART_ONE_STOP_BIT,
   EUSCI_A_UART_MODE,
   EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION
},{//4MHz
   EUSCI_A_UART_CLOCKSOURCE_SMCLK,
   26,                                                                         // clockPrescalar
   0,                                                                          // firstModReg
   214,                                                                        // secondModReg
   EUSCI_A_UART_NO_PARITY,
   EUSCI_A_UART_LSB_FIRST,
   EUSCI_A_UART_ONE_STOP_BIT,
   EUSCI_A_UART_MODE,
   EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION
},{//5.33MHz
   EUSCI_A_UART_CLOCKSOURCE_SMCLK,
   34,                                                                         // clockPrescalar
   11,                                                                         // firstModReg
   17,                                                                         // secondModReg
   EUSCI_A_UART_NO_PARITY,
   EUSCI_A_UART_LSB_FIRST,
   EUSCI_A_UART_ONE_STOP_BIT,
   EUSCI_A_UART_MODE,
   EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION
},{//7MHz
   EUSCI_A_UART_CLOCKSOURCE_SMCLK,
   45,                                                                         // clockPrescalar
   9,                                                                          // firstModReg
   17,                                                                         // secondModReg
   EUSCI_A_UART_NO_PARITY,
   EUSCI_A_UART_LSB_FIRST,
   EUSCI_A_UART_ONE_STOP_BIT,
   EUSCI_A_UART_MODE,
   EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION
},{//8MHz
   EUSCI_A_UART_CLOCKSOURCE_SMCLK,
   52,                                                                         // clockPrescalar
   1,                                                                          // firstModReg
   73,                                                                         // secondModReg
   EUSCI_A_UART_NO_PARITY,
   EUSCI_A_UART_LSB_FIRST,
   EUSCI_A_UART_ONE_STOP_BIT,
   EUSCI_A_UART_MODE,
   EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION
},{//16MHz
   EUSCI_A_UART_CLOCKSOURCE_SMCLK,
   104,                                                                        // clockPrescalar
   2,                                                                          // firstModReg
   182,                                                                        // secondModReg
   EUSCI_A_UART_NO_PARITY,
   EUSCI_A_UART_LSB_FIRST,
   EUSCI_A_UART_ONE_STOP_BIT,
   EUSCI_A_UART_MODE,
   EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION
}};


void print2uart(char* format,...)
{
    char *traverse;
    int i;
    unsigned long l;
    char *s;

    //Module 1: Initializing Myprintf's arguments
    va_list arg;
    va_start(arg, format);

    for(traverse = format; *traverse != '\0'; traverse++)
    {
        while( *traverse != '%' && *traverse != '\0' )
        {
            EUSCI_A_UART_transmitData(EUSCI_A0_BASE, (uint8_t)*traverse);
            traverse++;
        }

        if(*traverse == '\0')
            break;

        traverse++;

        //Module 2: Fetching and executing arguments
        switch(*traverse)
        {
            case 'c' :
                i = va_arg(arg,int);        //Fetch char argument
                EUSCI_A_UART_transmitData(EUSCI_A0_BASE, (uint8_t)i);
                break;
            case 'l' :
                l = va_arg(arg,unsigned long);        //Fetch Decimal/Integer argument
                print2uart(convertl(l,10));
                break;
            case 'd' :
                i = va_arg(arg,int);        //Fetch Decimal/Integer argument
                if(i<0)
                {
                    i = -i;
                    EUSCI_A_UART_transmitData(EUSCI_A0_BASE, (uint8_t)'-');
                }
                print2uart(convert(i,10));
                break;
            case 's':
                s = va_arg(arg,char *);         //Fetch string
                print2uart(s);
                break;
            case 'x':
                i = va_arg(arg,unsigned int); //Fetch Hexadecimal representation
                print2uart(convert(i,16));
                break;
        }
    }
    //Module 3: Closing argument list to necessary clean-up
    va_end(arg);
}

void dummyprint(char* format,...)
{
    return;
}


void print2uartlength(char* str,int length)
{
    int i;

    for(i = 0; i < length; i++)
    {
        EUSCI_A_UART_transmitData(EUSCI_A0_BASE, (uint8_t)*(str+i));
    }
}

char *convert(unsigned int num, int base)
{
    static char Representation[]= "0123456789ABCDEF";
    static char buffer[50];
    char *ptr;

    ptr = &buffer[49];
    *ptr = '\0';

    do
    {
        *--ptr = Representation[num%base];
        num /= base;
    }while(num != 0);

    return(ptr);
}

char *convertl(unsigned long num, int base)
{
    static char Representation[]= "0123456789ABCDEF";
    static char buffer[50];
    char *ptr;

    ptr = &buffer[49];
    *ptr = '\0';

    do
    {
        *--ptr = Representation[num%base];
        num /= base;
    }while(num != 0);

    return(ptr);
}

/* Initialize serial */
void uartinit()
{
    if(uartsetup == 0){
        // Configure UART
        EUSCI_A_UART_initParam param = UartParams[FreqLevel-1];

        if(STATUS_FAIL == EUSCI_A_UART_init(EUSCI_A0_BASE, &param))
            return;

        EUSCI_A_UART_enable(EUSCI_A0_BASE);

        EUSCI_A_UART_clearInterrupt(EUSCI_A0_BASE,
                                    EUSCI_A_UART_RECEIVE_INTERRUPT);

        // Enable USCI_A0 RX interrupt
        EUSCI_A_UART_enableInterrupt(EUSCI_A0_BASE,
                                     EUSCI_A_UART_RECEIVE_INTERRUPT); // Enable interrupt

        // Enable globale interrupt
        __enable_interrupt();

        // Select UART TXD on P2.0
        GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN0, GPIO_SECONDARY_MODULE_FUNCTION);
        uartsetup = 1;
    }
}


