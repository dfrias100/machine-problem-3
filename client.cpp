/* 
    File: client.cpp

    Author: Daniel Frias
            Department of Computer Science
            Texas A&M University
    Date  : 

    Client main program for MP3
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
#include <iomanip>
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
    size_t* n_req_threads;
    size_t n_req;
    pthread_t* thread_id;
    PCBuffer* PCB;
    std::string patient_name;
} RTFargs;

typedef struct {
    size_t* n_wkr_threads;
    pthread_t* thread_id;
    PCBuffer* PCB;
    RequestChannel* rc;
    std::unordered_map<std::string, PatientHistogram>* PatientData;
} WTFargs;

typedef struct {
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

const size_t NUM_PATIENTS = 5;

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

    /* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* LOCAL FUNCTIONS -- SUPPORT FUNCTIONS */
/*--------------------------------------------------------------------------*/

void print_histogram(std::vector<int> histogram[]) {
    size_t total_data_points = 0;
    size_t scale = 1;

    for (size_t i = 0; i < 10; i++)
        total_data_points += histogram[i].size();

    std::cout << "Size of data: " << total_data_points << std::endl;

    if (total_data_points >= 1000) {
        scale = total_data_points / 100;
        std::cout << "Scale: 1 x for every " << scale << " data points." << std::endl;
    } else {
        std::cout << "Scale: 1 x for every data point." << std::endl;
    }

    for (size_t i = 0; i < 10; i++) {
        std::cout << std::setw(2) << std::setfill('0') << i * 10 << "-" << std::setw(2) << std::setfill('0') << i * 10 + 9 << ":";
        for (size_t j = 0; j < histogram[i].size(); j++) {
            if ((j + 1) % scale == 0) {
                std::cout << "x"; 
            }
        }
        std::cout << std::endl;
    }
}

void* request_thread_func(void* args) {
    RTFargs* rtfargs = (RTFargs*) args;
    size_t* n_req_threads = rtfargs->n_req_threads;
    for (size_t i = 0; i < rtfargs->n_req; i++) {
        std::string req = "data " + rtfargs->patient_name;
        std::cout << "Depositing request..." << std::endl;
	    rtfargs->PCB->Deposit(req);
    }

    n_req_thread_count_mutex.P();
    if (*n_req_threads != 1)
        *n_req_threads = *n_req_threads - 1;
    else {
        rtfargs->PCB->Deposit("done");
        *n_req_threads = *n_req_threads - 1;
    }
    std::cout << "Request thread finished." << std::endl;
    n_req_thread_count_mutex.V();
    pthread_detach(*rtfargs->thread_id);
    return nullptr;
}

void* worker_thread_func(void* args) {
    WTFargs* wtfargs = (WTFargs*) args;
    size_t* n_wkr_threads = wtfargs->n_wkr_threads;
    for(;;) {
        std::string req = wtfargs->PCB->Retrieve();
        n_wkr_thread_count_mutex.P();
        if (req.compare("done") == 0) {
            std::cout << "Worker thread read 'done' from PCBuffer" << std::endl;
            wtfargs->rc->send_request("quit");
            wtfargs->PCB->Deposit("done");
            if (*n_wkr_threads != 1) {
                *n_wkr_threads = *n_wkr_threads - 1;
                delete wtfargs->rc;
            } else {
                pthread_join(*wtfargs->thread_id, NULL);
                delete wtfargs->rc;
                for (auto i : *wtfargs->PatientData) {
                    i.second.PatientDataBuffer->Deposit("done");
                }
                *n_wkr_threads = *n_wkr_threads - 1;
            }
            std::cout << "Worker thread finished." << std::endl;
            n_wkr_thread_count_mutex.V();
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
    pthread_detach(*wtfargs->thread_id);
    return nullptr;
}

void* stats_thread_func(void* args) {
    STFargs* stfargs = (STFargs *) args;
    std::unordered_map<std::string, PatientHistogram>* PatientData = stfargs->PatientData;
    std::string patient_name = stfargs->patient_name;
    PatientHistogram* patient_histogram = &PatientData->find(patient_name)->second;
    for (;;) {
        std::string req = patient_histogram->PatientDataBuffer->Retrieve();
        if (req.compare("done") == 0) {
            histogram_print_sync.P();
            std::cout << "Statistic thread read 'done' from PCBuffer" << std::endl;
            std::cout << "HISTOGRAM FOR " << stfargs-> patient_name << std::endl;
            print_histogram(patient_histogram->histogram);
            delete patient_histogram->PatientDataBuffer;
            histogram_print_sync.V();
            break;
        } else {
            size_t data = stoi(req);
            if (data <= 9) {
                patient_histogram->histogram[0].push_back(data);
            } else if (data <= 19) {
                patient_histogram->histogram[1].push_back(data);
            } else if (data <= 29) {
                patient_histogram->histogram[2].push_back(data);
            } else if (data <= 39) {
                patient_histogram->histogram[3].push_back(data);
            } else if (data <= 49) {
                patient_histogram->histogram[4].push_back(data);
            } else if (data <= 59) {
                patient_histogram->histogram[5].push_back(data);
            } else if (data <= 69) {
                patient_histogram->histogram[6].push_back(data);
            } else if (data <= 79) {
                patient_histogram->histogram[7].push_back(data);
            } else if (data <= 89) {
                patient_histogram->histogram[8].push_back(data);
            } else {
                patient_histogram->histogram[9].push_back(data);
            }
        }
    }
    return nullptr;
}

void create_worker(int _thread_num, RequestChannel* _rc, PCBuffer* _PCB, size_t* n_wkr_thread_count, 
                    std::unordered_map<std::string, PatientHistogram>* patient_data, 
                    pthread_t* wk_threads, WTFargs* args) {
    args[_thread_num].PCB = _PCB;
    args[_thread_num].rc = _rc;
    args[_thread_num].PatientData = patient_data;
    args[_thread_num].thread_id = &wk_threads[_thread_num]; 
    args[_thread_num].n_wkr_threads = n_wkr_thread_count;
    std::cout << "Creating new worker thread..." << std::endl;
    pthread_create(&wk_threads[_thread_num], NULL, worker_thread_func, (void*) &args[_thread_num]);
}

void create_requester(int _thread_num, int _num_requests, std::string _patient_name, PCBuffer* _PCB, 
                        pthread_t* rq_threads, size_t* _n_req_threads, RTFargs* args) {
    args[_thread_num].n_req = _num_requests;
    args[_thread_num].patient_name = _patient_name;
    args[_thread_num].PCB = _PCB;
    args[_thread_num].thread_id = &rq_threads[_thread_num];
    args[_thread_num].n_req_threads = _n_req_threads;
    pthread_create(&rq_threads[_thread_num], NULL, request_thread_func, (void*) &args[_thread_num]);
}

void create_stats(int _thread_num, std::string _patient_name, std::unordered_map<std::string, PatientHistogram>* patient_data, 
                    pthread_t* st_threads, STFargs* args) {
    args[_thread_num].PatientData = patient_data;
    args[_thread_num].patient_name = _patient_name;
    pthread_create(&st_threads[_thread_num], NULL, stats_thread_func, (void*) &args[_thread_num]);
}

/*--------------------------------------------------------------------------*/
/* MAIN FUNCTION */
/*--------------------------------------------------------------------------*/

int main(int argc, char * argv[]) {
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
                std::cout << "Invalid parameters or no parameters passed. Check your input and start again." << std::endl;
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

    if (fork() == 0){ 
        execve("dataserver", NULL, NULL);
    } else {  
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

        size_t n_req_threads = NUM_PATIENTS;
        size_t n_wkr_threads = num_threads;

        std::cout << "Creating request threads..." << std::endl;
        for (size_t i = 0; i < NUM_PATIENTS; i++) {
            std::string patient_name = "Patient " + std::to_string(i + 1);

            create_requester(i, num_requests, patient_name, PCB, rq_threads, &n_req_threads, rtfargs);

            PCBuffer* stats_buff = new PCBuffer(pcb_size);
            (*patient_data)[patient_name].PatientDataBuffer = stats_buff;
            create_stats(i, patient_name, patient_data, st_threads, stfargs);
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

        std::cout << "Closing control request channel..." << std::endl;
        std::string fin_reply = chan.send_request("quit");
        std::cout << "Reply from control channel read: " << fin_reply << std::endl;
        std::cout << "done." << std::endl;

        std::cout << "Clearing the heap..." << std::endl;
        delete[] rq_threads;
        delete[] wk_threads;
        delete[] st_threads;
        delete[] wtfargs;
        delete[] stfargs;
        delete[] rtfargs;
        delete PCB;
        delete patient_data;
        std::cout << "Client stopped successfully." << std::endl;
    }

    usleep(1000000);
    exit(0);
}
