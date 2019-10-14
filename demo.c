/*
 * DEMO.c
 *
 * Descriptions: Implementation of the demo application
 */
#include <config.h>
#include <TaskManager/taskManager.h>
#include <DataManager/SimpDB.h>
#include <demo.h>

//matrix multiplication
void matrixmultiplication();
//floating math functions
void math32();
/*
 * description: create two tasks as demo applications
 * parameters: none
 * return: none
 * note: the define variable ITERMATRIXMUL, ITERMATH32 can be used to control the workload intensity for each task
 * */
void demo()
{
    xTaskCreate( math32, "math32", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL, IDMATH32, INVM);
    xTaskCreate( matrixmultiplication, "matrix multiplication", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL, IDMATMUL, INVM);
}

typedef unsigned short UInt16;
const UInt16 m1[3][4] = {
    {0x01, 0x02, 0x03, 0x04},
    {0x05, 0x06, 0x07, 0x08},
    {0x09, 0x0A, 0x0B, 0x0C}
    };
const UInt16 m2[4][5] = {
    {0x01, 0x02, 0x03, 0x04, 0x05},
    {0x06, 0x07, 0x08, 0x09, 0x0A},
    {0x0B, 0x0C, 0x0D, 0x0E, 0x0F},
    {0x10, 0x11, 0x12, 0x13, 0x14}
    };
extern int DID0;
/*
 * description: do matrix multiplication
 * parameters: none
 * return: none
 * note: the define variable ITERMATRIXMUL can be used to control the workload intensity
 * */
void matrixmultiplication()
{
    int k, m, n, p;
    volatile long m3[3][5];
    unsigned long progress = 0;//used for debugging lengthy tasks

    while(1)//this task computes repeatedly
    {
        registerTCB(IDMATMUL);
        //matrix multiplication as the computation workloads here
        for(k = 0; k < ITERMATRIXMUL; k++)
        {
            for(m = 0; m < 3; m++)
            {
                for(p = 0; p < 5; p++)
                {
                    m3[m][p] = 0;
                    for(n = 0; n < 4; n++)
                    {
                        m3[m][p] += m1[m][n] * m2[n][p];
                    }
                }
            }
        }
        //commit the resultant value
        struct working data;
        DBworking(&data, DID0);
        unsigned long* ptr = data.address;
        *ptr = m3[progress%3][progress%5];
        DID0 = DBcommit(&data,4,1); //4 byte, 1 data
        progress++;
    }
}

typedef unsigned long UInt32;
UInt32 add(UInt32 a, UInt32 b)
{
    return (a + b);
}
UInt32 mul(UInt32 a, UInt32 b)
{
    return (a * b);
}
UInt32 div(UInt32 a, UInt32 b)
{
    return (a / b);
}

extern int DID1;
/*
 * description: do 32 bit math operations
 * parameters: none
 * return: none
 * note: the define variable ITERMATH32 can be used to control the workload intensity
 * */
void math32()
{
    int i;

    volatile long result32[4];
    volatile long Input[2];
    unsigned long progress = 0;

    while(1)
    {//this task computes repeatedly
        registerTCB(IDMATMUL);
        result32[0] = 43125;
        result32[1] = 14567;
        //mat operations as the computation workloads here
        for(i = 0; i < ITERMATH32; i++)
        {
            result32[2] = result32[0] + result32[1];
            result32[1] = result32[0] * result32[2];
            result32[3] = result32[1] / result32[2];
        }
        struct working data;
        DBworking(&data, DID1);
        unsigned long* ptr = data.address;
        *ptr = result32[3];
        DID1 = DBcommit(&data,4,1); //4 byte, 1 data
        progress++;
    }
}




