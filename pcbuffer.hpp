/* 
    File: pcbuffer.hpp

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 2020/09/27

*/

#ifndef _pcbuffer_H_                   // include file only once
#define _pcbuffer_H_

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include <string>
#include <pthread.h>

using namespace std;
/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */ 
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CLASS   bounded P C B u f f e r  */
/*--------------------------------------------------------------------------*/

class PCBuffer {

private:
  /* -- INTERNAL DATA STRUCTURES
     You will need to change them to fit your implementation. */

  string        * buffer; // We buffer the data in an array of strings. 
                          // You may instead prefer a vector, or a queue, or ...
  int             size;   // Size of the bounded buffer.
 
public:

  /* -- CONSTRUCTOR/DESTRUCTOR */

  PCBuffer(int _size) : size(_size);

  ~PCBuffer();
  
  /* -- OPERATIONS ON PC BUFFER */

  int Deposit(string _item);

  string Retrieve();

};


#endif


