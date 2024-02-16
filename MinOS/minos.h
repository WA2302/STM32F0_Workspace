/**
  ******************************************************************************
  * @file    minos.h 
  * @author  Windy Albert
  * @version V2.1
  * @date    04-November-2023
  * @brief   This is the head file for MinOS, a NO GRAB OS for embedded system.
  *          TASK_REG_LEVEL_x()        @USAGE TASK_REG_LEVEL_3(Demo1_task, 0x80);
  *          void OSStart( void );     @USAGE OSStart( );
  *          void OSTimeDly( ticks );  @USAGE OSTimeDly( 1000 );
  *          OSWait( con )             @USAGE OSWait( var >= 1 );
  *          OSWaitTime( con,time );   @USAGE OSWaitTime( var >= 1,1000 );
  *          OSisTimeOut()             @USAGE if( OSisTimeOut() ){ //do sth }  
  ******************************************************************************
  */

#ifndef _MINOS_H
#define _MINOS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx.h"
#include "minos_conf.h"

typedef struct os_tcb {
    uint32_t *OSTCBStkPtr;           /* Pointer to current top of stack       */
    struct os_tcb *OSTCBNext;        /* Pointer to next TCB in the TCB list   */
    uint64_t OSTCBWakeTime;          /* Next time to run this task            */
} OS_TCB;

extern void TaskCreate (void (*task)(void), uint32_t *ptos);
extern void __Sched (void);

extern uint64_t         SysTime;
extern OS_TCB          *OSTCBCur;
#define OSTime_Now()    SysTime

/******************************************************************************/
/* Public functions ----------------------------------------------------------*/
/******************************************************************************/  
/* Macro to register task function */
typedef void (*task_fnc_t) (void);
typedef struct {
    task_fnc_t task_fnc;
    uint32_t *p_Stack;
} task_t;
#define TASK_REG(name,stk,level)                                               \
                static void task_##name(void) __attribute__((__used__));       \
                static uint32_t stack_##name[stk] __attribute__((__used__));   \
                const static task_t task_tbl_##name __attribute__((__used__))  \
                __attribute__((__section__("_task_level." #level))) = {        \
                    .task_fnc = task_##name,                                   \
                    .p_Stack = &stack_##name[stk-1]                            \
                };                                                             \
                static void task_##name(void)
                              
#define TASK_REG_LEVEL_0(name,stk) TASK_REG(name,stk,0)
#define TASK_REG_LEVEL_1(name,stk) TASK_REG(name,stk,1)
#define TASK_REG_LEVEL_2(name,stk) TASK_REG(name,stk,2)
#define TASK_REG_LEVEL_3(name,stk) TASK_REG(name,stk,3)
        
extern void OSStart( void );
extern void OSTimeDly( uint16_t ticks );

#define OSWait( con )  while(!(con)){__Sched();}

#define OSWaitTime( con,time )                                      \
                { OSTCBCur->OSTCBWakeTime = OSTime_Now() + time;    \
                  while(!(con) ){                                   \
                    if( OSTime_Now()<OSTCBCur->OSTCBWakeTime ){     \
                        __Sched();                                  \
                    }else{                                          \
                        OSTCBCur->OSTCBWakeTime = 0;break;          \
                    }                                               \
                  }                                                 \
                }
#define OSisTimeOut() (OSTCBCur->OSTCBWakeTime==0)

                
#ifdef __cplusplus
}
#endif

#endif /* _MINOS_H */

/************* (C) COPYRIGHT 2023 Windy Albert *********** END OF FILE ********/
