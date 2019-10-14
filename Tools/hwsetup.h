/*
 * hwsetup.h
 *
 * Descriptions: Header file for setting up required hardware components
 */

#ifndef TOOLS_HWSETUP_H_
#define TOOLS_HWSETUP_H_
#include "config.h"
#include <Tools/dvfs.h>
#include <driverlib.h>

#pragma NOINIT(voltage)
static int voltage;
enum VOL{
    BELOW = 0,
    ABOVE = 1
};

/*
 * Configure low-voltage interrupt
 */
void initVDetector();

/*
 * Configure the hardware as necessary.
 */
void prvSetupHardware( void );

#endif /* TOOLS_HWSETUP_H_ */
