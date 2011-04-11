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

#include <list>

using namespace std;

class UThread;

//
// The singleton user threads scheduler.
//

class UScheduler
{
    //
    // The number of existing user threads.
    //

    static int m_numThreads;

    //
    // The currently running thread.
    //

    static UThread *m_pRunningThread;

    //
    // The list of schedulable user threads.
    // The next thread to run is retrieved from the head of the list.
    //

    static list<UThread *> m_readyQueue;

    //
    // The user thread proxy of the main operating system thread. This thread 
    // is switched back in when there are no more runnable user threads and the 
    // scheduler will exit.
    //

    static UThread *m_pMainThread;
public:

    //
    // Initializes the scheduler. The operating system thread that calls the 
    // function switches to a user thread and resumes execution only when all 
    // user threads have exited.
    //

    static void Run();

private:

    //
    // Private constructor.
    //

    UScheduler();

    //
    // Returns and removes the first user thread in the ready queue. 
    // If the ready queue is empty, the main thread is returned.
    //

    static UThread * find_next_thread();

    //
    // UThread instances can access the USchedulers's private state.
    //

    friend class UThread;
};
