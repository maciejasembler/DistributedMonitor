#include <stdio.h>
#include <stdlib.h>
#include "semafor.h"
#include "pvm3.h"
#include <unistd.h>
#include <sys/time.h> 
#define GRUPA "grupa"
#define SEM_ALLOW 3000
#define SEM_ACQUIRE 3001

/*
*Konstruktorek
*/
Semafor::Semafor(int n,int master,int tid_table[], int id, int lim, int slvtid) {
	nproc = n;
	master_tid = master;
	tids = tid_table;
	my_id = id;
	limit = lim;
	mytid = slvtid;
	tura = 0;
	sem_init(&mutex,0,0);
//	sem_init(&enter_mutex,0,1);
	allows = 0;
	section = 0;
	want = 0;
}

void Semafor::semaforEnter() {
//broadcast i semafor
//allows = 0;
want = 1; //zgloszenie checi wejscia do sekcji
Semafor::broadcastAcquire(); //wyslanie broadcatu do monitorow
sem_wait(&mutex); //wait na opuszczonym z poczatku mutexie - usypia watek az do odebrania dostatecznej ilosci zgod
}

void Semafor::broadcastAcquire() {
int i;
int type = SEM_ACQUIRE;
for (i=0;i<nproc;i++) {
if (tids[i]!=mytid) { //wyslanie do kazdego procz siebie, bo po co
pvm_initsend(PvmDataDefault);
pvm_pkint(&mytid,1,1);
pvm_pkint(&type,1,1);
pvm_pkint(&tura,1,1);
pvm_pkint(&my_id,1,1);
pvm_send(tids[i], my_id);
}
}
}

void Semafor::acquire() { //metoda acquire, tworzy watek, ktory wysyla chec wejscia do sekcji
allows = 0;
pthread_create(&thread_a, NULL, semafor_enter_proxy, this);
pthread_join(thread_a, NULL);
}

void Semafor::release() { //zwolnienie semafora
section = 0;
want = 0;
tura++;
Semafor::releaseQueue(); //zwolnienie kolejki
}

void Semafor::sendAllow(int who, int turn) { //wyslanie zgody
int type = SEM_ALLOW;
pvm_initsend(PvmDataDefault);
pvm_pkint(&mytid,1,1);
pvm_pkint(&type,1,1);
pvm_pkint(&turn,1,1); //tutaj ta tura to nie jest tura tego obiektu, to tura odebrana z zadania do wejscia do sekcji
pvm_pkint(&my_id,1,1);
pvm_send(who, my_id);
}

void Semafor::releaseQueue() { 
while (!kolejka.empty()) { //wiec o co jol tutaj
//w liste pakuje zawsze 2 integery, wpierw tida i potem ture zadania sekcji
int rcver = kolejka.front();
kolejka.pop();//pobranie tida i przeskoczenie do next elementu
int trn = kolejka.front();
kolejka.pop(); //pobranie tury i next element
Semafor::sendAllow(rcver, trn); //odeslanie zgody
//a to wszystko przez to ze nie chcialo mi sie robic nowego obiektu/structa
}

}
//dodanie do kolejki oczekujacych
void Semafor::addToQueue(int who, int turn) {
kolejka.push(who);
kolejka.push(turn);
}

void* semafor_enter_proxy(void* ptr) {
	static_cast<Semafor*>(ptr)->semaforEnter();
}
//wejscie do sekcji
void Semafor::signal() {
if (section==0) {
allows = 0; //reset allowsow
section = 1; //sekcja
sem_post(&mutex); //podnosi z poczatku opuszczony semafor
//powoduje ze watek acquire sie zakonczy i monitor ruszy dalej
}
}
