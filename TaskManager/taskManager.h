/*
 * taskManager.h
 *
 *  Created on: 2019¦~3¤ë8¤é
 *      Author: Meenchen
 *      Description: We build every "static" for the ease of implementation, which means some parameters need to be properly set for successful usage
 */

#ifndef TASKMANAGER_TASKMANAGER_H_
#define TASKMANAGER_TASKMANAGER_H_

#include <FreeRTOS.h>
#include <task.h>
#include "config.h"

#define HEAPBUFF 10
#define DATABUFF 10

enum{
    INVM =0,
    INNVM
};

enum{
    STOP = 0,
    RUN
};

//Indicator for recovery
#pragma NOINIT(Tallocation)
static unsigned char Tallocation[NUMTASK];
#pragma NOINIT(Running)
static unsigned char Running[NUMTASK];


//maintain all NVM tasks
#pragma NOINIT(SBuffer)//their stack buffer
static unsigned char SBuffer[NUMTASK][configMINIMAL_STACK_SIZE*sizeof( StackType_t)];//12*140
#pragma NOINIT(HBuffer)//heap buffers
static unsigned char HBuffer[NUMTASK][HEAPBUFF];
#pragma NOINIT(HIndex)//indexes for each buffer usage
static unsigned int HIndex[NUMTASK];
#pragma NOINIT(DBuffer)//data buffers
static unsigned char DBuffer[NUMTASK][DATABUFF];
#pragma NOINIT(DIndex)//indexes for each buffer usage
static unsigned int DIndex[NUMTASK];



/* used to recover tasks */
void setRunning(int taskID);
/* used to recover tasks */
void setStop(int taksID);
/* get status of the task stack */
int getStatus(int taskID);//if it is running, we cannot guarantee its register is properly saved
/* set the task in NVM */
void allocateInNVM(int taskID);
/* set the task in VM */
void allocateInVM(int taskID);
/* get location of the task stack */
int getLocation(int taskID);

/* get the task's workspace */
void* getTaskWork(int taskID);
/* get the NVM reserved for that task */
void* getStackAddress(int taskID);
/* get the NVM reserver for that task's TCB */
void* getTCBAddress(int taskID);
/* get a piece of NVM from the data buffer reserved for the task */
void* allocateNVMData(int size, int taskID);
/* get a piece of NVM from the heap buffer reserved for the task */
void* allocateNVMHeap(int size,int taskID);
/* reset the NVM for a task */
void resetTask(int taskID);
/* rest all tasks */
void resetAllTasks();
/* suspend all lengthy tasks */
void suspendLengthy(int current);

#endif /* TASKMANAGER_TASKMANAGER_H_ */
