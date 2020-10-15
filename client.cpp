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

typedef struct {
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
    PCBuffer* PatientDataBuffer;
    std::vector<int> histogram[10]; 
} PatientHistogram;

typedef struct {
    int* n_req_threads;
    int n_req;
    std::string patient_name;
    PCBuffer* PCB;
} RTFargs;

typedef struct {
    int* n_wkr_threads;
    PCBuffer* PCB;
    RequestChannel* rc;
    std::unordered_map<std::string, PatientHistogram>* PatientData;
} WTFargs;

typedef struct {
    int* n_stt_threads;
    std::unordered_map<std::string, PatientHistogram>* PatientData;
    std::string patient_name;
} STFargs;

Semaphore request_chan_mutex(1);
Semaphore n_req_thread_count_mutex(1);
Semaphore n_wkr_thread_count_mutex(1);
Semaphore histogram_print_sync(1);

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

const int NUM_PATIENTS = 5;

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
    n_req_thread_count_mutex.P();
    if (*(rtfargs->n_req_threads) != 1)
        *(rtfargs->n_req_threads) = *(rtfargs->n_req_threads) - 1;
    else {
        std::cout << "Depositing 'done'" << std::endl;
        rtfargs->PCB->Deposit("done");
        *(rtfargs->n_req_threads) = *(rtfargs->n_req_threads) - 1;
    }
    n_req_thread_count_mutex.V();
    std::cout << "Request thread finished." << std::endl;
    pthread_detach(pthread_self());
    return nullptr;
}

void* worker_thread_func(void* args) {
    WTFargs* wtfargs = (WTFargs*) args;
    for(;;) {
        std::string req = wtfargs->PCB->Retrieve();
        n_wkr_thread_count_mutex.P();
        if (req.compare("done") == 0) {
            std::cout << "Worker thread read: done" << std::endl;
            break;
        }
        n_wkr_thread_count_mutex.V();

        std::cout << "New request: " << req << std::endl;
        request_chan_mutex.P();
        std::string reply = wtfargs->rc->send_request(req);
        std::cout << "Out from PCBuffer: " << req << std::endl;
        request_chan_mutex.V();
	    std::cout << "Reply to request '" << req << "': " << reply << std::endl;

        std::string name = req.substr(5, req.length() - 1);
        PatientHistogram* patient_histogram = &(*wtfargs->PatientData)[name];
        patient_histogram->PatientDataBuffer->Deposit(reply);
    }
    wtfargs->rc->send_request("quit");
    wtfargs->PCB->Deposit("done");
    if (*(wtfargs->n_wkr_threads) != 1) {
        pthread_detach(pthread_self());
        *(wtfargs->n_wkr_threads) = *(wtfargs->n_wkr_threads) - 1;
        delete wtfargs->rc;
    } else {
        delete wtfargs->rc;
        pthread_detach(pthread_self());
        for (auto i : *wtfargs->PatientData) {
            i.second.PatientDataBuffer->Deposit("done");
        }
        *(wtfargs->n_wkr_threads) = *(wtfargs->n_wkr_threads) - 1;
    }
    n_wkr_thread_count_mutex.V();
    return nullptr;
}

void* stats_thread_func(void* args) {
    STFargs* stfargs = (STFargs *) args;
    std::unordered_map<std::string, PatientHistogram>* PatientData = stfargs->PatientData;
    std::string patient_name = stfargs->patient_name;
    for (;;) {
        std::string req = (*PatientData)[stfargs->patient_name].PatientDataBuffer->Retrieve();
        if (req.compare("done") == 0) {
            histogram_print_sync.P();
            std::cout << "Statistic thread read: 'done'" << std::endl;
            std::cout << "HISTOGRAM FOR " << stfargs-> patient_name << std::endl;
            std::cout << "00-09: " << (*PatientData)[patient_name].histogram[0].size() << std::endl;
            std::cout << "10-19: " << (*PatientData)[patient_name].histogram[1].size() << std::endl;
            std::cout << "20-29: " << (*PatientData)[patient_name].histogram[2].size() << std::endl;
            std::cout << "30-39: " << (*PatientData)[patient_name].histogram[3].size() << std::endl;
            std::cout << "40-49: " << (*PatientData)[patient_name].histogram[4].size() << std::endl;
            std::cout << "50-59: " << (*PatientData)[patient_name].histogram[5].size() << std::endl;
            std::cout << "60-69: " << (*PatientData)[patient_name].histogram[6].size() << std::endl;
            std::cout << "70-79: " << (*PatientData)[patient_name].histogram[7].size() << std::endl;
            std::cout << "80-89: " << (*PatientData)[patient_name].histogram[8].size() << std::endl;
            std::cout << "90-99: " << (*PatientData)[patient_name].histogram[9].size() << std::endl;
            delete (*PatientData)[patient_name].PatientDataBuffer;
            histogram_print_sync.V();
            break;
        }
        else {
            int data = stoi(req);
            if (data <= 9) {
                (*PatientData)[patient_name].histogram[0].push_back(data);
            } else if (data <= 19) {
                (*PatientData)[patient_name].histogram[1].push_back(data);
            } else if (data <= 29) {
                (*PatientData)[patient_name].histogram[2].push_back(data);
            } else if (data <= 39) {
                (*PatientData)[patient_name].histogram[3].push_back(data);
            } else if (data <= 49) {
                (*PatientData)[patient_name].histogram[4].push_back(data);
            } else if (data <= 59) {
                (*PatientData)[patient_name].histogram[5].push_back(data);
            } else if (data <= 69) {
                (*PatientData)[patient_name].histogram[6].push_back(data);
            } else if (data <= 79) {
                (*PatientData)[patient_name].histogram[7].push_back(data);
            } else if (data <= 89) {
                (*PatientData)[patient_name].histogram[8].push_back(data);
            } else {
                (*PatientData)[patient_name].histogram[9].push_back(data);
            }
        }
    }
    return nullptr;
}

void create_worker(int _thread_id, RequestChannel* rq, PCBuffer* PCB, int* n_wkr_thread_count, 
                    std::unordered_map<std::string, PatientHistogram>* patient_data, 
                    pthread_t* wk_threads, WTFargs* args) {
    args[_thread_id].PCB = PCB;
    args[_thread_id].rc = rq;
    args[_thread_id].PatientData = patient_data;
    args[_thread_id].n_wkr_threads = n_wkr_thread_count;
    std::cout << "Creating new worker thread..." << std::endl;
    pthread_create(&wk_threads[_thread_id], NULL, worker_thread_func, (void*) &args[_thread_id]);
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char * argv[]) {
    /* TODO:
        - Clean up code & re-check, otherwise, the program is finished.
    */
    size_t num_requests = 0;
    size_t pcb_size = 0;
    size_t num_threads = 0;
        
    int opt;

    if (fork() == 0){ 
        execve("dataserver", NULL, NULL);
    } else {
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
        std::cout << "Creating hash map..." << std::endl;
        std::unordered_map<std::string, PatientHistogram>* patient_data = new std::unordered_map<std::string,PatientHistogram>();
        std::cout << "done." << std::endl;

        std::cout << "Creating PCBuffer..." << std::endl;
        PCBuffer* PCB = new PCBuffer(pcb_size);
        if (PCB == 0) {
            std::cout << "PCBuffer creation failed! Exiting..." << std::endl;
            exit(1);
        }
        std::cout << "done." << std::endl;

        pthread_t* rq_threads = new pthread_t[NUM_PATIENTS];
        pthread_t* st_threads = new pthread_t[NUM_PATIENTS];
        pthread_t* wk_threads = new pthread_t[num_threads];
        RTFargs* rtfargs = new RTFargs[NUM_PATIENTS];
        STFargs* stfargs = new STFargs[NUM_PATIENTS]; 

        int n_req_threads = NUM_PATIENTS;
        int n_wkr_threads = num_threads;

        std::cout << "Creating request threads..." << std::endl;
        for (size_t i = 0; i < NUM_PATIENTS; i++) {
            rtfargs[i].n_req = num_requests;
            rtfargs[i].patient_name = "Patient " + std::to_string(i + 1);
            rtfargs[i].PCB = PCB;
            rtfargs[i].n_req_threads = &n_req_threads;
            pthread_create(&rq_threads[i], NULL, request_thread_func, (void*) &rtfargs[i]);

            PCBuffer* stats_buff = new PCBuffer(pcb_size);
            (*patient_data)[rtfargs[i].patient_name].PatientDataBuffer = stats_buff;
            stfargs[i].PatientData = patient_data;
            stfargs[i].patient_name = rtfargs[i].patient_name;
            pthread_create(&st_threads[i], NULL, stats_thread_func, (void*) &stfargs[i]);
        }
        std::cout << "done." << std::endl;

        WTFargs* wtfargs = new WTFargs[num_threads];

        for (size_t i = 0; i < num_threads; i++) {
            std::string reply = chan.send_request("newthread");
            std::cout << "Reply to request 'newthread' is " << reply << std::endl;
            std::cout << "Establishing new control channel... " << std::flush;
            RequestChannel* new_chan = new RequestChannel(reply, RequestChannel::Side::CLIENT);
            std::cout << "done." << std::endl;
            create_worker(i, new_chan, PCB, &n_wkr_threads, patient_data, wk_threads, wtfargs);
        }

        for (size_t i = 0; i < NUM_PATIENTS; i++)
            pthread_join(st_threads[i], NULL);

        std::string fin_reply = chan.send_request("quit");
        std::cout << "Reply from control read: " << fin_reply << std::endl;

        delete[] rq_threads;
        delete[] wk_threads;
        delete[] st_threads;
        delete[] wtfargs;
        delete[] stfargs;
        delete[] rtfargs;
        delete PCB;
        delete patient_data;
    }

    usleep(1000000);
    exit(0);
}
