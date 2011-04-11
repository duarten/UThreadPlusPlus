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

#include "UThread.h"

using namespace std;

class Mutex
{
    //
    // The thread that currently owns the Mutex instance. If NULL, then the mutex is free.
    //

    UThread *m_pOwner;

    //
    // The number of recursive acquires by the Owner thread.
    //

    int m_recursionCounter;
        
    //
    // The wait list containing the blocked threads that are waiting on the mutex.
    //

    list<UThread *> m_waitList;
        
public:
        
    //
    // Creates a Mutex instance.
    //

    Mutex()
        : m_recursionCounter(0),
          m_pOwner(NULL),
          m_waitList()
    { }

    //
    // The Mutex destructor.
    //

    ~Mutex();

    //
    // Acquires the specified mutex, blocking the current thread if the mutex is not free.
    //

    void Acquire();

    //
    // Releases the specified mutex, eventually unblocking a waiting thread to which the
    // ownership of the mutex is transfered.
    //

    void Release();
};
