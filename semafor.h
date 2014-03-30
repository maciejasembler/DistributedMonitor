#include <semaphore.h>
#include <pthread.h>
#include <queue>
#ifndef _semafor_h_included_
#define _semafor_h_included_
//using namespace std;
class Semafor {
public:
int* tids;
int nproc;
int master_tid;
pthread_t thread_a;
int permited;
int my_id;
int want;
int section;
int tura;
int limit;
int mytid;
int allows;
std::queue<int> kolejka;
sem_t mutex;
Semafor(int n,int master,int tid_table[], int id, int limit, int slvtid);
void acquire();
void release();
void semaforEnter();
void sendAllow(int who, int turn);
void releaseQueue();
void addToQueue(int who, int turn);
void broadcastAcquire();
void signal();
};
void* semafor_enter_proxy(void* ptr);
#endif
