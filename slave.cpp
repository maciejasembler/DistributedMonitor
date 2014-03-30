#include "monitor.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h> 
#include "pvm3.h"
#define MSG_MSTR 20001
#define MSG_SLV  20000
#define NPROC 10
#define GRUPA "grupa"

int main(int argc, char **argv) {
	int id_mine;
	int groupid;
	int id_master;
	int tids[NPROC];
	int oczekujacy[NPROC];
	int y;
	int i;
	for (y = 0; y < NPROC; y++)
	{
		oczekujacy[y] = 0;
	}
	int mytid = pvm_mytid();
	long seed = time(NULL);
	/* slave task ids */
	
	groupid = pvm_joingroup(GRUPA);
	pvm_barrier(GRUPA, NPROC + 1); //bariera synchronizacyjna 1  
	pvm_recv(-1, MSG_MSTR);
	pvm_upkint(&id_master, 1, 1);
	
	pvm_barrier(GRUPA, NPROC + 1); //bariera synchronizacyjna 3.	
	pvm_recv(-1, MSG_MSTR);
	pvm_upkint(tids, NPROC, 1);

	pvm_initsend(PvmDataDefault);
	pvm_pkint(&groupid, 1, 1);
	pvm_pkint(tids, NPROC, 1);
	pvm_send(id_master, MSG_SLV);

	pvm_barrier(GRUPA, NPROC + 1); 
	int z = 0;
	//zasadniczy kod tutaj, tworzymy monitor i korzystamy z jego metod
	//do przetwarzania rozproszonego
	Monitor *M = new Monitor(NPROC, id_master, tids, mytid);
	M->run_listener(); //uruchamiamy watek sluchajacy

	//### przyklad1 - semafor przepuszczajacy n procesow
	/*while(1) {
	M->go();
	}*/
	

	//#### przyklad2 - jedzacy filozofowie
	/*while(1) {
	M->philosophy(groupid-1);
	}*/
	

	//#### przyklad3 - producent-konsument
	/*while(1) {
	if (groupid>3)
	M->produce(groupid);
	if (groupid<=3)
	M->consume();
	usleep(10000);
	}*/

	//#### przyklad4 - czytelnicy i pisarze
	while(1) {
	if (groupid>3)
	M->reader();
	else
	M->writer();
	usleep(10000);
	}

	while(1) { //jak nie koncza sie jednoczesnie to lepiej zebu tu wisialy
			//normalnie myslalem ze pvm_barrier przy m->destroy zalatwia sprawe
			//ale on te procesy zawiesz tak, ze nawet nie listenuja na komunikaty
	usleep(100000);
	}
	M->destroy(); //wylaczenie monitora (pvm_barrier blokuje ten proces na amen wiec rozwazam wywalenie tego)
	pvm_exit(); //wyjscie
	return 0;
}

