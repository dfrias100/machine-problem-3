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
#include <unordered_map>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "reqchannel.hpp"
#include "pcbuffer.hpp"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */ 
/*--------------------------------------------------------------------------*/

/*typedef struct {*/ 
    /* Histogram bins
        0 - 9
        10 - 19
        20 - 29
        30 - 39
        40 - 49
        50 - 59
        60 - 69
        70 - 79
        80 - 89
        90 - 99 */
    /*PCBuffer* PatientDataBuffer;
    vector<int> histogram[10]; 
} PatientHistogram;*/

typedef struct {
    int n_req;
    string patient_name;
    PCBuffer* PCB;
} RTFargs;

typedef struct {
    PCBuffer* PCB;
    RequestChannel* rc;
    /* unordered_map<string, PatientHistogram>* PatientData; */
    pthread_t thread_id;
} WTFargs;

/*typedef struct {
    unordered_map<string, PatientHistogram>* PatientData;
    string patient_name;
} STFargs;*/

Semaphore request_chan_mutex(1);

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

const int NUM_PATIENTS = 100;

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
        request_chan_mutex.P();
        std::string reply = wtfargs->rc->send_request(req);
        request_chan_mutex.V();
	    std::cout << "Reply to request '" << req << "': " << reply << std::endl;

        /* std::string name = reply.substr(5, reply.length() - 1);
        PatientHistogram* patient_histogram = wtfargs->PatientData[name];
        patient_histogram->PatientDataBuffer->deposit(std::stoi(reply); */
    }
    return nullptr;
}

void* stats_thread_func(void* args) {
    for (;;) {
        /* Pseudocode
            string Name = stfargs->patient_name;
            PatientHistogram* hist = &stfargs->hash_table->search(Name);
            if (stfargs->data <= 9)
                hist[0].push_back(stfargs->data);
            ... (do this until <= 99)
        */
    }
}

void create_worker(int _thread_id, RequestChannel* rq, PCBuffer* PCB, WTFargs* args) {
    args[_thread_id].PCB = PCB;
    args[_thread_id].rc = rq;
    std::cout << "Creating new worker thread..." << std::endl;
    pthread_create(&args[_thread_id].thread_id, NULL, worker_thread_func, (void*) &args[_thread_id]);
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char * argv[]) {
    /* TODO: 
        - Implement hashtables for patients
        - Create multiple PCBuffers for stats threads
        - Have worker thread deposit into PCBuffer
    */

    size_t num_requests = 0;
    size_t pcb_size = 0;
    size_t num_threads = 0;
    
    int opt;

    while((opt = getopt(argc, argv, ":n:b:w:")) != -1) {
        switch (opt) {
            case 'n':
                sscanf(optarg, "%zu", &num_requests);
                break;
            case 'b':
                sscanf(optarg, "%zu", &pcb_size);
                break;
            case 'w':
                sscanf(optarg, "%zu", &num_threads);
                break;
            case ':':
                std::cout << "Option requires an argument" << std::endl;
                return 1;
            case '?':
                std::cout << "Unknown argument" << std::endl;
                return 1;
        }
    }

    if (num_requests == 0 || pcb_size == 0 || num_threads == 0) {
        std::cout << "Invalid parameters or no parameters passed. Check your input and start again." << std::endl;
        exit(1);
    }   

    std::cout << "CLIENT STARTED:" << std::endl;

    std::cout << "Establishing control channel... " << std::flush;
    RequestChannel chan("control", RequestChannel::Side::CLIENT);
    std::cout << "done." << std::endl;

    /*std::cout << "Creating hash map..." << std::endl;
    std::unordered_map<std::string, PatientHistogram>* patient_data = new unordered_map<string,PatientHistogram>();
    std::cout << "done." << std::endl;*/

    std::cout << "Creating PCBuffer..." << std::endl;
    PCBuffer* PCB = new PCBuffer(pcb_size);
    if (PCB == 0) {
        std::cout << "PCBuffer creation failed! Exiting..." << std::endl;
        exit(1);
    }
    std::cout << "done." << std::endl;

    pthread_t* rq_threads = new pthread_t[NUM_PATIENTS];
    pthread_t* st_threads = new pthread_t[NUM_PATIENTS];
    RTFargs* rtfargs = new RTFargs[NUM_PATIENTS];
    // STFargs* stfargs = new STFargs[NUM_PATIENTS]; 

    std::cout << "Creating request threads..." << std::endl;
    for (size_t i = 0; i < NUM_PATIENTS; i++) {
        rtfargs[i].n_req = num_requests;
        rtfargs[i].patient_name = "Patient " + std::to_string(i + 1);
        rtfargs[i].PCB = PCB;
        pthread_create(&rq_threads[i], NULL, request_thread_func, (void*) &rtfargs[i]);

        
        /* PCBuffer* stats_buff = new PCBuffer(pcb_size);
        (*patient_data)[rtfargs[i].patient_name].PatientDataBuffer = stats_buff;
        stfargs[i].PatientData = patient_data;
        stfargs[i].patient_name = rtfargs[i].patient_name;
        pthread_create(&st_threads[i], NULL, stats_thread_func, (void*) &stfargs[i]);*/
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
