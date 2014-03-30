#include <stdio.h>
#include <stdlib.h>
#include "cond.h"
#include "pvm3.h"
#include <unistd.h>
#include <sys/time.h>
#define GRUPA "grupa"
#define COND_WAIT 4000
#define COND_SIGNAL 4001

void* cond_wait_proxy(void* t) {
struct condThread *args = (struct condThread *)t;
static_cast<CondVar*>(args->obj)->cond_lock(args->sem);
}

CondVar::CondVar(int n,int master,int tids_table[], int id, int slvtid) {
my_id = id;
nproc = n;
master_tid = master;
mytid = slvtid;
tids = tids_table;
sem_init(&mutex, 0,0);
waiting = 0;
}

void CondVar::cond_lock(Semafor* s) {
s->release();
waiting = 1;
CondVar::broadcastWait();
sem_wait(&mutex);
s->acquire();
}

void CondVar::cond_wait(Semafor* s) {
struct condThread t;
t.obj = this;
t.sem =s;
pthread_create(&thread_a, NULL, cond_wait_proxy,(void*) &t);
pthread_join(thread_a, NULL);
}


void CondVar::broadcastWait() {
int typ = COND_WAIT;
int nic = 0;
int i;
for (i=0;i<nproc;i++) {
pvm_initsend(PvmDataDefault);
pvm_pkint(&mytid,1,1);
pvm_pkint(&typ,1,1);
pvm_pkint(&nic,1,1);
pvm_pkint(&my_id,1,1);
pvm_send(tids[i],my_id);
}
}

void CondVar::cond_signal() {
int typ = COND_SIGNAL;
int nic = 0;
if (!awaiting.empty()) {
int rcver = awaiting.front();
awaiting.pop();
pvm_initsend(PvmDataDefault);
pvm_pkint(&mytid,1,1);
pvm_pkint(&typ,1,1);
pvm_pkint(&nic,1,1);
pvm_pkint(&my_id,1,1);
pvm_send(rcver,my_id);
}
}

void CondVar::cond_signal_all() {
int typ = COND_SIGNAL;
int nic = 0;
while (!awaiting.empty()) {
int rcver = awaiting.front();
awaiting.pop();
pvm_initsend(PvmDataDefault);
pvm_pkint(&mytid,1,1);
pvm_pkint(&typ,1,1);
pvm_pkint(&nic,1,1);
pvm_pkint(&my_id,1,1);
pvm_send(rcver,my_id);
}
}

void CondVar::add_to_awaiting(int who) {
awaiting.push(who);
}

void CondVar::signalThis() {
if (waiting==1) {
waiting=0;
sem_post(&mutex);
}
}
