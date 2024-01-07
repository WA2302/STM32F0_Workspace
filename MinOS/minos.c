/**
  ******************************************************************************
  * @file    minos.c 
  * @author  Windy Albert
  * @version V2.1
  * @date    04-November-2023
  * @brief   This is the core file for MinOS, a NO GRAB OS for embedded system.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "minos.h"

OS_TCB *OSTCBCur;                    /* Pointer to currently running TCB      */
static OS_TCB OSTCBTbl[OS_MAX_TASKS];/* Table of TCBs                         */

uint32_t SysTime = 0;

/**
  * @brief  All tasks MUST be created completed before OSSTart().
  * @param  task     is a pointer to the task's code
  *         ptos     is a pointer to the task's top of stack.
  * @retval None
  */
void  TaskCreate(void (*task)(void), uint32_t *stk)
{ 
    static uint8_t task_num = 0;
    
    if( task_num >= OS_MAX_TASKS )
    {
        while(1);                       /* Never run to here.                 */
    }
                                        /* Registers auto-saved on exception  */
    *(stk)    = (uint32_t)0x01000000L;  /* xPSR                               */
    *(--stk)  = (uint32_t)task;         /* Entry Point                        */
    *(--stk)  = (uint32_t)0xFFFFFFFEL;  /* R14 (LR) (So task CANNOT return)   */
    *(--stk)  = (uint32_t)0x12121212L;  /* R12                                */
    *(--stk)  = (uint32_t)0x03030303L;  /* R3                                 */
    *(--stk)  = (uint32_t)0x02020202L;  /* R2                                 */
    *(--stk)  = (uint32_t)0x01010101L;  /* R1                                 */
    *(--stk)  = (uint32_t)0x00000000L;  /* R0 : argument                      */
                                        /* Registers saved on PSP             */
    *(--stk)  = (uint32_t)0x11111111L;  /* R11                                */
    *(--stk)  = (uint32_t)0x10101010L;  /* R10                                */
    *(--stk)  = (uint32_t)0x09090909L;  /* R9                                 */
    *(--stk)  = (uint32_t)0x08080808L;  /* R8                                 */
    *(--stk)  = (uint32_t)0x07070707L;  /* R7                                 */
    *(--stk)  = (uint32_t)0x06060606L;  /* R6                                 */
    *(--stk)  = (uint32_t)0x05050505L;  /* R5                                 */
    *(--stk)  = (uint32_t)0x04040404L;  /* R4                                 */

    OSTCBCur                = &OSTCBTbl[task_num];
    OSTCBCur->OSTCBStkPtr   = stk;        /* Initialize the task's stack      */
    OSTCBCur->OSTCBNext     = &OSTCBTbl[0];
    OSTCBCur->OSTCBWakeTime = 0;

    if( task_num > 0 )
    {
        OSTCBTbl[task_num-1].OSTCBNext = OSTCBCur;
    }

    task_num++;
}

/**
  * @brief  Trigger the PendSV.
  * @param  None
  * @retval None
  */
__inline void __Sched (void) 
{
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

/**
  * @brief  You MUST have created at least one task when you going to call OSStart().
  * @param  None
  * @retval None
  */
__inline void OSStart (void)
{
    __NVIC_SetPriority ( PendSV_IRQn, 0xFF );  /** SCB->SHP[10] = 0xFF;      **/
    __set_PSP(0);
    __Sched();
    __enable_irq();
}

/**
  * @brief  This function is called to delay execution of the currently running 
  *         task until the specified number of system ticks expires.
  * @param  ticks     is the time delay that the task will be suspended.
  * @retval None
  */
__inline void OSTimeDly( uint16_t ticks )
{
    OSTCBCur->OSTCBWakeTime = OSTime_Now() + ticks;
    while( OSTime_Now() < OSTCBCur->OSTCBWakeTime )
    {
        __Sched();
    }
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
__ASM void PendSV_Handler (void)
{
    extern OSTCBCur

    PRESERVE8
    
    CPSID I               /* Prevent interruption during context switch       */
    MRS   R0, PSP         /* PSP is process stack pointer                     */
    CBZ   R0, _nosave     /* Skip register save the first time                */
                          /*                                                  */
    STMDB R0!, {R4-R11}   /* PUSH r4-11 to current process stack              */
                          /*                                                  */
    LDR   R1, =OSTCBCur   /* OSTCBCur->OSTCBStkPtr = PSP;                     */
    LDR   R1, [R1]        /*                                                  */
    STR   R0, [R1]        /* R0 is SP of process being switched out           */
                          /* Now, entire context of process has been saved    */
_nosave                   /*                                                  */
    LDR   R0, =OSTCBCur   /* OSTCBCur  = OSTCBCur->OSTCBNext;                 */
    LDR   R2, [R0]        /*                                                  */
    ADD   R2, R2, #0x04   /*                                                  */
    LDR   R2, [R2]        /*                                                  */
    STR   R2, [R0]        /*                                                  */
                          /*                                                  */
    LDR   R0, [R2]        /* PSP = OSTCBCur->OSTCBStkPtr                      */
    LDMIA R0!, {R4-R11}   /* POP r4-11 from new process stack                 */
                          /*                                                  */
    MSR   PSP, R0         /* Load PSP with new process SP                     */

#if 0 /* Debug code */
    MOVS   R1, #0x00
    STMDB R0!,{R1} 
    STMDB R0!,{R1} 
    STMDB R0!,{R1} 
    STMDB R0!,{R1} 
    STMDB R0!,{R1} 
    STMDB R0!,{R1} 
    STMDB R0!,{R1} 
    STMDB R0!,{R1} 
#endif

    ORR   LR, LR, #0x04   /* Ensure exception return uses process stack       */
    CPSIE I               /*                                                  */
                          /*                                                  */
    BX    LR              /* Exception return will restore remaining context  */

    ALIGN
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
    uint8_t i;
    SysTime++;
    if( SysTime >= 0xFFFF0000 )
    {
        for(i = 0;i < OS_MAX_TASKS;i++)
        {
            if(OSTCBTbl[i].OSTCBWakeTime >= SysTime)
            {
                OSTCBTbl[i].OSTCBWakeTime -= 0xFFFF0000;
            }
        }
        SysTime = 0;
    }
}
/**************** (C) COPYRIGHT 2023 Windy Albert ******** END OF FILE ********/
