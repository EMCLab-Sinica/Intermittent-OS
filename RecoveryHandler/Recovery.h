/*
 * Recover.h
 *
 *  Created on: 2018¦~2¤ë12¤é
 *      Author: Meenchen
 */

#include <config.h>

#ifndef RECOVERYHANDLER_RECOVERY_H_
#define RECOVERYHANDLER_RECOVERY_H_

void taskRerun();

#pragma NOINIT(RecreateTime)
static unsigned short RecreateTime[NUMTASK];// record how many times a unfinished task is recreated
/* Used for rerunning unfinished tasks */
#pragma NOINIT(unfinished)
static unsigned short unfinished[NUMTASK];// 1: running, others for invalid
#pragma NOINIT(address)
static void* address[NUMTASK];// Function address of tasks
#pragma NOINIT(priority)
static unsigned short priority[NUMTASK];
#pragma NOINIT(TCBNum)
static unsigned short TCBNum[NUMTASK];
#pragma NOINIT(TCBAdd)
static void* TCBAdd[NUMTASK];// TCB address of tasks
#pragma NOINIT(schedulerTask)
static int schedulerTask[NUMTASK];// if it is schduler's task, we don't need to recreate it because the scheduler does
#pragma NOINIT(tID)
static int tID[NUMTASK];// Function address of tasks

void resetTasks();
void taskRerun();
void markCommit();
void regTaskStart(void* add, unsigned short pri, unsigned short TCB, void* TCBA, int stopTrack, int taskID);
void regTaskEnd();
void regTaskEndByIdle(int TCBNuM);
void failureRecovery();
void freePreviousTasks();

#endif /* RECOVERYHANDLER_RECOVERY_H_ */
