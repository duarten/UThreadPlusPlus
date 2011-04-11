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

#include "Mutex.h"

//
// The Mutex destructor.
//

Mutex::~Mutex()
{
    assert(m_pOwner == NULL);
    assert(m_waitList.empty());
}

//
// Acquires the specified mutex, blocking the current thread if the mutex is not free.
//

void Mutex::Acquire()
{
    UThread &currentThread = UThread::Current();

    if (m_pOwner == &currentThread) {

        //
        // Recursive aquisition. Increment the recursion counter.
        //

        m_recursionCounter += 1;
    } else if (m_pOwner == NULL) {

        //
        // Mutex is free. Acquire the mutex by setting its owner to the current thread.
        //

        m_pOwner = &currentThread;
        m_recursionCounter = 1;
    } else {

        //
        // Insert the running thread in the wait list.
        //

        m_waitList.push_back(&currentThread);

        //
        // Park the current thread. When the thread is unparked, it will have ownership of the mutex.
        //

        UThread::Park();
        assert(m_pOwner == &currentThread);
    }
}
        
//
// Releases the specified mutex, eventually unblocking a waiting thread to which the
// ownership of the mutex is transfered.
//

void Mutex::Release()
{
    UThread &currentThread = UThread::Current();

    assert(m_pOwner == &currentThread);

    if ((m_recursionCounter -= 1) > 0) {
        
        //
        // The current thread is still the owner of the mutex.
        //

        return;
    }

    if (m_waitList.empty()) {

        //
        // No threads are blocked; the mutex becomes free.
        //

        m_pOwner = NULL;
        return;
    }

    //
    // Get the next blocked thread and transfer mutex ownership to it.
    //

    UThread *thread = m_waitList.front();
    m_waitList.pop_front();

    m_pOwner = thread;
    m_recursionCounter = 1;
        
    //
    // Unpark the thread.
    //

    thread->Unpark();
}
