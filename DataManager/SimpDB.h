/*
 * SimpDB.h
 *
 *  Description: This simple DB is used to manage data and task snapshot(stacks)
 */
#include <../DataManager/maps.h>
#include <FreeRTOSConfig.h>
#include <stdint.h>
#include <config.h>

#define TOTAL_DATA_SIZE 16 * NUMOBJ
#define DWORKSIZE NUMTASK*(NUMOBJ+1)*4 //every one (even for the creation) for a word for now; TODO: better utilization is needed

#define STATICSTACKVMSIZE 400

// used for validation: Task t with Task's TCB = WSRTCB[i], SRBegin[NUMTASK] = min(writer's begin), WSRValid[NUMTASK] = 1
static unsigned long WSRBegin[NUMTASK]; //The "begin time of every commit operation" for an object "read by task i" is saved in WSRBegin[i]
static unsigned short WSRTCB[NUMTASK];
static unsigned char WSRValid[NUMTASK];

struct working{//working space of data for tasks
    void* address;
    int loc;//1 stands for SRAM, 0 stands for¡@NVM
    int id;//-1 for create
};

struct data{//two-version data structure
    void* cacheAdd;//Should point to VM or NVM(depends on mode)
    unsigned int size;
    unsigned short readTCBNum[MAXREAD];//store 5 readers' TCB number
};

/* for validation */
extern unsigned long timeCounter;

#pragma NOINIT(DBSpace) //space for maintaining data structure of data
static uint8_t DBSpace[TOTAL_DATA_SIZE];

#pragma NOINIT(DB) //data structures for all data
static struct data* DB;

#pragma DATA_SECTION(dataId, ".map") //id for data labeling
static int dataId;

/* Functions to access and maintain data objects */
void constructor();
void destructor();
int DBcommit(struct working *work, int size, int num);
void* DBread(int id);
void DBreadIn(void* to,int id);
void DBworking(struct working* wIn, int id);
void * getStackVM(int taskID);
void * getTCBVM(int taskID);

/* functions for validation*/
void registerTCB(int id);
void unresgisterTCB(int id);

/* internal functions */
static unsigned long min(unsigned long a, unsigned long b){
    if (a > b)
        return b;
    else
        return a;
}

