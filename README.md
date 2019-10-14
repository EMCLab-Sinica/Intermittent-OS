# Intermittent OS

<!-- ABOUT THE PROJECT -->
## Project Description

This project develops an intermittent operating system (OS) to preserve computation progress across power cycles without checkpointing for energy-harvesting systems. The intermittent OS is endowed with several capabilities as follows:
1. run multiple tasks *concurrently* to improve computation progress 
2. achieve *consistency* between data and computing progress
3. recover the system *instantly* from power failures
4. accumulatively preserve computation progress across power cycles to avoid progress *stagnation*


The intermittent OS is built upon FreeRTOS, a real-time operating system supporting many kinds of commercial microcontrollers, running
on MSP-EXP430FR5994 LaunchPad, a Texas Instruments platform featuring 256KB FRAM and 8KB on-chip SRAM. We add a data manager and a recovery handler in FreeRTOS, so that the system runtime can cope with intermittence and exempts application developers from this responsibility. Due to the limitation of the memory size, the current implementation supports up to 10 user tasks and 16 data objects. For more technical details, please refer to [our paper](https://arxiv.org/pdf/1910.04949.pdf "link") and [its previous version](https://www.citi.sinica.edu.tw/papers/pchsiu/6715-F.pdf "link").

<p align="center">
<img src="https://github.com/meenchen/failure-resilient-OS/blob/master/SystemOverview.jpg" width="720">
</p>

<!-- TABLE OF CONTENTS -->
## Table of Contents
* [Getting Started](#getting-started)
  * [Prerequisites](#prerequisites)
  * [Setup and Build](#setup-and-build)
* [Porting to Other Devices](#porting-to-other-devices)
  * [Memory Map](#memory-map)
  * [System Hardware](#system-hardware)
* [License](#license)
* [Contact](#contact)
<!--* [Contributing](#contributing)-->
<!-- GETTING STARTED -->
## Getting Started

### Prerequisites

Here is the basic software and hardware you need to build the applications running on this intermittent OS. 

* [Code composer studio](http://www.ti.com/tool/CCSTUDIO "link") (recommended versions: > 7.0)
* [MSP-EXP430FR5994 LaunchPad](http://www.ti.com/tool/MSP-EXP430FR5994 "link")

However, if you are using other software (i.e., IDE or compiler) or other boards to develope your applications, please refer to [here](#porting-to-other-devices) for device settings.

### Setup and Build

1. Download/clone this repository

2. Import this project to your workspace of code composer studio (CCS). Do the following steps in your CCS
  * go file -> import 
  * select Code Composer Studio -> CCS Projects
  * search the directory of the downloaded project
  * select the project, namely ``Failure-resilient-Intermittent-Systems``, and click finish
  
3. Set up for your energy management unit

All project files should be properly referred by you CCS. In this project, we detect the voltage of the capacitor to generate interrupts when the energy in the capacitor is lower than a threshold. The threshold depends on the capacitance, maximum power consumption, and the context switch period of the operating system. To set up the threshold properly, please go to the configuration file ``config.h`` to set up two parameters, ADC_MONITOR_THRESHOLD and ADC_MONITOR_THRESHOLD_GAP.

```JS
#define ADC_MONITOR_THRESHOLD V_op
#define ADC_MONITOR_THRESHOLD_GAP V_gap
```

The V_op is the operating voltage of your energy management units. For example, we use a switch to turn on (resp. off) the device when the voltage of the capacitor is low than 2.8V (resp. 2.4V). In this case, the ADC_MONITOR_THRESHOLD should be set as 2.4. Since the fine-grained setting of V_gap needs the profiling the system before running the demo, we set as it to 0.1 by default as a coarse-grained solution. If you want to know more details of configuring the parameter, please refer to our paper (will be release in the near future).

 4. Build and flash to your device

Now, the demo project is ready to go. Just launch the demo application by clicking the debug button. In CCS, you can trace how the design work step by step. 

## Porting to Other Devices

This project is self-contained and very portable among MSP430-based devices which is equipped with FRAM. In different development environment (e.g., other IED), you can directly include all c and header files to your project. However, the configuration file for hardware setting (e.g., the memory map for partitions) should be modified according to your IDE and device specification. 

  ### Memory Map

The first thing is to create memory partitions (SRAM and FRAM) based on our memory map configuration (``lnk_msp430fr5994.cmd``). You need to allocate sufficient memory space for the RAM, MAP and NOINT partitions used by our design. If the allocated memory space is not sufficient, you may see some error messages when you try to flash your compiled binary file to your device.

```JS
MEMORY
{
    TINYRAM                 : origin = 0xA, length = 0x16
    BSL                     : origin = 0x1000, length = 0x800
    INFOD                   : origin = 0x1800, length = 0x80
    INFOC                   : origin = 0x1880, length = 0x80
    INFOB                   : origin = 0x1900, length = 0x80
    INFOA                   : origin = 0x1980, length = 0x80
    RAM                     : origin = 0x1C00, length = 0x2000
    MAP						          : origin = 0xFE00, length = 0x0180
    NOINI                   : origin = 0x10000,length = 0x24000
    ...
```

  ### System Hardware

The scheduler in FreeRTOS and the low-voltage detector respectively use hardware timers to generate interrupts. Thus, you need to modify the ``vApplicationSetupTimerInterrupt``, ``vConfigureTimerForRunTimeStats``, ``ADC12_ISR``, ``initVDetector``, and ``vTickISREntry`` according to your device specification. The detailed steps are listed as follows.

 * Timer setup: The two timer should be setup properly in the following two functions. First of all, one timer should be configured to periodically trigger the ADC on the device to poll the current voltage level. Secondly, the other timer should be configured to periodically trigger interrupt events where the operating system performs context switch according to the operating system's context switch period.
```JS
void vApplicationSetupTimerInterrupt( void )
{
const unsigned short usACLK_Frequency_Hz = 32768;

    /* Ensure the timer is stopped. */
    TA0CTL = 0;
    ...
    
    void vConfigureTimerForRunTimeStats( void )
{
    /* Configure a timer that is used as the time base for run time stats.  See
    http://www.freertos.org/rtos-run-time-stats.html */
    
    /* Ensure the timer is stopped. */
    TA1CTL = 0;
    ...
```
 * ADC (Analog-to-Digital Converter): Our design uses an ADC to detect the voltage capacitor. The setting of ADC depends on your device, so please refer to the manual for your device and modify the follow function accordingly.
 
```JS
void initVDetector()
{
    /* Configure internal 2.0V reference. */
    while(REFCTL0 & REFGENBUSY);
    ...
```

 * Interrupt service routines: The following two interrupt service routines handle the interrupt events generated by the timer and ADC. After your timers and ADC are set, please update the vector and bits set/toggle in the handler functions to match the interrupt service routines with proper interrupt sources.

```JS
#pragma vector=configTICK_VECTOR
interrupt void vTickISREntry( void )
{
extern void vPortTickISR( void );
	__bic_SR_register_on_exit( SCG1 + SCG0 + OSCOFF + CPUOFF );
    timeCounter++;//keep track of running time
    ...
    
#pragma vector = ADC12_VECTOR
__interrupt void ADC12_ISR(void)
{
  switch(__even_in_range(ADC12IV,76))
  {
    case  ADC12IV_NONE: break;                // Vector  0:  No interrupt
    ...
```
## License

See the [LICENSE](https://github.com/meenchen/failure-resilient-OS/blob/master/LICENSE) file for license rights and limitations


## Contributors

This project is co-led by Dr. Pi-Cheng Hsiu (Academia Sinica) and Dr. Tei-Wei Kuo (National Taiwan University). Listed below are the contributors.

Wei-Ming Chen (Academia Sinica & National Taiwan University),
Pi-Cheng Hsiu (Academia Sinica),
Tei-Wei Kuo (National Taiwan University)

## Contact

If you have any questions or comments, please contact Wei-Ming Chen at d04922006@ntu.edu.tw.

