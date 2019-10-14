/*
 * maps.c
 *
 * Description: Functions to maintain the address maps
 */
#include <DataManager/maps.h>
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

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

extern tskTCB * volatile pxCurrentTCB;

/*
 * description: reset all the mapSwitcher and maps
 * parameters: none
 * return: none
 * */
 void init(){
    memset(mapSwitcher, 0, sizeof(unsigned int) * NUMCOMMIT);

    memset(map0, 0, sizeof(void*) * NUMOBJ);
    memset(validBegin0, 0, sizeof(unsigned long) * NUMOBJ);
    memset(validEnd0, 0, sizeof(unsigned long) * NUMOBJ);

    memset(map1, 0, sizeof(void*) * NUMOBJ);
    memset(validBegin1, 0, sizeof(unsigned long) * NUMOBJ);
    memset(validEnd1, 0, sizeof(unsigned long) * NUMOBJ);
}

 /*
  * description: return the address for the commit data, reduce the valid interval
  * parameters: number of the object
  * return: none
  * */
void accessCache(int numObj){
    int prefix = numObj/16,postfix = numObj%16;
    if(CHECK_BIT(mapSwitcher[prefix], postfix) > 0)
        pxCurrentTCB->vBegin = max(pxCurrentTCB->vBegin, validBegin1[numObj]+1);
    else
        pxCurrentTCB->vBegin = max(pxCurrentTCB->vBegin, validBegin0[numObj]+1);
    return;
}

/*
 * description: return the address for the commit data, reduce the valid interval
 * parameters: number of the object
 * return: none
 * */
void* access(int numObj){
    int prefix = numObj/16,postfix = numObj%16;
    if(CHECK_BIT(mapSwitcher[prefix], postfix) > 0){
        pxCurrentTCB->vBegin = max(pxCurrentTCB->vBegin, validBegin1[numObj]+1);
        return map1[numObj];
    }
    else{
        pxCurrentTCB->vBegin = max(pxCurrentTCB->vBegin, validBegin0[numObj]+1);
        return map0[numObj];
    }
}

volatile int dummy;// the compiler mess up something which will skip compiling the CHECK_BIT procedure, we need this to make the if/else statement work!
/*
 * description: return the address for the commit data
 * parameters: number of the object
 * return: none
 * */
void* accessData(int numObj){
    int prefix = numObj/16,postfix = numObj%16;
    if(CHECK_BIT(mapSwitcher[prefix], postfix) > 0){
        dummy = 1;
        return map1[numObj];
    }
    else{
        dummy = 0;
        return map0[numObj];
    }
}

/*
 * description: commit the address for certain commit data
 * parameters: number of the object, source address
 * return: none
 * */
void commit(int numObj, void* commitaddress, unsigned long vBegin, unsigned long vEnd){
    int prefix = numObj/16,postfix = numObj%16;
    if(CHECK_BIT(mapSwitcher[prefix], postfix) > 0){
        map0[numObj] = commitaddress;
        validBegin0[numObj] = vBegin;
        validEnd0[numObj] = vEnd;
    }
    else{
        map1[numObj] = commitaddress;
        validBegin1[numObj] = vBegin;
        validEnd1[numObj] = vEnd;
    }

    //atomic commit
    mapSwitcher[prefix] ^= 1 << (postfix);
    //TODO: we need to use some trick to the stack pointer to use pushm for multiple section
}


/*
 * description: commit the address for certain commit data
 * parameters: number of the object
 * return: value of the begin interval
 * */
unsigned long getBegin(int numObj){
    int prefix = numObj/16, postfix = numObj%16;
    if(CHECK_BIT(mapSwitcher[prefix], postfix) > 0){
        dummy = 1;
        return validBegin1[numObj];
    }
    else{
        dummy = 0;
        return validBegin0[numObj];
    }
}


/*
 * description: commit the address for certain commit data
 * parameters: number of the object
 * return: value of the End interval
 * */
unsigned long getEnd(int numObj){
    int prefix = numObj/16,postfix = numObj%16;
    if(CHECK_BIT(mapSwitcher[prefix], postfix) > 0){
        dummy = 1;
        return validEnd1[numObj];
    }
    else{
        dummy = 0;
        return validEnd0[numObj];
    }
}


/*
 * description: use for debug, dump all info.
 * parameters: none
 * return: none
 * */
void dumpAll(){
    int i;
    printf("address maps\n");
    for(i = 0; i < NUMOBJ; i++){
        printf("%d: %p\n", i, accessData(i));
    }
    printf("mapSwitcher\n");
    for(i = 0; i < NUMOBJ; i++){
        int prefix = i/8, postfix = i%8;
        if(CHECK_BIT(mapSwitcher[prefix], postfix) > 0)
            printf("1");
        else
            printf("0");
    }
    printf("\n");
}
