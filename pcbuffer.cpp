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

PCBuffer::PCBuffer(int _size) : size(_size) {
    m = PTHREAD_MUTEX_INITIALIZER;    
    notfull = PTHREAD_COND_INITIALIZER;
    notempty = PTHREAD_CONT_INITIALIZER;
    buffer = new String[size];
    nextin = 0;
    nextout = 0;
    count = 0;
}

PCBuffer::~PCBuffer() {
    delete[] buffer;
}

int PCBuffer::Deposit(string _item) {
    pthread_mutex_lock(&m);
    while (count == size)
        pthread_cont_wait(&notfull, &m)
    buffer[nextin] = _item;
    nextin = (nextin + 1) % size;
    pthread_cond_signal(&notempty);
    pthread_mutex_unlock(&m);
    return count;
}

string PCBuffer::Retrieve() {
    pthread_mutex_lock(&m);
    while (count == 0)
        pthread_cond_wait(&notempty, &m);
    string ret = buffer[nextout];
    nextout = (nextout + 1) % size;
    pthread_cond_signal(&notfull);
    pthread_mutex_unlock(&m);
}