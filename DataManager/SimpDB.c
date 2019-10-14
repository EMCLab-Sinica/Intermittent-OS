/*
 * SimpDB.cpp
 *
 *  Created on: 2017¦~7¤ë12¤é
 *      Author: WeiMingChen
 */

#include <DataManager/SimpDB.h>
#include <RecoveryHandler/Recovery.h>
#include <FreeRTOS.h>
#include <stdio.h>
#include <task.h>
#include <config.h>

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

//Space for working versions at SRAM
static long Working[NUMTASK][NUMOBJ+1];

/* stacks allocated for tasks */
#pragma location = 0x1C00 //Space for working at SRAM
static unsigned char StacksVM[STATICSTACKVMSIZE*NUMTASK];
static unsigned char TCBVM[sizeof( tskTCB )*NUMTASK];

extern tskTCB * volatile pxCurrentTCB;


/*
 * description: initialize all data structure in the database
 * parameters: none
 * return: none
 * */
void constructor(){
    init();
    DB = (struct data*) DBSpace;
    int i,j;
    for(i = 0; i < NUMOBJ; i++){
        DB[i].cacheAdd = NULL;
        DB[i].size = 0;
        for(j = 0; j <MAXREAD; j++)
            DB[i].readTCBNum[j] = 0;
    }
    dataId = 0;

    for(i = 0; i < NUMTASK; i++)
        WSRValid[i] = 0;
}

/*
 * description: free all allocated data, implemented for completeness and not used currently
 * parameters: none
 * return: none
 * */
void destructor(){
    int i;
    for(i = 0; i < NUMOBJ; i++){
        if(DB[i].size > 0)
            vPortFree(accessData(i));
    }
}

/*
 * description: create/write a data entry
 * parameters: source address of the data(max for 3 data commit atomically), size in terms of bytes
 * return: the id of the data, -1 for failure
 * note: currently support for committing one data object
 * */
int DBcommit(struct working *work, int size, int num){
    int creation = 0,workId;
    void* previous;

    /* creation or invalid ID */
    if(work->id < 0){
        work->id = dataId++;
        creation = 1;
    }
    else//need to free it after commit
        previous = accessData(work->id);

    if(work->id >= NUMOBJ)
        return -1;

    workId = work->id;
    taskENTER_CRITICAL();

    int i,j;
    /* Validation */
    // for read set that have been updated after the read
    for(j = 0; j < NUMTASK; j++){
       if(WSRValid[j] > 0 && WSRTCB[j] == pxCurrentTCB->uxTCBNumber){
           pxCurrentTCB->vEnd = min(pxCurrentTCB->vEnd,WSRBegin[j]-1);
           WSRBegin[j] = 4294967295;//incase for a task with multiple commits
           break;
       }
    }

    // for write set:
    if(creation == 0)
        pxCurrentTCB->vBegin = max(pxCurrentTCB->vBegin, getBegin(workId)+1);

    // should be finished no more later than current time
    pxCurrentTCB->vEnd = min(pxCurrentTCB->vEnd, timeCounter);

    // validation fail
    if(pxCurrentTCB->vBegin > pxCurrentTCB->vEnd){
        taskEXIT_CRITICAL();
        regTaskEnd();
        taskRerun();
        return -1;
    }

    /* validation success, commit all changes*/
    //working at VM, then write to a new space in NVM as consistent version
    void* temp = (void*)pvPortMalloc(size);
    memcpy(temp, work->address, size);
    commit(workId,temp, pxCurrentTCB->vBegin, pxCurrentTCB->vEnd);
    markCommit(pxCurrentTCB->taskID);

    /* Free the previous consistent data */
    if(creation == 0)
        vPortFree(previous);

    /* Link the data */
    DB[workId].size = size;
    DB[workId].cacheAdd = work->address;

    /* validation: for those written data read by other tasks*/
    // all write set's readers can be removed from readTCBNum[] after their valid interval is reduced
    for(i = 0; i < MAXREAD; i++){
        if(DB[workId].readTCBNum[i] != 0){
            //no point to self-restricted
            if(DB[workId].readTCBNum[i] == pxCurrentTCB->uxTCBNumber){
                DB[workId].readTCBNum[i] = 0;
                continue;
            }

            //search for valid, the task has read the written data
            for(j = 0; j < NUMTASK; j++){
                if(WSRValid[j] == 1 &&  WSRTCB[j] == DB[workId].readTCBNum[i]){
                    WSRBegin[j] = min(pxCurrentTCB->vBegin,WSRBegin[j]);
                    break;
                }
            }
            DB[workId].readTCBNum[i] = 0;// configure for write set's readers
        }
    }

    taskEXIT_CRITICAL();
    return workId;
}

/*
 * description: return the address of a data copy of the data object
 * parameters: id of the data
 * return: the pointer of data, NULL for failure
 * */
void* DBread(int id){

    if(id > NUMOBJ || id < 0 || DB[id].size <= 0)
        return NULL;
    else{
        /* Validation: save the reader's TCBNumber for committing tasks */
        int i;
        //can use Mutex for efficiency
        taskENTER_CRITICAL();
        for(i = 0; i < MAXREAD; i++)
            if(DB[id].readTCBNum[i] == pxCurrentTCB->uxTCBNumber){
                i = -1;
                break;//already in the list
            }
        if(i != -1)
            for(i = 0; i < MAXREAD; i++){
                if(DB[id].readTCBNum[i] == 0){
                    DB[id].readTCBNum[i] = pxCurrentTCB->uxTCBNumber;//put it to the list
                    break;
                }
            }
        taskEXIT_CRITICAL();

        if(DB[id].cacheAdd != NULL){
            accessCache(id);
            return DB[id].cacheAdd;
        }
        else/* Return the data */
            return access(id);
    }
}

/*
 * description: read the data from memory
 * parameters: read to where, id of the data
 * return:
 * */
void DBreadIn(void* to,int id){
    //TODO: range of id needs to be checked here
    memcpy(to, DBread(id), DB[id].size);
}

/*
 * description: return a working space for the task
 * parameters: data structure of working space, size of the required data
 * return: none
 * */
void DBworking(struct working* wIn, int id)
{
    if(id >= 0 && id < 16)
        wIn->address = &Working[pxCurrentTCB->taskID][id];
    else
        wIn->address = &Working[pxCurrentTCB->taskID][16];//default for creation
    wIn->id = id;

    return;
}

/*
 * description: start the concurrency control of the current task, this function will register the current TCB to the DB, and initialize the TCB's validity interval
 * parameters: the TCB number
 * return: none
 * */
void registerTCB(int id){
    int i;
    unsigned short TCB = pxCurrentTCB->uxTCBNumber;

    //initialize the TCB's validity interval
    pxCurrentTCB->vBegin = 0;
    pxCurrentTCB->vEnd = 4294967295;
    //register the current TCB to the DB,
    for(i = 0; i < NUMTASK; i++){
        if(!WSRValid[i]){
            WSRTCB[i] = TCB;
            WSRBegin[i] = 4294967295;
            WSRValid[i] = 1;
            return;
        }
    }
    //should not be here
    //TODO: error handling
}


/*
 * description: finish the concurrency control of the current task, unregister the current TCB from the DB
 * parameters: the TCB number
 * return: none
 * */
void unresgisterTCB(int id)
{
    int i;
    unsigned short TCB = pxCurrentTCB->uxTCBNumber;
    for(i = 0; i < NUMTASK; i++){
        if(WSRValid[i]){
            if(WSRTCB[i] == TCB){
                WSRValid[i] = 0;
                return;
            }
        }
    }
    //TODO: error handling
}


/*
 * description: get the stack allocated for the task
 * parameters: task ID
 * return: the designated space for the task
 * */
void* getStackVM(int taskID)
{
    return &StacksVM[STATICSTACKVMSIZE*taskID];
}

/*
 * description: get the TCB allocated for the task
 * parameters: task ID
 * return: the designated space for the task
 * */
void* getTCBVM(int taskID)
{
    return &TCBVM[sizeof( tskTCB )*taskID];
}

