/* 
    File: simpleclient.C

    Author: R. Bettati
            Department of Computer Science
            Texas A&M University
    Date  : 

    Simple client main program for MPs in CSCE 313
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include <cassert>
#include <cstring>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "reqchannel.hpp"
#include "pcbuffer.hpp"
#include "semaphore.hpp"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

 typedef struct {
     int n_req;
     string patient_name;
     PCBuffer* PCB;
 } RTFargs;

typedef struct {
    PCBuffer* PCB;
    RequestChannel* rc;
    pthread_t thread_id;
} WTFargs;

Semaphore s(1);

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS -- SUPPORT FUNCTIONS */
/*--------------------------------------------------------------------------*/

void* request_thread_func(void* args) {
    RTFargs* rtfargs = (RTFargs*) args;
    for (size_t i = 0; i < rtfargs->n_req; i++) {
        std::string req = "data " + rtfargs->patient_name;
        std::cout << "Depositing request..." << std::endl;
        rtfargs->PCB->Deposit(req);
    }
    return nullptr;
}

void* worker_thread_func(void* args) {
    WTFargs* wtfargs = (WTFargs*) args;
    for(;;) {
        std::string req = wtfargs->PCB->Retrieve();
        std::cout << "New request: " << req << std::endl;
        std::string reply = wtfargs->rc->send_request(req);
        std::cout << "Reply to request '" << req << "': " << reply << std::endl;
    }
    return nullptr;
}

void create_worker(int _thread_id, RequestChannel* rq, PCBuffer* PCB, WTFargs* args) {
    args[_thread_id].PCB = PCB;
    args[_thread_id].rc = rq;
    std::cout << "Creating new worker thread..." << std::endl;
    pthread_create(&args[_thread_id].thread_id, NULL, worker_thread_func, (void*) &args[_thread_id]);
    pthread_join(args[_thread_id].thread_id, NULL);
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char * argv[]) {

    std::cout << "CLIENT STARTED:" << std::endl;

    std::cout << "Establishing control channel... " << std::flush;
    RequestChannel chan("control", RequestChannel::Side::CLIENT);
    std::cout << "done." << std::endl;

    size_t num_threads = 5;

    std::cout << "Creating PCBuffer..." << std::endl;
    PCBuffer* PCB = new PCBuffer(10*40);
    if (PCB == 0) {
        std::cout << "PCBuffer creation failed! Exiting..." << std::endl;
        exit(-1);
    }
    std::cout << "done." << std::endl;

    pthread_t* rq_threads = new pthread_t[40];
    RTFargs* rtfargs = new RTFargs[40];

    std::cout << "Creating request threads..." << std::endl;
    for (size_t i = 0; i < 40; i++) {
        rtfargs[i].n_req = 10;
        rtfargs[i].patient_name = "Patient " + std::to_string(i + 1);
        rtfargs[i].PCB = PCB;
        pthread_create(&rq_threads[i], NULL, request_thread_func, (void*) &rtfargs[i]);
        pthread_join(rq_threads[i], NULL);
    }
    std::cout << "done." << std::endl;
 
    WTFargs* wtfargs = new WTFargs[num_threads];

    for (size_t i = 0; i < num_threads; i++) {
        string reply = chan.send_request("newthread");
        std::cout << "Reply to request 'newthread' is " << reply << std::endl;
        std::cout << "Establishing new control channel... " << std::flush;
        RequestChannel* new_chan = new RequestChannel(reply, RequestChannel::Side::CLIENT);
        std::cout << "done." << std::endl;
        create_worker(i, new_chan, PCB, wtfargs);
    }
    
    usleep(1000000);
    exit(0);
}
