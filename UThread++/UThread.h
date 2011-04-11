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

#pragma once

class UScheduler;

//
// The representation of a user thread.
//

class UThread 
{
public:

    typedef void *Argument;
    typedef void (*Function)(Argument);

private:
    
    //
    // The data structure representing the layout of a thread's execution 
    // context when saved in the thread's stack.
    //

    struct Context
    {
        unsigned EDI;
        unsigned ESI;
        unsigned EBX;
        unsigned EBP;
        void (*Ret)();

        //
        // Set the thread's initial context by initializing the values of EDI, EBX, ESI 
        // and EBP (must be zero for Visual Studio to correctly present a thread's call stack)
        // and by hooking the return address. Upon the first context switch to this thread, 
        // after popping the dummy values of the "saved" registers, a ret instruction will 
        // place InternalStart's address on the processor's IP.
        //

        Context() 
            : EBX(0x11111111),
              ESI(0x22222222),
              EDI(0x33333333),
              EBP(0x00000000),
              Ret(trampoline)
        { }
    };

    //
    // The fixed stack size for a user thread.
    //

    static const int m_stackSize = 16 * 4096;

    //
    // The thread id.
    //

    int m_threadId;

    //
    // The memory block used as the thread's stack.
    //

    unsigned char *m_pStack;

    //
    // A pointer to the thread's context stored in its stack.
    //

    Context *m_pContext;

    //
    // The thread's starting function and argument.
    //
        
    Function m_pFunction;
    Argument m_argument;

public:
        
    //
    // Creates a user thread to run the specified function. 
    // The new thread is placed at the end of the ready queue.
    //

    static void Create(Function function, Argument argument);
        
    //
    // Relinquishes the processor to the first user thread in the ready queue. 
    // If there are no ready threads, the function returns immediately.
    //
        
    static void Yield();

    //
    // Terminates the execution of the currently running thread. All associated 
    // resources will be freed after a context switch to the next ready thread. If 
    // there are no threads in the ready queue, then the main thread is switched in 
    // and the scheduler will exit.
    //

    __declspec(noreturn) static void Exit();
     
    //
    // Returns the UThread instance representing the currently executing thread.
    //

    static UThread & Current();

    //
    // Halts the execution of the current user thread.
    //

    static void Park();

    //
    // Places the UThread instance in the ready queue, making the user thread eligible to run.
    //

    void Unpark();

    //
    // Returns the thread's id.
    //

    int GetId() const
    {
        return m_threadId;
    }
        
private:

    //
    // Creates a UThread instance without allocating memory for the stack.
    //

    UThread();
        
    //
    // Creates a UThread instance.
    //

    UThread(Function function, Argument argument);

    //
    // A private copy construtor used to prohibit copies. It has no definition.
    //

    UThread(const UThread &);
        
    //
    // The UThread destructor.
    //

    ~UThread();

    //
    // A private assign operator used to prohibit copies. It has no definition.
    //

    UThread & operator =(const UThread &);

    //
    // The function that a user thread begins by executing, through which 
    // the associated function is called.
    //

    static void trampoline();

    //
    // Helper function called by assembly code to proxy the application of delete.
    //

    void self_destroy()
    {
        delete this;
    }

    //
    // UScheduler can access the private state of an UThread instance.
    //

    friend class UScheduler;
};
