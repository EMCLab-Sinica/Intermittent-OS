/*
 * config.h
 *
 *  Description: Configurations parameters
 */

#ifndef CONFIG_H_
#define CONFIG_H_

//use the ADC to set the low voltage detection parameters, please refer to our paper and your device manual to set the following two parameters
#define ADC_MONITOR_THRESHOLD 2.4 //the operating voltage of the used capacitor
#define ADC_MONITOR_THRESHOLD_GAP 0.1 //The maximum required energy that is sufficient to run the device before switching out a task

#define DEBUGOVERFLOW //take it out for fast recovery

#define NUMTASK 12 //10 user tasks + 2 FreeRTOS tasks
#define MAXREAD 8

//Used for demo
#define IDIDLE 0
#define IDTIMER 1
#define IDMATMUL 2
#define IDMATH32 3

#endif /* CONFIG_H_ */
