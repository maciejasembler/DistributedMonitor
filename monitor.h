#include <semaphore.h>
#include <pthread.h>
#include <list>
#include "cond.h"
#include "semafor.h"
#include "buffer.h"
#ifndef _monitor_h_included_
#define _monitor_h_included_
class Monitor {
public:
int* tids;
int nproc;
int master_tid;
int my_tid;
pthread_t thread_b;
Monitor(int n,int master,int tid_table[],int slvtid);
void monitor_listen();
void destroy();
void run_listener();
void go();
void philosophy(int groupid);
void in_section(int tid, int turn);
void out_section(int tid, int turn);
void val_produced(int val, int tid);
void val_consumed(int val, int tid);
void phil_out_section(int tid);
void phil_in_section(int tid);
void phil_table_in(int tid);
void phil_table_out(int tid);
void reader_in_section(int tid);
void writer_in_section(int tid);
void reader_out_section(int tid);
void writer_out_section(int tid);
void produce(int val);
void consume();
void reader();
void writer();
int hasGreaterPriority(int t1, int t2,int token1, int token2);
private:
std::list<Semafor> semafory;
std::list<CondVar> zm_warunkowe;
std::list<Buffer> bufory;
void semaphoreAllowed(int who, int id, int turn);
void semaphoreAcquired(int who, int id, int turn);
void condWaiting(int recv_id,int who);
void condSignal(int recv_id);
void bufferSignal(int recv_id);
void bufferUpdate(int recv_id, int who, int newval);
Semafor* getSemaphore(int id);
CondVar* getCondVar(int id);
Buffer* getBuffer(int id);
};
void* monitor_listen_proxy(void* ptr);
#endif
