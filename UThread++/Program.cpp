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
#include <iostream>
#include "UScheduler.h"
#include "UThread.h"
#include "Mutex.h"
#include "Semaphore.h"

using namespace std;

///////////////////////////////////////////////////////////////
//															 //
// Test 1: 10 threads, each one printing its number 16 times //
//															 //
///////////////////////////////////////////////////////////////

unsigned int test1_count;

void test1_thread(UThread::Argument arg) 
{
    char c = (char) arg;

    for (int i = 0; i < 16; ++i) {
        cout << c;

        if ((rand() % 4) == 0) {
            UThread::Yield();
        }
    }

    ++test1_count;
    
    UThread::Exit();
}

void test1() {
    cout << endl << ":: Test 1 - BEGIN ::" << endl << endl;

    test1_count = 0; 

    for (int i = 0; i < 10; ++i) {
        UThread::Create(test1_thread, (UThread::Argument) ('0' + i));
    }

    UScheduler::Run();

    assert(test1_count == 10);
    cout << endl << ":: Test 1 - END ::" << endl << endl;
}

///////////////////////////////////////////////////////////////
//															 //
// Test 2: Testing mutexes									 //
//															 //
///////////////////////////////////////////////////////////////
                                
unsigned int test2_count;		

void test2_thread1(UThread::Argument arg) {
    Mutex *mutex = (Mutex *) arg;

    cout << "UThread 1 running" << endl << "UThread 1 acquiring the mutex..." << endl;
    mutex->Acquire();	
    cout << "UThread 1 acquired the mutex..." << endl;
    
    UThread::Yield();

    cout << "UThread 1 acquiring the mutex again..." << endl;
    mutex->Acquire();
    cout << "UThread 1 acquired the mutex again..." << endl;

    UThread::Yield();

    cout << "UThread 1 releasing the mutex..." << endl;
    mutex->Release();
    cout << "UThread 1 released the mutex..." << endl;
        
    UThread::Yield();

    cout << "UThread 1 releasing the mutex again..." << endl;
    mutex->Release();
    cout << "UThread 1 released the mutex again..." << endl << "UThread 1 exiting" << endl;
    
    ++test2_count;
}

void test2_thread2(UThread::Argument arg) {
    Mutex *mutex = (Mutex *) arg;

    cout << "UThread 2 running" << endl << "UThread 2 acquiring the mutex..." << endl;
    mutex->Acquire();	
    cout << "UThread 2 acquired the mutex..." << endl;
    
    UThread::Yield();

    cout << "UThread 2 releasing the mutex..." << endl;
    mutex->Release();
    cout << "UThread 2 released the mutex..." << endl << "UThread 2 exiting" << endl;
    
    ++test2_count;
}

void test2_thread3(UThread::Argument arg) {
    Mutex *mutex = (Mutex *) arg;

    cout << "UThread 3 running" << endl << "UThread 3 acquiring the mutex..." << endl;
    mutex->Acquire();	
    cout << "UThread 3 acquired the mutex..." << endl;

    UThread::Yield();

    cout << "UThread 3 releasing the mutex..." << endl;
    mutex->Release();
    cout << "UThread 3 released the mutex..." << endl << "UThread 3 exiting" << endl;
    
    ++test2_count;
}

void test2() {
    Mutex mutex;

    cout << endl << ":: Test 2 - BEGIN ::" << endl << endl;

    test2_count = 0;

    UThread::Create(test2_thread1, &mutex);
    UThread::Create(test2_thread2, &mutex);
    UThread::Create(test2_thread3, &mutex);
    UScheduler::Run();
    
    cout << endl << ":: Test 2 - END ::" << endl << endl;
    
    assert(test2_count == 3);
}

///////////////////////////////////////////////////////////////
//															 //
// Test 3: building a mailbox with a mutex and a semaphore   //
//															 //
///////////////////////////////////////////////////////////////

//
// Mailbox containing message queue, a lock to ensure exclusive access 
// and a semaphore to control the message queue.
//

template <typename T>
class Mailbox 
{
    Mutex m_lock;
    Semaphore m_semaphore;
    list<T *> m_messageQueue;

public:
    void Post(T *data);
    T * Wait();
};

template <typename T>
void Mailbox<T>::Post(T *data)
{
    assert(data != NULL);

    m_lock.Acquire();

    //
    // Insert the message in the mailbox queue.
    //

    m_messageQueue.push_back(data);
    //cout << "** enqueued: " << data << " **" << endl;
    
    m_lock.Release();
    
    //
    // Add one permit to indicate the availability of one more message.
    // 

    m_semaphore.Post();
}

template <typename T>
T * Mailbox<T>::Wait()
{
    T *data;
    
    //
    // Wait for a message to be available in the mailbox.
    //

    m_semaphore.Wait();
    
    //
    // Get the envelope from the mailbox queue.
    //

    m_lock.Acquire();
    
    UThread::Yield();
    
    data = m_messageQueue.front();
    m_messageQueue.pop_front();
    
    //cout << "** dequeued: " << data << " **" << endl;
    
    m_lock.Release();
    
    return data;
}

unsigned int test3_countp;
unsigned int test3_countc;

void test3_producer_thread(UThread::Argument arg) {
    static unsigned int current_id = 0;
    unsigned int producer_id = ++current_id;
        
    Mailbox<char>* mailbox = (Mailbox<char>*) arg;
    
    for (int msg_num = 0; msg_num < 5000; ++msg_num) {
        char *msg = (char *) malloc(64);
        sprintf_s(msg, 64, "Message %04d from producer %d", msg_num, producer_id);

        cout << " ** producer " << producer_id << ": sending message " << msg_num << " " << (void *)msg << endl;

        mailbox->Post(msg);

        if ((rand() % 2) == 0) {
            UThread::Yield();
        };
    }
    
    ++test3_countp;
}

void test3_consumer_thread(UThread::Argument arg) {
    static unsigned int current_id = 0;
    unsigned int consumer_id = ++current_id;

    Mailbox<char>* mailbox = (Mailbox<char>*) arg;
    unsigned int num_msgs = 0;

    do {

        //
        // Get a message from the mailbox.
        //

        char *msg = mailbox->Wait();

        if (msg != (char *)-1) {
            ++num_msgs;

            cout << " ** consumer " << consumer_id << ": got " << msg << endl;

            //
            // Free the memory used by the message.
            //

            free(msg);
        } else {
            cout << "++ consumer " << consumer_id << ": exiting after " << num_msgs << " messages" << endl;
            break;
        }
    } while (true);
    
    ++test3_countc;
}

void test3_first_thread(UThread::Argument arg)
{
    Mailbox<char> *mailbox = (Mailbox<char> *)arg;

    test3_countc = 0; 
    test3_countp = 0; 

    UThread::Create(test3_consumer_thread, mailbox);
    UThread::Create(test3_consumer_thread, mailbox);
    UThread::Create(test3_producer_thread, mailbox);
    UThread::Create(test3_producer_thread, mailbox);
    UThread::Create(test3_producer_thread, mailbox);
    UThread::Create(test3_producer_thread, mailbox);

    while (test3_countp != 4) {
        UThread::Yield();
    }

    mailbox->Post((char *)-1); 
    mailbox->Post((char *)-1); 

    while (test3_countc != 2) {
        UThread::Yield();
    }
}

void test3() 
{
    Mailbox<char> mailbox;

    cout << endl << ":: Test 3 - BEGIN ::" << endl << endl;

    UThread::Create(test3_first_thread, &mailbox);
    UScheduler::Run();

    cout << endl << endl << ":: Test 3 - END ::" << endl;
}

int main (
    )
{
    test1();
    test2();
    test3();

    getchar();
    return 0;
}
