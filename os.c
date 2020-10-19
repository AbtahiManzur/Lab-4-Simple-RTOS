// os.c
// Runs on LM4F120/TM4C123/MSP432
// A priority/blocking real-time operating system 
// lab 4 file

#include <stdint.h>
#include "os.h"
#include "CortexM.h"
#include "BSP.h"
#include "../inc/msp432p401r.h"

// function definitions in osasm.s
void StartOS(void);

#define NUMTHREADS  8        // maximum number of threads
#define NUMPERIODIC 2        // maximum number of periodic threads
#define STACKSIZE   100      // number of 32-bit words in stack per thread
struct tcb{
  int32_t *sp;       // pointer to stack (valid for threads not running
  struct tcb *next;  // linked-list pointer
	int32_t *block;
	uint32_t sleep;
	uint8_t priority;
};
typedef struct tcb tcbType;
tcbType tcbs[NUMTHREADS];
tcbType *RunPt;
int32_t Stacks[NUMTHREADS][STACKSIZE];
void static runperiodicevents(void);

// ******** OS_Init ************
// Initialize operating system, disable interrupts
// Initialize OS controlled I/O: periodic interrupt, bus clock as fast as possible
// Initialize OS global variables
// Inputs:  none
// Outputs: none
void OS_Init(void){
  DisableInterrupts();
  BSP_Clock_InitFastest();// set processor clock to fastest speed
	BSP_PeriodicTask_Init(runperiodicevents,1000,2);
// perform any initializations needed, 
// set up periodic timer to run runperiodicevents to implement sleeping
}

void SetInitialStack(int i){
	tcbs[i].sp = &Stacks[i][STACKSIZE-16];
	Stacks[i][STACKSIZE-1] = 0x01000000;
	Stacks[i][STACKSIZE-3] = 0x14141414;
	Stacks[i][STACKSIZE-4] = 0x12121212;
	Stacks[i][STACKSIZE-5] = 0x03030303;
	Stacks[i][STACKSIZE-6] = 0x02020202;
	Stacks[i][STACKSIZE-7] = 0x01010101;
	Stacks[i][STACKSIZE-8] = 0x00000000;
	Stacks[i][STACKSIZE-9] = 0x11111111;
	Stacks[i][STACKSIZE-10] = 0x10101010;
	Stacks[i][STACKSIZE-11] = 0x09090909;
	Stacks[i][STACKSIZE-12] = 0x08080808;
	Stacks[i][STACKSIZE-13] = 0x07070707;
	Stacks[i][STACKSIZE-14] = 0x06060606;
	Stacks[i][STACKSIZE-15] = 0x05050505;
	Stacks[i][STACKSIZE-16] = 0x04040404;
}

//******** OS_AddThreads ***************
// Add eight main threads to the scheduler
// Inputs: function pointers to eight void/void main threads
//         priorites for each main thread (0 highest)
// Outputs: 1 if successful, 0 if this thread can not be added
// This function will only be called once, after OS_Init and before OS_Launch
int OS_AddThreads(void(*thread0)(void), uint32_t p0,
                  void(*thread1)(void), uint32_t p1,
                  void(*thread2)(void), uint32_t p2,
                  void(*thread3)(void), uint32_t p3,
                  void(*thread4)(void), uint32_t p4,
                  void(*thread5)(void), uint32_t p5,
                  void(*thread6)(void), uint32_t p6,
                  void(*thread7)(void), uint32_t p7){
										int32_t status;
	status = StartCritical();
	tcbs[0].next = &tcbs[1];
	tcbs[1].next = &tcbs[2];
	tcbs[2].next = &tcbs[3];
  tcbs[3].next = &tcbs[4];
  tcbs[4].next = &tcbs[5];
 	tcbs[5].next = &tcbs[6];										
  tcbs[6].next = &tcbs[7];
  tcbs[7].next = &tcbs[0];										

	tcbs[0].block = 0;
	tcbs[1].block = 0;
	tcbs[2].block = 0;
  tcbs[3].block = 0;
  tcbs[4].block = 0;
  tcbs[5].block = 0;
  tcbs[6].block = 0;
  tcbs[7].block = 0;										
	
	tcbs[0].sleep = 0;
	tcbs[1].sleep = 0;
	tcbs[2].sleep = 0;
  tcbs[3].sleep = 0;
  tcbs[4].sleep = 0;
  tcbs[5].sleep = 0;
	tcbs[6].sleep = 0;
	tcbs[7].sleep = 0;
	
	tcbs[0].priority = p0;
	tcbs[1].priority = p1;
	tcbs[2].priority = p2;
	tcbs[3].priority = p3;
	tcbs[4].priority = p4;
	tcbs[5].priority = p5;
	tcbs[6].priority = p6;
	tcbs[7].priority = p7;
										
	SetInitialStack(0); Stacks[0][STACKSIZE-2] = (int32_t) thread0;
	SetInitialStack(1); Stacks[1][STACKSIZE-2] = (int32_t) thread1;
	SetInitialStack(2); Stacks[2][STACKSIZE-2] = (int32_t) thread2;
	SetInitialStack(3); Stacks[3][STACKSIZE-2] = (int32_t) thread3;
	SetInitialStack(4); Stacks[4][STACKSIZE-2] = (int32_t) thread4;
	SetInitialStack(5); Stacks[5][STACKSIZE-2] = (int32_t) thread5;
	SetInitialStack(6); Stacks[6][STACKSIZE-2] = (int32_t) thread6;
	SetInitialStack(7); Stacks[7][STACKSIZE-2] = (int32_t) thread7;
	
	RunPt = &tcbs[0]; 								 
	EndCritical(status);
  return 1;               // successful
}


void static runperiodicevents(void){
// In Lab 4, handle periodic events in RealTimeEvents
	for(int i = 0;i < NUMTHREADS;i++)
	{
		if(tcbs[i].sleep)
		{
			tcbs[i].sleep--;
		}
	}
}

//******** OS_Launch ***************
// Start the scheduler, enable interrupts
// Inputs: number of clock cycles for each time slice
// Outputs: none (does not return)
// Errors: theTimeSlice must be less than 16,777,216
void OS_Launch(uint32_t theTimeSlice){
  STCTRL = 0;                  // disable SysTick during setup
  STCURRENT = 0;               // any write to current clears it
  SYSPRI3 =(SYSPRI3&0x00FFFFFF)|0xE0000000; // priority 7
  STRELOAD = theTimeSlice - 1; // reload value
  STCTRL = 0x00000007;         // enable, core clock and interrupt arm
  StartOS();                   // start on the first task
}
// runs every ms
void Scheduler(void){      // every time slice
// look at all threads in TCB list choose
// highest priority thread not blocked and not sleeping 
// If there are multiple highest priority (not blocked, not sleeping) run these round robin
	uint8_t max = 255;
	tcbType *BestPt, *pt;
 	pt = RunPt->next;
	for(int i =0; i < NUMTHREADS; i++)
	{
		pt = pt->next;
		if((pt->block == 0) && (pt->sleep == 0) && ((pt->priority) < max))
		{
			max = pt->priority;
			BestPt = pt;
		}
	}
	RunPt = BestPt;
}

//******** OS_Suspend ***************
// Called by main thread to cooperatively suspend operation
// Inputs: none
// Outputs: none
// Will be run again depending on sleep/block status
void OS_Suspend(void){
  STCURRENT = 0;        // any write to current clears it
  INTCTRL = 0x04000000; // trigger SysTick
// next thread gets a full time slice
}

// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(uint32_t sleepTime){
// set sleep parameter in TCB, same as Lab 3
// suspend, stops running
	RunPt->sleep = sleepTime;
	OS_Suspend();
}

// ******** OS_InitSemaphore ************
// Initialize counting semaphore
// Inputs:  pointer to a semaphore
//          initial value of semaphore
// Outputs: none
void OS_InitSemaphore(int32_t *semaPt, int32_t value){
	*semaPt = value;
}

// ******** OS_Wait ************
// Decrement semaphore and block if less than zero
// Lab2 spinlock (does not suspend while spinning)
// Lab3 block if less than zero
// Inputs:  pointer to a counting semaphore
// Outputs: none
void OS_Wait(int32_t *semaPt){
	DisableInterrupts();
	*semaPt -= 1;
	if((*semaPt) < 0) {
		RunPt->block = semaPt;
		EnableInterrupts();
		OS_Suspend();
	}
	EnableInterrupts();
}

// ******** OS_Signal ************
// Increment semaphore
// Lab2 spinlock
// Lab3 wakeup blocked thread if appropriate
// Inputs:  pointer to a counting semaphore
// Outputs: none
void OS_Signal(int32_t *semaPt){
	tcbType *pt;
	DisableInterrupts();
	*semaPt += 1;
	if((*semaPt) <= 0)
	{
		pt = RunPt->next;
		while(pt->block != semaPt)
		{
			pt = pt->next;
		}
		pt->block = 0;
	}
	EnableInterrupts();
}

#define FSIZE 10    // can be any size
uint32_t PutI;      // index of where to put next
uint32_t GetI;      // index of where to get next
uint32_t Fifo[FSIZE];
int32_t CurrentSize;// 0 means FIFO empty, FSIZE means full
uint32_t LostData;  // number of lost pieces of data

// ******** OS_FIFO_Init ************
// Initialize FIFO.  The "put" and "get" indices initially
// are equal, which means that the FIFO is empty.  Also
// initialize semaphores to track properties of the FIFO
// such as size and busy status for Put and Get operations,
// which is important if there are multiple data producers
// or multiple data consumers.
// Inputs:  none
// Outputs: none
void OS_FIFO_Init(void){
	PutI, GetI, LostData = 0;
	OS_InitSemaphore(&CurrentSize, 0);
}

// ******** OS_FIFO_Put ************
// Put an entry in the FIFO.  Consider using a unique
// semaphore to wait on busy status if more than one thread
// is putting data into the FIFO and there is a chance that
// this function may interrupt itself.
// Inputs:  data to be stored
// Outputs: 0 if successful, -1 if the FIFO is full
int OS_FIFO_Put(uint32_t data){
	if(CurrentSize == FSIZE)
	{
		LostData++;
		return -1;
	}
	else 
	{
		Fifo[PutI] = data;
		PutI = (PutI + 1)%FSIZE;
		OS_Signal(&CurrentSize);
		return 0;   // success
	}
}

// ******** OS_FIFO_Get ************
// Get an entry from the FIFO.  Consider using a unique
// semaphore to wait on busy status if more than one thread
// is getting data from the FIFO and there is a chance that
// this function may interrupt itself.
// Inputs:  none
// Outputs: data retrieved
uint32_t OS_FIFO_Get(void){uint32_t data;
	OS_Wait(&CurrentSize);
	data = Fifo[GetI];
	GetI = (GetI + 1)%FSIZE;
  return data;   // success
}
// *****periodic events****************
int32_t *PeriodicSemaphore0;
uint32_t Period0; // time between signals
int32_t *PeriodicSemaphore1;
uint32_t Period1; // time between signals
void RealTimeEvents(void){
  int flag=0;
  static int32_t realCount = -10; // let all the threads execute once
  // Note to students: we had to let the system run for a time so all user threads ran at least one
  // before signalling the periodic tasks
  realCount++;
  if(realCount >= 0){
		if((realCount%Period0)==0){
      OS_Signal(PeriodicSemaphore0);
      flag = 1;
		}
    if((realCount%Period1)==0){
      OS_Signal(PeriodicSemaphore1);
      flag=1;
		}
    if(flag){
      OS_Suspend(); // run the scheduler
    }
  }
}
// ******** OS_PeriodTrigger0_Init ************
// Initialize periodic timer interrupt to signal 
// Inputs:  semaphore to signal
//          period in ms
// priority level at 0 (highest)
// Outputs: none
void OS_PeriodTrigger0_Init(int32_t *semaPt, uint32_t period){
	PeriodicSemaphore0 = semaPt;
	Period0 = period;
	BSP_PeriodicTask_InitC(&RealTimeEvents,1000,0);
}
// ******** OS_PeriodTrigger1_Init ************
// Initialize periodic timer interrupt to signal 
// Inputs:  semaphore to signal
//          period in ms
// priority level at 0 (highest)
// Outputs: none
void OS_PeriodTrigger1_Init(int32_t *semaPt, uint32_t period){
	PeriodicSemaphore1 = semaPt;
	Period1 = period;
	BSP_PeriodicTask_InitC(&RealTimeEvents,1000,0);
}

//****edge-triggered event************
int32_t *edgeSemaphore;
// ******** OS_EdgeTrigger_Init ************
// Initialize button1, P5.1, to signal on a falling edge interrupt
// Inputs:  semaphore to signal
//          priority
// Outputs: none
void OS_EdgeTrigger_Init(int32_t *semaPt, uint8_t priority){
	edgeSemaphore = semaPt;
//***IMPLEMENT THIS***
  // P5.1 input with pullup
  // (c) P5.1 is falling edge event
  // (d) clear flag1 
  // (e) arm interrupt on P5.1
  // (f) priority 
  // (g) enable interrupt 39 in NVIC
	*edgeSemaphore = 0;
	P5SEL1 &= ~(1<<1);
	P5SEL0 &= ~(1<<1);
	P5DIR &= ~(1<<1);
	P5REN |= (1<<1);
	P5OUT |= (1<<1);
	P5IES |= (1<<1);
	P5IFG &= ~(1<<1);
	P5IE |= (1<<1);
	NVIC_IPR9 &= ~((uint32_t) 7<<29); 
	NVIC_IPR9 |= (uint32_t)(priority<<29);
	NVIC_ISER1 |= (1<<7);
}

// ******** OS_EdgeTrigger_Restart ************
// restart button1 to signal on a falling edge interrupt
// rearm interrupt
// Inputs:  none
// Outputs: none
void OS_EdgeTrigger_Restart(void){
//***IMPLEMENT THIS***
  // (g) enable interrupt 39 in NVIC
  // (d) clear flag1 
	P5IE |= (1<<1);
	P5IFG &= ~(1<<1);
}
void PORT5_IRQHandler(void){
//***IMPLEMENT THIS***
  // step 1 acknowledge by clearing flag
  // step 2 signal semaphore (no need to run scheduler)
  // step 3 disarm interrupt to prevent bouncing to create multiple signals
	P5IFG &= ~(1<<1);
	OS_Signal(edgeSemaphore);
	P5IE &= ~(1<<1);
}


