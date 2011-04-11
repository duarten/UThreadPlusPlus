///////////////////////////////////////////////////////////
//
// CCISEL 
// 2007-2010
//
// UThread library:
//     User threads supporting cooperative multithreading.
//     The current version of the library provides:
//        - Threads
//        - Mutexes
//        - Semaphores
//
// Authors: Carlos Martins, Joao Trindade, Duarte Nunes
// 
//

#include <cassert>
#include <cstdlib>
#include <list>
#include "UScheduler.h"
#include "UThread.h"

using namespace std;

//
// An oversimplified unique ID generator seed.
//

static int m_threadIdSeed = 0;

//
// The number of existing user threads.
//

int UScheduler::m_numThreads = 0;

//
// The currently running thread.
//

UThread * UScheduler::m_pRunningThread = NULL;

//
// The list of schedulable user threads.
// The next thread to run is retrieved from the head of the list.
//

list<UThread *> UScheduler::m_readyQueue;

//
// The user thread proxy of the main operating system thread. This thread 
// is switched back in when there are no more runnable user threads and the 
// scheduler will exit.
//

UThread * UScheduler::m_pMainThread;

//
// Forward declaration of the helper functions.
//

//
// Performs a context switch from currentThread (switch out) to nextThread (switch in).
// __fastcall sets the calling convention such that currentThread is in ECX and  
// nextThread in EDX.
//

void __fastcall context_switch(UThread *currentThread, UThread *nextThread);

//
// Frees the resources associated with currentThread and switches to nextThread.
// __fastcall sets the calling convention such that currentThread is in ECX and  
// nextThread in EDX.
//

void __fastcall internal_exit(UThread *currentThread, UThread *nextThread);

//
// Definition of the UScheduler and UThread member functions.
//

//
// Runs the scheduler. The operating system thread that calls the function 
// switches to a user thread and resumes execution only when all user threads 
// have exited.
//

void UScheduler::Run()
{
    //
    // There can be only one scheduler instance running.
    //

    assert(m_pRunningThread == NULL);

    if (m_readyQueue.empty()) {
        return;
    }

    //
    // Create the proxy for the underyling operating system thread. This instance 
    // will not allocate space for the thread's stack.
    //

    UThread mainThread;
    m_pMainThread = &mainThread;

    //
    // Switch to a user thread.
    //
    
    context_switch(&mainThread, find_next_thread());

    //
    // When we get here, there are no more runnable user threads (although there 
    // might be threads blocked on synchronizers).
    //

    assert(m_readyQueue.empty());

    //
    // Allow another call to UScheduler::Run().
    //

    m_pRunningThread = NULL;
}

//
// Returns and removes the first user thread in the ready queue. 
// If the ready queue is empty, the main thread is returned.
//

UThread * UScheduler::find_next_thread() 
{
    UThread *nextThread;

    if (!UScheduler::m_readyQueue.empty()) {
        nextThread = UScheduler::m_readyQueue.front();
        UScheduler::m_readyQueue.pop_front();
    } else {
        nextThread = UScheduler::m_pMainThread;
    }

    return nextThread;
}

//
// Creates a UThread instance without allocating memory for the stack.
//

UThread::UThread() 
{
    UScheduler::m_numThreads += 1;
    m_threadId = ++m_threadIdSeed;
    m_pStack = NULL;
}

//
// Creates a UThread instance.
//

UThread::UThread(Function function, Argument argument) 
    : m_pFunction(function),
      m_argument(argument)
{
    UScheduler::m_numThreads += 1;
    m_threadId = ++m_threadIdSeed;

    m_pStack = new unsigned char[m_stackSize];

    //
    // Initialize the stack to zeros for emotional confort.
    //

    memset(m_pStack, 0, m_stackSize);
            
    //
    // Map a UThread::Context on the thread's stack.
    // We'll use it to save the initial context of the thread.
    //
    // +--------------+
    // |  0x00000000  |    <- Highest word of a thread's stack space
    // +==============+       (needs to be set to 0 for Visual Studio to
    // | Context::Ret | \     correctly present a thread's call stack).
    // +--------------+  |
    // | Context::EBP |  |
    // +--------------+  |
    // | Context::EBX |   >   UThread::Context mapped on the stack.
    // +--------------+  |
    // | Context::ESI |  |
    // +--------------+  |
    // | Context::EDI | /  <- Stack pointer will be set to this address
    // +==============+       at the next context switch to this thread.
    // |              | \
    // +--------------+  |
    // |       :      |  |
    //         :          >   Remaining stack space.
    // |       :      |  |
    // +--------------+  |
    // |              | /  <- Lowest word of a thread's stack space
    // +--------------+       (m_pStack always points to this location).
    //
            
    m_pContext = new (m_pStack + m_stackSize - sizeof(unsigned) -
                      sizeof(Context)) UThread::Context;	
}

//
// The UThread destructor.
//

UThread::~UThread()
{
    UScheduler::m_numThreads -= 1;

    //
    // Deletes the stack space. Note that m_pStack may be null.
    //

    delete[] m_pStack;
}

//
// Creates a user thread to run the specified function. 
// The thread is placed at the end of the ready queue.
//

void UThread::Create(Function function, Argument argument)
{
    (new UThread(function, argument))->Unpark();
}

//
// Relinquishes the processor to the first thread in the ready queue. 
// If there are no ready threads, the function returns immediately.
//

void UThread::Yield()
{
    if (!UScheduler::m_readyQueue.empty()) {
        
        //
        // Place the current thread at the end of the ready queue, 
        // so it can resume execution later on.
        //

        UScheduler::m_readyQueue.push_back(UScheduler::m_pRunningThread);

        //
        // Remove the first thread in the ready queue and switch it in.
        //
        
        UThread *nextThread = UScheduler::m_readyQueue.front();
        UScheduler::m_readyQueue.pop_front();
        context_switch(UScheduler::m_pRunningThread, nextThread);
    }
}

//
// Terminates the execution of the currently running thread. All associated 
// resources will be freed after a context switch to the next ready thread. If 
// there are no threads in the ready queue, then the main thread is switched in 
// and the scheduler will exit.
//

__declspec(noreturn) void UThread::Exit()
{
    internal_exit(UScheduler::m_pRunningThread, UScheduler::find_next_thread());
    assert(!"supposed to be here!");
}

//
// Returns the UThread instance representing the currently executing thread.
//

UThread & UThread::Current() 
{
    assert(UScheduler::m_pRunningThread != NULL);
    return *UScheduler::m_pRunningThread;
}

//
// Halts the execution of the current user thread.
//

void UThread::Park()
{
    assert(UScheduler::m_pRunningThread != NULL);
    context_switch(UScheduler::m_pRunningThread, UScheduler::find_next_thread());
}

//
// Places the UThread instance in the ready queue, making the user thread eligible to run.
//

void UThread::Unpark()
{
    UScheduler::m_readyQueue.push_back(this);
}

//
// The function that a user thread begins by executing, through which 
// the associated function is called.
//

void UThread::trampoline()
{
    UThread *currentThread = UScheduler::m_pRunningThread;
    currentThread->m_pFunction(currentThread->m_argument);
    Exit();
}

//
// Performs a context switch from currentThread (switch out) to nextThread (switch in).
// __fastcall sets the calling convention such that currentThread is in ECX and  
// nextThread in EDX.
//

__declspec(naked) void __fastcall context_switch(UThread *currentThread, UThread *nextThread)
{
    __asm {

        //
        // Switch out the running currentThread, saving the execution context on 
        // the thread's own stack. The return address is atop the stack, having 
        // been placed there by the call to this function.
        //

        push    ebp
        push    ebx
        push    esi
        push    edi

        //
        // Save ESP in currentThread->m_pContext.
        //

        mov     dword ptr [ecx + UThread::m_pContext], esp

        //
        // Set nextThread as the running thread.
        //

        mov     UScheduler::m_pRunningThread, edx		

        //
        // Load nextThread's context, starting by switching to its stack,  
        // where the registers are saved.
        //

        mov     esp, dword ptr [edx + UThread::m_pContext]
        
        pop     edi
        pop     esi
        pop     ebx
        pop     ebp

        //
        // Jump to the return address saved on nextThread's stack when 
        // the function was called.
        //

        ret
    }
}

//
// Frees the resources associated with currentThread and switches to nextThread.
// __fastcall sets the calling convention such that currentThread is in ECX and  
// nextThread in EDX.
//

__declspec(naked) void __fastcall internal_exit(UThread *currentThread, UThread *nextThread)
{
    __asm {

        //
        // Set nextThread as the running thread.
        //

        mov     UScheduler::m_pRunningThread, edx

        //
        // Load nextThread's stack pointer before calling UThread::self_destroy(): 
        // making the call while using currentThread's stack would mean using the 
        // same memory being freed -- the stack.
        //

        mov     esp, dword ptr [edx + UThread::m_pContext]

        call    UThread::self_destroy

        //
        // Finish switching in nextThread.
        //

        pop     edi
        pop     esi
        pop     ebx
        pop     ebp

        ret
    }
}
