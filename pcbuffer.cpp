/* 
    File: pcbuffer.cpp

    Author: Daniel Frias
            Department of Computer Science
            Texas A&M University
    Date  : 2020/09/30

    Source code for the PCBuffer class.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "pcbuffer.hpp"

/*--------------------------------------------------------------------------*/
/* NAME SPACES */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FUNCTIONS FOR CLASS P C B u f f e r */
/*--------------------------------------------------------------------------*/

PCBuffer::PCBuffer(int _size) : size(_size), empty(_size), full(0), mutex(1) {
    //mutex = PTHREAD_MUTEX_INITIALIZER;
    //notfull = PTHREAD_COND_INITIALIZER;
    //notempty = PTHREAD_COND_INITIALIZER;
    //empty = Semaphore(size);
    //full = Semaphore(0);
    //mutex = Semaphore(1);
    buffer = new std::string[size];
    nextin = 0;
    nextout = 0;
    count = 0;
}

PCBuffer::~PCBuffer() {
    delete[] buffer;
}

int PCBuffer::Deposit(std::string _item) {
    //pthread_mutex_lock(&m);
    //while (count == size)
    //    pthread_cond_wait(&notempty, &m);
    empty.P();
    mutex.P();
    //pthread_mutex_lock(&mutex);
    buffer[nextin] = _item;
    nextin = (nextin + 1) % size;
    //pthread_mutex_unlock(&mutex);
    mutex.V();
    full.V();
    //pthread_mutex_unlock(&m);
    //pthread_cond_signal(&notfull);
    return 0;
}


std::string PCBuffer::Retrieve() {
    //while (count == 0)
    //    pthread_cond_wait(&notempty, &m);
    //pthread_mutex_lock(&m);
    full.P();
    mutex.P();
    //pthread_mutex_lock(&mutex);
    std::string ret = buffer[nextout];
    nextout = (nextout + 1) % size;
    mutex.V();
    empty.V();
    //pthread_mutex_unlock(&m);
    //pthread_cond_signal(&notfull);
    return ret;
}
