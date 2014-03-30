#include <stdio.h>
#include <stdlib.h>
#include "monitor.h"
#include "semafor.h"
#include "cond.h"
#include "pvm3.h"
#include <unistd.h>
#include <sys/time.h> 
#define GRUPA "grupa"
#define SEM_ALLOW 3000
#define SEM_ACQUIRE 3001
#define COND_WAIT 4000
#define COND_SIGNAL 4001
#define IN_SECTION 10000
#define OUT_SECTION 10001
#define MSG_SLV 20000
#define BUFFER_UPDATE 5000
#define BUFFER_ACK 5001
#define PHILOSOPHY_SECT 6000
#define PHILOSOPHY_OUT 6001
#define PHILOSOPHY_TABLE_IN 6002
#define PHILOSOPHY_TABLE_OUT 6003
#define PRODUCED 7000
#define CONSUMED 7001
#define READING 8000
#define WRITING 8001
#define READING_OUT 8002
#define WRITING_OUT 8003
/*
*Konstruktorek
*uzywamy listy z semaforami, dzieki czemu mozna korzystac z wielu semaforow 
*w wielu metodach :OO
*/
Monitor::Monitor(int n, int master, int tid_table[], int slvtid) {
	nproc = n;
	master_tid = master;
	tids = tid_table;
	my_tid = slvtid;
	//id=1 - klasyk do wpuszczania do sekcji n procesow - tutaj4
	Semafor *s = new Semafor(nproc, master_tid, tids, 1, 4, my_tid);
	semafory.push_back(*s);
	//id = 2 - lokaj wpuszczajacy 4 filozofow
	//pozostale - semafory paleczki
	Semafor *lokaj = new Semafor(nproc, master_tid, tids, 2, 4, my_tid);
	Semafor *paleczka1 = new Semafor(nproc, master_tid, tids, 3, 1, my_tid);
	Semafor *paleczka2 = new Semafor(nproc, master_tid, tids, 4, 1, my_tid);
	Semafor *paleczka3 = new Semafor(nproc, master_tid, tids, 5, 1, my_tid);
	Semafor *paleczka4 = new Semafor(nproc, master_tid, tids, 6, 1, my_tid);
	Semafor *paleczka5 = new Semafor(nproc, master_tid, tids, 7, 1, my_tid);
	semafory.push_back(*lokaj);
	semafory.push_back(*paleczka1);
	semafory.push_back(*paleczka2);
	semafory.push_back(*paleczka3);
	semafory.push_back(*paleczka4);
	semafory.push_back(*paleczka5);

	//semafor - producent-konsument oraz zmienne warunkowe i buforek
	Semafor* prod_kons = new Semafor(nproc, master_tid, tids, 8, 1, my_tid);
	CondVar *c1 = new CondVar(nproc, master_tid, tids, 10, my_tid);
	CondVar *c2 = new CondVar(nproc, master_tid, tids, 11, my_tid);
	Buffer* b = new Buffer(5, nproc, 12, tids, my_tid);
	bufory.push_back(*b);
	semafory.push_back(*prod_kons);
	zm_warunkowe.push_back(*c1);
	zm_warunkowe.push_back(*c2);

	//semafory i inne cuda do pisarzy-czytelnikow
	Semafor* s1 = new Semafor(nproc, master_tid, tids, 20, 1, my_tid);
	Semafor* s2 = new Semafor(nproc, master_tid, tids, 21, 1, my_tid);
	Buffer* b2 = new Buffer(nproc, nproc, 23, tids, my_tid);
        bufory.push_back(*b2);
        semafory.push_back(*s1);
	semafory.push_back(*s2);
}

//zalacza watek sluchajacy, uzywamy takiego proxy bo inaczej sie nie da bo 
//metoda tegoz obiektu
void Monitor::run_listener() {
	pthread_create(&thread_b, NULL, monitor_listen_proxy,this);
	pvm_barrier(GRUPA,nproc);
}
//watek sluchajacy
void Monitor::monitor_listen() {
int type;
int who;
int turn;
int recv_id;
while(1){
    usleep(100000); //koniecznie sleep zeby bylo przelaczanie kontekstu i sie nie wieszalo
	if (pvm_probe(-1, -1)) {
	pvm_recv(-1, -1);
	pvm_upkint(&who, 1, 1);
	pvm_upkint(&type, 1, 1);
	pvm_upkint(&turn,1,1);
	pvm_upkint(&recv_id,1,1); //odebranie pakietu (4 zmienne)
	switch (type) {
	case SEM_ALLOW: //jak dostajemy zgode to takie o
	Monitor::semaphoreAllowed(who, recv_id, turn);
	break;
	case SEM_ACQUIRE: //jak ktos sie ubiega to takie o 
	Monitor::semaphoreAcquired(who, recv_id, turn);
	break;
	case COND_WAIT:
	Monitor::condWaiting(recv_id, who);
	break;
	case COND_SIGNAL:
	Monitor::condSignal(recv_id);
	break;
	case BUFFER_ACK:
	Monitor::bufferSignal(recv_id);
	break;
	case BUFFER_UPDATE:
	int newval = turn;
	Monitor::bufferUpdate(recv_id, who, newval);
	break;
	}
    }
}
}

void Monitor::bufferSignal(int recv_id) {
Buffer* b = Monitor::getBuffer(recv_id);
b->signal();
}

void Monitor::bufferUpdate(int recv_id, int who, int newval) {
Buffer* b = Monitor::getBuffer(recv_id);
b->rcv_update(who, newval);
}

void Monitor::condWaiting(int recv_id, int who) {
CondVar *c = Monitor::getCondVar(recv_id);
c->add_to_awaiting(who);
}

void Monitor::condSignal(int recv_id) {
CondVar *c = Monitor::getCondVar(recv_id);
c->signalThis();
}

void Monitor::semaphoreAllowed(int who, int id, int turn) {
   std::list<Semafor>::iterator it;
   for( it=semafory.begin(); it!=semafory.end(); ++it )
   {
      if ((it->my_id==id) && (it->tura==turn)) //odnajdujemy semafor na liscie
	{ //tura == turn musi to byc odpowiedz na zadanie aktualnie a nie z przeszlosci!
	if (it->section==0) { //jak nie w sekcji
	it->allows++; //licznik zgod
	if (it->allows>=nproc-it->limit) //jak spelnia warunki wejscia (odpowiednia ilosc zgod)
	it->signal();//wejscie do sekcji
	}
	}
   }
}

void Monitor::semaphoreAcquired(int who, int id, int turn) {
   std::list<Semafor>::iterator it;
   for( it=semafory.begin(); it!=semafory.end(); ++it )
   {
      if (it->my_id==id) //znajdujemy sie na liscie
        {
	if (it->section==0) { //jak nie jestesmy w sekcji 
	if (it->want==0) { //i sie nie ubiegamy o sekcje
        it->sendAllow(who, turn);
	//Monitor::in_section(0,0); //odeslac zgode
	}
	else { //jak sie ubiegamy o wejscie - rozsztrzygnij challenge
	if (Monitor::hasGreaterPriority(it->tura,turn,my_tid,who)) {
	it->addToQueue(who, turn); //jak ja mam lepszy priorytet
	}
	else { // a jak nie to 
	it->sendAllow(who, turn);
	}
	}
        } //jak jestesmy w sekcji to 
	else{
	it->addToQueue(who, turn);
	}
	}
   }

}
//sprawdza priorytet
int Monitor::hasGreaterPriority(int t1, int t2,int token1, int token2) {
    if (t1 < t2) //ilosc kolejek
        return 1;
    if ((t1==t2) && (token1>token2)) //jak te sama ilosc razy bylem i mam nizszy tid to wygrywam
    	return 1;
    return 0;
}

void Monitor::destroy() { //wywalenie monitora
	pvm_barrier(GRUPA,nproc); //konieczne bo musimy wylaczyc wszystkie watki sluchajace kiedy nie sa potrzebne - jak kazdy wywolal destroy 
	pthread_cancel(thread_b);
}

void* monitor_listen_proxy(void* ptr){
	static_cast<Monitor*>(ptr)->monitor_listen();
}

void Monitor::go() {
Semafor *s;
s=Monitor::getSemaphore(1);
s->acquire();
Monitor::in_section(s->mytid, s->tura);
sleep(1);
Monitor::out_section(s->mytid, s->tura);
s->release();
}


void Monitor::philosophy(int groupid) {
Semafor* lokaj;
lokaj = Monitor::getSemaphore(2);
Semafor* widelec1;
Semafor* widelec2;
widelec1= getSemaphore((groupid%5)+3);
widelec2= getSemaphore(((groupid+1)%5)+3);
lokaj->acquire();
Monitor::phil_table_in(lokaj->mytid);
widelec1->acquire();
widelec2->acquire();
Monitor::phil_in_section(lokaj->mytid);
sleep(1);
Monitor::phil_out_section(lokaj->mytid);
widelec2->release();
widelec1->release();
Monitor::phil_table_out(lokaj->mytid);
lokaj->release();
}

void Monitor::produce(int val) {
Buffer* b;
CondVar* c1;
CondVar* c2;
Semafor* s;
int curr_ptr;
s = Monitor::getSemaphore(8);
b = Monitor::getBuffer(12);
c1 = Monitor::getCondVar(10);
c2 = Monitor::getCondVar(11);
s->acquire();
curr_ptr = b->pointer;
do {
if (curr_ptr<5) {
b->produce(val);
curr_ptr = b->pointer;
Monitor::val_produced(val, curr_ptr);
break;
}
else {
c1->cond_wait(s);
curr_ptr = b->pointer;
}
} while(1);
c2->cond_signal();
s->release();
}

void Monitor::consume() {
Buffer* b;
CondVar* c1;
CondVar* c2;
Semafor* s;
int curr_ptr;
s = Monitor::getSemaphore(8);
b = Monitor::getBuffer(12);
c1 = Monitor::getCondVar(10);
c2 = Monitor::getCondVar(11);
int val;
s->acquire();
curr_ptr = b->pointer;
do {
if (curr_ptr>0) {
val = b->consume();
curr_ptr = b->pointer;
Monitor::val_consumed(val, curr_ptr);
break;
}
else {
c2->cond_wait(s);
curr_ptr = b->pointer;
}
} while(1);
c1->cond_signal();
s->release();
}

void Monitor::reader() {
Semafor* writeSem;
Semafor* readSem;
Buffer* czytelnicy;
writeSem = Monitor::getSemaphore(20);
readSem = Monitor::getSemaphore(21);
czytelnicy = Monitor::getBuffer(23);

readSem->acquire();
czytelnicy->produce(1);
if (czytelnicy->pointer>=1)
writeSem->acquire();
readSem->release();
Monitor::reader_in_section(readSem->mytid);
sleep(1);
readSem->acquire();
czytelnicy->consume();
Monitor::reader_out_section(readSem->mytid);
if (czytelnicy->pointer==0)
writeSem->release();
readSem->release();
}

void Monitor::writer() {
Semafor* writeSem;
writeSem = Monitor::getSemaphore(20);
writeSem->acquire();
Monitor::writer_in_section(writeSem->mytid);
sleep(2);
Monitor::writer_out_section(writeSem->mytid);
writeSem->release();
}

Semafor* Monitor::getSemaphore(int id) {
std::list<Semafor>::iterator it;
  for( it=semafory.begin(); it!=semafory.end(); ++it )
   {
   if (it->my_id==id)
   return &*it; //zagranie godne bialego bario malotelliego XDDD
   }
}

CondVar* Monitor::getCondVar(int id) {
std::list<CondVar>::iterator it;
for (it=zm_warunkowe.begin(); it!=zm_warunkowe.end(); ++it) {
if (it->my_id==id)
return &*it;
}
}

Buffer* Monitor::getBuffer(int id) {
std::list<Buffer>::iterator it;
for (it=bufory.begin(); it!=bufory.end(); ++it) {
if (it->my_id==id)
return &*it;
}
}


//komunikat diagnostyczny
void Monitor::phil_in_section(int tid) {
int wiad = PHILOSOPHY_SECT;
int from = tid;
int turka = 0;
pvm_initsend(PvmDataDefault);
pvm_pkint(&wiad,1,1);
pvm_pkint(&from,1,1);
pvm_pkint(&turka,1,1);
pvm_send(master_tid,MSG_SLV);
}

void Monitor::phil_out_section(int tid) {
int wiad = PHILOSOPHY_OUT;
int from = tid;
int turka = 0;
pvm_initsend(PvmDataDefault);
pvm_pkint(&wiad,1,1);
pvm_pkint(&from,1,1);
pvm_pkint(&turka,1,1);
pvm_send(master_tid,MSG_SLV);
}

void Monitor::phil_table_in(int tid) {
int wiad = PHILOSOPHY_TABLE_IN;
int from = tid;
int turka = 0;
pvm_initsend(PvmDataDefault);
pvm_pkint(&wiad,1,1);
pvm_pkint(&from,1,1);
pvm_pkint(&turka,1,1);
pvm_send(master_tid,MSG_SLV);
}

void Monitor::phil_table_out(int tid) {
int wiad = PHILOSOPHY_TABLE_OUT;
int from = tid;
int turka = 0;
pvm_initsend(PvmDataDefault);
pvm_pkint(&wiad,1,1);
pvm_pkint(&from,1,1);
pvm_pkint(&turka,1,1);
pvm_send(master_tid,MSG_SLV);
}


void Monitor::reader_in_section(int tid) {
int wiad = READING;
int from = tid;
int turka = 0;
pvm_initsend(PvmDataDefault);
pvm_pkint(&wiad,1,1);
pvm_pkint(&from,1,1);
pvm_pkint(&turka,1,1);
pvm_send(master_tid,MSG_SLV);
}

void Monitor::writer_in_section(int tid) {
int wiad = WRITING;
int from = tid;
int turka = 0;
pvm_initsend(PvmDataDefault);
pvm_pkint(&wiad,1,1);
pvm_pkint(&from,1,1);
pvm_pkint(&turka,1,1);
pvm_send(master_tid,MSG_SLV);
}

void Monitor::reader_out_section(int tid) {
int wiad = READING_OUT;
int from = tid;
int turka = 0;
pvm_initsend(PvmDataDefault);
pvm_pkint(&wiad,1,1);
pvm_pkint(&from,1,1);
pvm_pkint(&turka,1,1);
pvm_send(master_tid,MSG_SLV);
}


void Monitor::writer_out_section(int tid) {
int wiad = WRITING_OUT;
int from = tid;
int turka = 0;
pvm_initsend(PvmDataDefault);
pvm_pkint(&wiad,1,1);
pvm_pkint(&from,1,1);
pvm_pkint(&turka,1,1);
pvm_send(master_tid,MSG_SLV);
}


//komunikat diagnostyczny
void Monitor::in_section(int tid, int turn) {
int wiad = IN_SECTION;
int from = tid;
int turka = turn;
pvm_initsend(PvmDataDefault);
pvm_pkint(&wiad,1,1);
pvm_pkint(&from,1,1);
pvm_pkint(&turka,1,1);
pvm_send(master_tid,MSG_SLV);
}
//komunikat diagnostyczny
void Monitor::out_section(int tid, int turn) {
int wiad = OUT_SECTION;
int from = tid;
int turka = turn;
pvm_initsend(PvmDataDefault);
pvm_pkint(&wiad,1,1);
pvm_pkint(&from,1,1);
pvm_pkint(&turka,1,1);
pvm_send(master_tid, MSG_SLV);
}

void Monitor::val_produced(int val, int tid) {
int wiad = PRODUCED;
int from = tid;
int turka = val;
pvm_initsend(PvmDataDefault);
pvm_pkint(&wiad,1,1);
pvm_pkint(&from,1,1);
pvm_pkint(&turka,1,1);
pvm_send(master_tid, MSG_SLV);
}

void Monitor::val_consumed(int val, int tid) {
int wiad = CONSUMED;
int from = tid;
int turka = val;
pvm_initsend(PvmDataDefault);
pvm_pkint(&wiad,1,1);
pvm_pkint(&from,1,1);
pvm_pkint(&turka,1,1);
pvm_send(master_tid, MSG_SLV);
}

