/*
 * hwsetup.c
 *
 *  Descriptions: Implementation of functions to setup hardware of the board
 */

#include "hwsetup.h"

/* we set the CPU frequency as 16 MHz by default */
unsigned int FreqLevel = 8;

/*
 * description: Initialize the ADC to get interrupt for low voltage detection
 * parameters: none
 * return: none
 * note: need to set up timer properly to polling the voltage via the ADC (e.g., set it as OUTMOD_3)
 * */
void initVDetector()
{
    /* Configure internal 2.0V reference. */
    while(REFCTL0 & REFGENBUSY);
    REFCTL0 |= REFVSEL_1 | REFON;
    while(!(REFCTL0 & REFGENRDY));

    ADC12CTL0 = ADC12SHT0_2 | ADC12ON;
    ADC12CTL1 = ADC12SHS_1 | ADC12SSEL_0 | ADC12CONSEQ_2 | ADC12SHP;
    ADC12CTL3 = ADC12BATMAP;
    ADC12MCTL0 = ADC12INCH_31 | ADC12VRSEL_1 | ADC12WINC;
    ADC12HI = (uint16_t)(4096*((ADC_MONITOR_THRESHOLD+ADC_MONITOR_THRESHOLD_GAP)/2)/(2.0));
    ADC12LO = (uint16_t)(4096*(ADC_MONITOR_THRESHOLD/2)/(2.0));
    ADC12IFGR2 &= ~(ADC12HIIFG | ADC12LOIFG | ADC12INIFG);
    ADC12IER2 = ADC12HIIE;
    ADC12CTL0 |= ADC12ENC;

    voltage = ABOVE;
}


/*
 * description: Initialize clocks and UART of the MSP430 board
 * parameters: none
 * return: none
 * */
void prvSetupHardware( void )
{
    /* Stop Watchdog timer. */
    WDT_A_hold( __MSP430_BASEADDRESS_WDT_A__ );

    /* Set PJ.4 and PJ.5 for LFXT. */
    GPIO_setAsPeripheralModuleFunctionInputPin(  GPIO_PORT_PJ, GPIO_PIN4 + GPIO_PIN5, GPIO_PRIMARY_MODULE_FUNCTION  );

    /* Set DCO frequency to 16 MHz. */
    setFrequency(FreqLevel);

    /* Set external clock frequency to 32.768 KHz. */
    CS_setExternalClockSource( 32768, 0 );

    /* Set ACLK = LFXT. */
    CS_initClockSignal( CS_ACLK, CS_LFXTCLK_SELECT, CS_CLOCK_DIVIDER_1 );

    /* Set SMCLK = DCO with frequency divider of 1. */
    CS_initClockSignal( CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1 );

    /* Set MCLK = DCO with frequency divider of 1. */
    CS_initClockSignal( CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1 );

    /* Start XT1 with no time out. */
    CS_turnOnLFXT( CS_LFXT_DRIVE_0 );

    /* Disable the GPIO power-on default high-impedance mode. */
    PMM_unlockLPM5();

    /* Initialize UART */
    uartinit();
}

/*
 * description: Interrupt handler for low voltage detection
 * parameters: none
 * return: none
 */
#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR(void)
{
  switch(__even_in_range(ADC12IV,76))
  {
    case  ADC12IV_NONE: break;                // Vector  0:  No interrupt
    case  ADC12IV_ADC12OVIFG: break;          // Vector  2:  ADC12MEMx Overflow
    case  ADC12IV_ADC12TOVIFG: break;         // Vector  4:  Conversion time overflow
    case  ADC12IV_ADC12HIIFG:                  // Vector  6:  ADC12HI
        /* Disable the high side and enable the low side interrupt.
         * Shouldn't go here for regular capacitor because we reset this detector after power resumption*/
        ADC12IER2 &= ~ADC12HIIE;
        ADC12IER2 |= ADC12LOIE;
        ADC12IFGR2 &= ~ADC12LOIFG;
        voltage = ABOVE;
        break;
    case  ADC12IV_ADC12LOIFG:                  // Vector  8:  ADC12LO
        /* Disable the low side and enable the high side interrupt. */
        ADC12IER2 &= ~ADC12LOIE;
        ADC12IER2 |= ADC12HIIE;
        ADC12IFGR2 &= ~ADC12HIIFG;
        voltage = BELOW;
        break;
    case ADC12IV_ADC12INIFG: break;           // Vector 10:  ADC12IN
    case ADC12IV_ADC12IFG0:                   // Vector 12:  ADC12MEM0
        break;
    case ADC12IV_ADC12IFG1:                   // Vector 14:  ADC12MEM1
        break;
    case ADC12IV_ADC12IFG2: break;            // Vector 16:  ADC12MEM2
    case ADC12IV_ADC12IFG3: break;            // Vector 18:  ADC12MEM3
    case ADC12IV_ADC12IFG4: break;            // Vector 20:  ADC12MEM4
    case ADC12IV_ADC12IFG5: break;            // Vector 22:  ADC12MEM5
    case ADC12IV_ADC12IFG6: break;            // Vector 24:  ADC12MEM6
    case ADC12IV_ADC12IFG7: break;            // Vector 26:  ADC12MEM7
    case ADC12IV_ADC12IFG8: break;            // Vector 28:  ADC12MEM8
    case ADC12IV_ADC12IFG9: break;            // Vector 30:  ADC12MEM9
    case ADC12IV_ADC12IFG10: break;           // Vector 32:  ADC12MEM10
    case ADC12IV_ADC12IFG11: break;           // Vector 34:  ADC12MEM11
    case ADC12IV_ADC12IFG12: break;           // Vector 36:  ADC12MEM12
    case ADC12IV_ADC12IFG13: break;           // Vector 38:  ADC12MEM13
    case ADC12IV_ADC12IFG14: break;           // Vector 40:  ADC12MEM14
    case ADC12IV_ADC12IFG15: break;           // Vector 42:  ADC12MEM15
    case ADC12IV_ADC12IFG16: break;           // Vector 44:  ADC12MEM16
    case ADC12IV_ADC12IFG17: break;           // Vector 46:  ADC12MEM17
    case ADC12IV_ADC12IFG18: break;           // Vector 48:  ADC12MEM18
    case ADC12IV_ADC12IFG19: break;           // Vector 50:  ADC12MEM19
    case ADC12IV_ADC12IFG20: break;           // Vector 52:  ADC12MEM20
    case ADC12IV_ADC12IFG21: break;           // Vector 54:  ADC12MEM21
    case ADC12IV_ADC12IFG22: break;           // Vector 56:  ADC12MEM22
    case ADC12IV_ADC12IFG23: break;           // Vector 58:  ADC12MEM23
    case ADC12IV_ADC12IFG24: break;           // Vector 60:  ADC12MEM24
    case ADC12IV_ADC12IFG25: break;           // Vector 62:  ADC12MEM25
    case ADC12IV_ADC12IFG26: break;           // Vector 64:  ADC12MEM26
    case ADC12IV_ADC12IFG27: break;           // Vector 66:  ADC12MEM27
    case ADC12IV_ADC12IFG28: break;           // Vector 68:  ADC12MEM28
    case ADC12IV_ADC12IFG29: break;           // Vector 70:  ADC12MEM29
    case ADC12IV_ADC12IFG30: break;           // Vector 72:  ADC12MEM30
    case ADC12IV_ADC12IFG31: break;           // Vector 74:  ADC12MEM31
    case ADC12IV_ADC12RDYIFG: break;          // Vector 76:  ADC12RDY
    default: break;
  }
}
