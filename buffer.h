#include <semaphore.h>
#include <pthread.h>
#include <queue>
#ifndef _buffer_h_included_
#define _buffer_h_included_

class Buffer {
public:
Buffer(int elements, int n, int id, int rcv_tids[], int slvtid);
int* tids;
int nproc;
int my_id;
int mytid;
int pointer;
int *values;
int element_count;
int acks;
int halted;
sem_t mutex, recv_mutex;
pthread_t thread_a;
void produce(int val);
int consume();
void signal();
void update();
void broadcast_update(int newval);
void rcv_update(int who, int newval);
};
void* buffer_update_proxy(void* ptr);
#endif

