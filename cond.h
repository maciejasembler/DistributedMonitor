#include <semaphore.h>
#include <pthread.h>
#include <queue>
#include "semafor.h"
#ifndef _cond_h_included_
#define _cond_h_included_

struct condThread {
void* obj;
Semafor *sem;
};

class CondVar {
pthread_t thread_a;
sem_t mutex;
std::queue<int> awaiting;
int nproc;
int master_tid;
int mytid;
int waiting;
int* tids;
public:
void cond_wait(Semafor* s );
void cond_signal();
void cond_signal_all();
void add_to_awaiting(int who);
void broadcastWait();
void signalThis();
int my_id;
void cond_lock(Semafor* s);
CondVar(int n,int master,int tids_table[], int id, int slvtid);
};
void* cond_wait_proxy(void* t);
#endif

