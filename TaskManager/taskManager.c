/*
 * taskManager.c
 *
 *  Created on: 2019¦~3¤ë8¤é
 *      Author: Meenchen
 */

//This module will handle the stack, heap, data, register of "lengthy" tasks

#include <TaskManager/taskManager.h>


/*
 * Task control block.  A task control block (TCB) is allocated for each task,
 * and stores task state information, including a pointer to the task's context
 * (the task's run time environment, including register values)
 */
typedef struct tskTaskControlBlock
{
    volatile StackType_t    *pxTopOfStack;  /*< Points to the location of the last item placed on the tasks stack.  THIS MUST BE THE FIRST MEMBER OF THE TCB STRUCT. */

    #if ( portUSING_MPU_WRAPPERS == 1 )
        xMPU_SETTINGS   xMPUSettings;       /*< The MPU settings are defined as part of the port layer.  THIS MUST BE THE SECOND MEMBER OF THE TCB STRUCT. */
    #endif

    ListItem_t          xStateListItem; /*< The list that the state list item of a task is reference from denotes the state of that task (Ready, Blocked, Suspended ). */
    ListItem_t          xEventListItem;     /*< Used to reference a task from an event list. */
    UBaseType_t         uxPriority;         /*< The priority of the task.  0 is the lowest priority. */
    StackType_t         *pxStack;           /*< Points to the start of the stack. */
    char                pcTaskName[ configMAX_TASK_NAME_LEN ];/*< Descriptive name given to the task when created.  Facilitates debugging only. */ /*lint !e971 Unqualified char types are allowed for strings and single characters only. */

    /*------------------------------  Extend to support validation: Start ------------------------------*/
    unsigned long vBegin;
    unsigned long vEnd;
    /*------------------------------  Extend to support validation: End ------------------------------*/
    /*------------------------------  Extend to support dynamic stack: Start ------------------------------*/
    void * AddressOfVMStack;
    void * AddressOffset;
    int StackInNVM;
    int taskID;
    /*------------------------------  Extend to support dynamic stack: End ------------------------------*/
    /*------------------------------  Extend to support dynamic function: Start ------------------------------*/
    void * AddressOfNVMFunction;
    void * AddressOfVMFunction;
    void * CodeOffset;
    int SizeOfFunction;
    int CodeInNVM;
    /*------------------------------  Extend to support dynamic function: End ------------------------------*/

    #if ( portSTACK_GROWTH > 0 )
        StackType_t     *pxEndOfStack;      /*< Points to the end of the stack on architectures where the stack grows up from low memory. */
    #endif

    #if ( portCRITICAL_NESTING_IN_TCB == 1 )
        UBaseType_t     uxCriticalNesting;  /*< Holds the critical section nesting depth for ports that do not maintain their own count in the port layer. */
    #endif

    #if ( configUSE_TRACE_FACILITY == 1 )
        UBaseType_t     uxTCBNumber;        /*< Stores a number that increments each time a TCB is created.  It allows debuggers to determine when a task has been deleted and then recreated. */
        UBaseType_t     uxTaskNumber;       /*< Stores a number specifically for use by third party trace code. */
    #endif

    #if ( configUSE_MUTEXES == 1 )
        UBaseType_t     uxBasePriority;     /*< The priority last assigned to the task - used by the priority inheritance mechanism. */
        UBaseType_t     uxMutexesHeld;
    #endif

    #if ( configUSE_APPLICATION_TASK_TAG == 1 )
        TaskHookFunction_t pxTaskTag;
    #endif

    #if( configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0 )
        void *pvThreadLocalStoragePointers[ configNUM_THREAD_LOCAL_STORAGE_POINTERS ];
    #endif

    #if( configGENERATE_RUN_TIME_STATS == 1 )
        uint32_t        ulRunTimeCounter;   /*< Stores the amount of time the task has spent in the Running state. */
    #endif

    #if ( configUSE_NEWLIB_REENTRANT == 1 )
        /* Allocate a Newlib reent structure that is specific to this task.
        Note Newlib support has been included by popular demand, but is not
        used by the FreeRTOS maintainers themselves.  FreeRTOS is not
        responsible for resulting newlib operation.  User must be familiar with
        newlib and must provide system-wide implementations of the necessary
        stubs. Be warned that (at the time of writing) the current newlib design
        implements a system-wide malloc() that must be provided with locks. */
        struct  _reent xNewLib_reent;
    #endif

    #if( configUSE_TASK_NOTIFICATIONS == 1 )
        volatile uint32_t ulNotifiedValue;
        volatile uint8_t ucNotifyState;
    #endif

    /* See the comments above the definition of
    tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE. */
    #if( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
        uint8_t ucStaticallyAllocated;      /*< Set to pdTRUE if the task is a statically allocated to ensure no attempt is made to free the memory. */
    #endif

    #if( INCLUDE_xTaskAbortDelay == 1 )
        uint8_t ucDelayAborted;
    #endif

} tskTCB;

typedef tskTCB TCB_t;
extern tskTCB * volatile pxCurrentTCB;

#pragma NOINIT(TBuffer)
static unsigned char TBuffer[NUMTASK][sizeof( TCB_t )];

/* used to recover tasks */
void setRunning(int taskID)
{
    Running[taskID] = RUN;
}

/* used to recover tasks */
void setStop(int taksID)
{
    Running[taksID] = STOP;
}

/* get status of the task stack */
int getStatus(int taskID)
{
    return Running[taskID];
}

/* set the task in NVM */
void allocateInNVM(int taskID)
{
    Tallocation[taskID] = INNVM;
}

/* set the task in VM */
void allocateInVM(int taskID)
{
    Tallocation[taskID] = INVM;
}

/* get location of the task stack */
int getLocation(int taskID)
{
    return Tallocation[taskID];
}

/* get the task's workspace */
void* getTaskWork(int taskID)
{
    return DBuffer[taskID];
}

/* get the NVM reserved for that task */
void* getStackAddress(int taskID)
{
    return &SBuffer[taskID][0];
}

/* get the NVM reserver for that task's TCB */
void* getTCBAddress(int taskID)
{
    return &TBuffer[taskID][0];
}

/* get the NVM reserver for that task's TCB */
void suspendLengthy(int current)
{
    int i;

    if(Tallocation[current] == INVM)
    {
        for(i = 0; i < NUMTASK; i++)
            if(Tallocation[i] == INNVM)
                vTaskSuspend(getTCBAddress(i));
        return;
    }
    else
    {
        for(i = 0; i < NUMTASK; i++)
            if(Tallocation[i] == INNVM && i != current)
                vTaskSuspend(getTCBAddress(i));
        vTaskSuspend(NULL);
        return;
    }

}

/*  get a piece of NVM from the data buffer reserved for the task */
void* allocateNVMData(int size, int taskID)
{
    void* ret;

    //let's assume the size is less than DATABUFF for now
    if(DIndex[taskID] + size >= DATABUFF)
        DIndex[taskID] = 0;
    ret = &DBuffer[taskID][DIndex[taskID]];
    DIndex[taskID] += size;
    return ret;
}

/*  get a piece of NVM from the heap buffer reserved for the task */
void* allocateNVMHeap(int size,int taskID)
{
    void* ret;

    //let's assume the size is less than HEAPBUFF for now
    if(HIndex[taskID] + size >= HEAPBUFF)
        HIndex[taskID] = 0;
    ret = &HBuffer[taskID][HIndex[taskID]];
    HIndex[taskID] += size;
    return ret;
}

/* reset the NVM for a task */
void resetTask(int taskID)
{
    /*we don't need to reset the stack memory because should be overwritten by CPU*/
    //memset(SBuffer[taskID],configMINIMAL_STACK_SIZE*sizeof( StackType_t));
    HIndex[taskID] = 0;
    DIndex[taskID] = 0;
    Running[taskID] = 0;
    Tallocation[taskID] = INVM;
}

/* rest all tasks */
void resetAllTasks()
{
    int i;

    for(i = 0;i < NUMTASK; i++)
        resetTask(i);
}
/*  */

