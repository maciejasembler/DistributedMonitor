#include <stdio.h>
#include <stdlib.h>
#include "buffer.h"
#include "pvm3.h"
#include <unistd.h>
#include <sys/time.h>
#define GRUPA "grupa"
#define BUFFER_UPDATE 5000
#define BUFFER_ACK 5001

void* buffer_update_proxy(void* t) {
static_cast<Buffer*>(t)->update();
}


Buffer::Buffer(int elements, int n, int id, int rcv_tids[], int slvtid) {
element_count = elements;
nproc = n;
my_id = id;
tids = rcv_tids;
mytid = slvtid;
values = new int[element_count];
pointer = 0;
sem_init(&mutex,0,0);
sem_init(&recv_mutex,0,1);
acks = 0;
halted = 0;
int i;
for (i=0;i<element_count;i++)
values[i]=0;
}

void Buffer::produce(int value) {
values[pointer] = value;
pointer++;
Buffer::broadcast_update(value);
pthread_create(&thread_a, NULL, buffer_update_proxy,this);
pthread_join(thread_a, NULL);
}


int Buffer::consume() {
pointer--;
int result = values[pointer];
Buffer::broadcast_update(-1);
pthread_create(&thread_a, NULL, buffer_update_proxy,this);
pthread_join(thread_a, NULL);
return result;
}

void Buffer::signal() {
acks++;
if (acks==nproc-1) {
acks=0;
if (halted==1) {
halted = 0;
sem_post(&mutex);
}
}
}

void Buffer::update() {
halted=1;
sem_wait(&mutex);
}

void Buffer::broadcast_update(int newval) {
int i;
int info = newval;
int typ = BUFFER_UPDATE;
for (i=0;i<nproc;i++) {
if (tids[i]!=mytid) {
pvm_initsend(PvmDataDefault);
pvm_pkint(&mytid,1,1);
pvm_pkint(&typ,1,1);
pvm_pkint(&info,1,1);
pvm_pkint(&my_id,1,1);
pvm_send(tids[i],my_id);
}
}
}

void Buffer::rcv_update(int who, int newval) {
sem_wait(&recv_mutex);
int nic = newval;
if (newval>=0) {
values[pointer] = newval;
pointer++;
}
else {
pointer--;
values[pointer] = 0;
}
int typ = BUFFER_ACK;
pvm_initsend(PvmDataDefault);
pvm_pkint(&mytid,1,1);
pvm_pkint(&typ,1,1);
pvm_pkint(&nic,1,1);
pvm_pkint(&my_id,1,1);
pvm_send(who,my_id);
sem_post(&recv_mutex);
}
