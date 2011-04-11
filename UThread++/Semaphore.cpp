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
#include "Semaphore.h"

//
// The Mutex destructor.
//

Semaphore::~Semaphore()
{
    assert(m_waitList.empty());
}

//
// Gets one permit from the semaphore. If no permits are available, the calling thread 
// is blocked until a call to Post() adds a permit.
//

void Semaphore::Wait()
{
    UThread &currentThread = UThread::Current();

    //
    // If there are permits available, get one and keep running.
    //

    if (m_permits > 0) {
        m_permits -= 1;
        return;
    }

    //
    // There are no permits available. Insert the running thread in the wait list.
    //

    m_waitList.push_back(&currentThread);

    //
    // Park the current thread. The thread is unparked by a call to Post().
    //

    currentThread.Park();
}

//
// Adds one permit to the semaphore, eventually unblocking a waiting thread.
//

void Semaphore::Post()
{
    UThread &currentThread = UThread::Current();

    if (m_waitList.empty()) {
        m_permits += 1;
        return;
    }

    //
    // Release a blocked thread. The permit is not added to m_permits, instead 
    // being consumed by the blocked thread.
    //

    UThread *thread = m_waitList.front();
    m_waitList.pop_front();

    thread->Unpark();
}
