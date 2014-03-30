#include <stdio.h>
#include "pvm3.h"
#define NPROC 10
#define GRUPA "grupa"
#define MSG_MSTR 20001
#define MSG_SLV  20000
#define SEM_ACQ 61
#define SEM_ALLOW 62
#define IN_SECTION 10000
#define OUT_SECTION 10001
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

int main(int argc, char **argv) {
	int i;
	int who;
	int mytid = pvm_mytid();
	int tids[NPROC];
	int inum = pvm_joingroup(GRUPA);
	int nproc = pvm_spawn("slave", NULL, PvmTaskDefault, 0, NPROC, tids);
	printf("(MASTER)  %d processes created: ", nproc);
	for (i=0; i<NPROC; i++)
	printf("%d ", tids[i]);
	printf("\n");

	pvm_initsend(PvmDataDefault); //inicjacja bufora
	pvm_pkint(&mytid, 1, 1); 
	pvm_barrier(GRUPA, NPROC + 1); //bariera synchronizacyjna 1 kazdy musi odebrac tid mastera
	pvm_bcast(GRUPA, MSG_MSTR);

	pvm_initsend(PvmDataDefault); 
	pvm_pkint(tids, NPROC, 1);
	pvm_barrier(GRUPA, NPROC + 1); //bariera synchronizacyjna kazdy slave ma miec tidsy do odsylania
	pvm_bcast(GRUPA, MSG_MSTR);

	for (i = 0; i < NPROC; i++) {
		int tmp[NPROC];
		pvm_recv(-1, MSG_SLV);
		pvm_upkint(&who, 1, 1);
		pvm_upkint(tmp, NPROC, 1);
		printf("%d: dostalem tids %d\n", who, tmp[i]);
	}
	pvm_barrier(GRUPA, NPROC + 1); //kazdy musi zespawnowac semafory i watki
	int ilosc = 0;   //output z sekcji
	while (1) {
		pvm_recv(-1, MSG_SLV);
		int src_tid;
		int tura;
		int typ;
		int komu;
		int zgody;
		pvm_upkint(&typ,1,1);
		pvm_upkint(&src_tid, 1, 1);
		pvm_upkint(&tura,1,1);
		//pvm_upkint(&komu,1,1);
		//pvm_upkint(&zgody,1,1);
		switch (typ) {
		case IN_SECTION:
		ilosc++;
		printf("proces %d wszedl do sekcji tura %d ilosc %d\n",src_tid,tura,ilosc);
		break;
		case OUT_SECTION:
		ilosc--;
		printf("proces %d wyszedl z sekcji tura %d ilosc %d\n",src_tid,tura,ilosc);
		break;
		case PHILOSOPHY_SECT:
		ilosc++;
		printf("filozof %d wcionga rogale\n",src_tid);
		break;
                case PHILOSOPHY_OUT:
		ilosc--;
                printf("filozof %d mysli\n",src_tid);
                break;
		case PHILOSOPHY_TABLE_IN:
		ilosc++;
		printf("przy stole %d\n",ilosc);
		break;
                case PHILOSOPHY_TABLE_OUT:
                ilosc--;
                printf("przy stole %d\n",ilosc);
                break;
		case PRODUCED:
                printf("producent %d zapakowal %d do bufora\n",src_tid, tura);
		break;
                case CONSUMED:
                printf("konsument %d zjadl %d z bufora\n",src_tid, tura);
		break;
		case READING:
                printf("czytarz %d czyta\n",src_tid);
                break;
                case WRITING:
                printf("pisarz %d pisze\n",src_tid);
                break;
		case READING_OUT:
                printf("czytarz %d wyszedl\n",src_tid);
                break;
                case WRITING_OUT:
                printf("pisarz %d wyszedl\n",src_tid);
                break;
		}
	}

	pvm_exit();
	return 0;
}
