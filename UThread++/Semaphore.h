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

#include <cstdlib>
#include <list>
#include "UThread.h"

using namespace std;

class Semaphore
{
    //
    // The number of permits.
    //

    int m_permits;
        
    //
    // The wait list containing the blocked threads that are waiting on the semaphore.
    //

    list<UThread *> m_waitList;
        
public:
        
    //
    // Creates a Semaphore instance.
    //

    Semaphore()
        : m_permits(0),
          m_waitList()
    { }

    //
    // The Semaphore destructor.
    //

    ~Semaphore();

    //
    // Gets one permit from the semaphore. If no permits are available, the calling
    // thread is blocked until a call to Post() adds a permit.
    //

    void Wait();

    //
    // Adds one permit to the semaphore, eventually unblocking a waiting thread.
    //

    void Post();
};
