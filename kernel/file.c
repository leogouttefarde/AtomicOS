
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include "string.h"
#include "cpu.h"
#include "file.h"
#include "mem.h"
#include "process.h"
#include "queue.h"

#define NBQUEUE 100

#define LOCALISATION  "[file %s, line %d]: "
#define LOCALARG  __FILE__, __LINE__
#define ERREUR(...) printf(__VA_ARGS__)
#define ERREUR1(_msg) ERREUR(LOCALISATION _msg, LOCALARG)
#define ERREUR2(_msg, ...) ERREUR(LOCALISATION _msg, LOCALARG, __VA_ARGS__)

#define ERR -1 //code d'erreur

//==============================================================================

/* File */
typedef struct file {
	int fid;
	bool isDeleted; //pour savoir si la file est supprimée
	bool isReseted; //pour savoir si la file est réinitialisée
	int *messages;
	int nextMessage;
	int windex;
	int sizeMessage;
	int sizeMessageUsed;
	int numProcReadBlocked;
	link listProcReadBlocked; //File des processus bloqués en lecture 
	                           //(à cause de la file vide)
	int numProcWriteBlocked;
	link listProcWriteBlocked; //File des processus bloqués en écriture 
	                            //(à cause de la file pleine)
	struct file *next;
} File;

/* Liste complete de files */
static File *files[NBQUEUE];

// Nombre de file créées depuis le début
static int numFileCreated = 0;

// Nombre de files disponibles
static int numFileAvailable = NBQUEUE;

typedef struct listFile {
        File *head;
        File *tail;
} ListFile;

//liste des files disponibles après la détruction
static ListFile ListAvailable = {NULL,NULL};

//==============================================================================

static File *extractHead (ListFile *plist) {
	if (plist==NULL){
		return NULL;
	}
	if (plist->head==NULL){
		return NULL;
	}
        //extraction d'une file depuis la tete d'une liste
        File *current = plist->head;
        plist->head = current->next;
        current->next=NULL;
        if (current==plist->tail) {
                plist->tail=NULL;
        }
        return current;
}

static void insertTail (ListFile *plist, File *pfile) {
	if (plist==NULL){
		return;
	}
        //Insertion en queue de la liste passée en parametre
        pfile->next=NULL;
        if (plist->tail!=NULL) {
                (plist->tail)->next=pfile;  
        }
        plist->tail=pfile;                
        if (plist->head==NULL){
                plist->head=plist->tail;
	}
}

//==============================================================================

//crée une file de messages
int pcreate(int count)
{
	//count non-valide
	if (count <= 0 || count >= 0x40000000){
		// ERREUR1("Erreur de creation\n");
		return ERR;
	}
	// printf("pcreate IN\n");

	//Si on n'a pas de files disponibles
	if (numFileAvailable <= 0){
		// ERREUR1("Erreur de creation - Erreur inattendue00\n");
		return ERR;
	}
	// printf("IN numFileAvailable = %d\n", numFileAvailable);

	File *pfile = NULL;
	if (numFileCreated < NBQUEUE){
		pfile = mem_alloc(sizeof(File));
		if(pfile == NULL){
			// ERREUR1("Erreur de creation - Erreur d'allocation de mémoire\n");
			return ERR;
		}

		pfile->messages = mem_alloc(count * sizeof(int));
		if (pfile->messages == NULL){
			mem_free_nolength(pfile);
			// ERREUR1("Erreur de creation - Erreur inattendue0\n");
			return ERR;
		}

		pfile->fid = numFileCreated;
		files[numFileCreated] = pfile;

		numFileCreated++;
	}else{
		//La réutilisation de la file déjà créée et disponible
		if (ListAvailable.head == NULL){
			// ERREUR1("Erreur de creation - Erreur inattendue\n");
			return ERR;
		}
		pfile = extractHead(&ListAvailable);

		pfile->messages = mem_alloc(count * sizeof(int));
		if (pfile->messages == NULL){
			// ERREUR1("Erreur de creation - Erreur inattendue2\n");
			return ERR;
		}
	}


	pfile->isDeleted = false;
	pfile->isReseted = false;
        
	pfile->sizeMessage = count;
	pfile->sizeMessageUsed = 0;
	pfile->nextMessage = 0;
	pfile->windex = 0;

	pfile->numProcReadBlocked = 0;

        INIT_LIST_HEAD(&pfile->listProcReadBlocked);

	pfile->numProcWriteBlocked = 0;

	INIT_LIST_HEAD(&pfile->listProcWriteBlocked);

	pfile->next = NULL;

	//Le nombre de file disponible décrémente par un
	numFileAvailable--;
	// printf("numFileAvailable = %d\n", numFileAvailable);

	// if (numFileAvailable < 4 || numFileAvailable > 98) sleep(1);

	return pfile->fid;
}

#define queue_del_safe(ppelem, attr)			\
	do {						\
		if (*(ppelem) != NULL) {		\
			queue_del(*(ppelem), attr);	\
			*(ppelem) = NULL;		\
		}					\
	} while (0)

//détruit une file de messages
int pdelete(int fid)
{
	// printf("pdelete\n");
	if (fid < 0 || fid >= NBQUEUE){
		return ERR;
	}

	File *pfile = files[fid];

	if (pfile == NULL || pfile->isDeleted){
		return ERR;
	}

	mem_free_nolength(pfile->messages);

	pfile->isDeleted = true;
	pfile->isReseted = false;
	pfile->messages = NULL;
	pfile->sizeMessage = 0;
	pfile->sizeMessageUsed = 0;
	pfile->numProcReadBlocked = 0;
	pfile->numProcWriteBlocked = 0;

	insertTail(&ListAvailable, pfile);

	numFileAvailable++;

	Process *proc = NULL, *del = NULL;
	queue_for_each(proc, &pfile->listProcReadBlocked, Process, msg_queue) {// A VOIR

		queue_del_safe(&del, msg_queue);

		if (proc != NULL) {
			// pfile->numProcReadBlocked--;
			proc->state = ACTIVABLE;
			proc->msg_reset = false;
			proc->blocked_queue = NULL;
			addProcActivable(proc);// A VOIR
			del = proc;
		}

	}
	queue_del_safe(&del, msg_queue);

	proc = NULL;
	queue_for_each(proc, &pfile->listProcWriteBlocked, Process, msg_queue) {// A VOIR

		queue_del_safe(&del, msg_queue);

		if (proc != NULL) {
			// pfile->numProcWriteBlocked--;
			proc->state = ACTIVABLE;
			proc->msg_reset = false;
			proc->blocked_queue = NULL;
			addProcActivable(proc);// A VOIR
			del = proc;
		}
	}
	queue_del_safe(&del, msg_queue);

	ordonnance();


	//si c'était soi-même à libérer?? //normelement non, car ce processus actuel n'est pas bloqué

	//le retour pour tous les processus libérés?? //A MODIF c'est fait car après chaque STI on a faitdes tests

	return 0;
}

//dépose un message dans une file
int psend(int fid, int message)
{
	//A MODIF -> A FACTORISER
	if (fid < 0 || fid >= NBQUEUE) {
		return ERR;
	}

	File *pfile = files[fid];

	if (pfile == NULL || pfile->isDeleted) {
		return ERR;
	}
        // printf("psend %d\n", message);

	// printf("psend : %c, count = %d\n", pfile->messages[pfile->nextMessage], pfile->sizeMessageUsed);


	if(pfile->sizeMessageUsed == pfile->sizeMessage) {
		Process *proc = NULL;

		if(pfile->numProcReadBlocked != 0) {
			proc = queue_out(&pfile->listProcReadBlocked, 
					 Process, msg_queue);
			assert(proc);
			pfile->numProcReadBlocked--;
			proc->blocked_queue = NULL;
			proc->state = ACTIVABLE;
			addProcActivable(proc);
			// ordonnance();
		}

		proc = pidToProc(getpid()); // A MODIF
		proc->state = WAITMSG; // A VOIR
		proc->msg_count = &pfile->numProcWriteBlocked;
		proc->blocked_queue = &pfile->listProcWriteBlocked;
		queue_add(proc, &pfile->listProcWriteBlocked,
			  Process, msg_queue, prio);
		pfile->numProcWriteBlocked++;
// printf("pfile->numProcWriteBlocked %d\n", pfile->numProcWriteBlocked);
		ordonnance();
		// printf("psend continue : %c, count = %d\n", pfile->messages[pfile->nextMessage], pfile->sizeMessageUsed);

		//si preset ou pdelete qui rend ce processus activable retour -1 ?? //A MODIF
		if(pfile->isDeleted || proc->msg_reset){
			proc->msg_reset = false;
			// printf("psend %d reset or delete\n", message);
			return ERR;
		}
	}

	if(pfile->sizeMessageUsed == 0) {
		if(pfile->numProcReadBlocked != 0) {
			//A FACTORISER
			assert(pfile->windex < pfile->sizeMessage);
			pfile->messages[pfile->windex] = message;
			pfile->sizeMessageUsed++;

			pfile->windex = (pfile->windex + 1) % pfile->sizeMessage;

			Process *proc;
			proc = queue_out(&pfile->listProcReadBlocked, 
					 Process, msg_queue);
			assert(proc);
			pfile->numProcReadBlocked--;
			proc->blocked_queue = NULL;
			proc->state = ACTIVABLE;// A VOIR
			addProcActivable(proc);// A VOIR
			ordonnance();

			return 0;
		}
	}

	//A cet état, le processus actuel (qui peut être celui qui vient d'être débloqué)
	//peut maintenant déposer un message
	assert(pfile->windex < pfile->sizeMessage);
	pfile->messages[pfile->windex]=message;
	pfile->windex = (pfile->windex + 1) % pfile->sizeMessage;
	pfile->sizeMessageUsed++;
	// printf("psend continue_end : %c, count = %d\n", pfile->messages[pfile->nextMessage], pfile->sizeMessageUsed);

	return 0;
}

//retire un message d'une file
int preceive(int fid, int *message)
{
	if (fid<0 || fid >= NBQUEUE){
		return ERR;
	}
// printf("preceive IN\n");

	File *pfile = files[fid];

	if (pfile == NULL || pfile->isDeleted){
		return ERR;
	}
	// printf("preceive : %c, count = %d\n", pfile->messages[pfile->nextMessage], pfile->sizeMessageUsed);
// printf("preceive IN2\n");

	//Si pas de message, le processus actuel passe à l'état WAITMSG et
	//on passe au processus activable suivant
	if(pfile->sizeMessageUsed == 0) {
		Process *proc = NULL;

		if(pfile->numProcWriteBlocked != 0) {
			proc = queue_out(&pfile->listProcWriteBlocked, 
					 Process, msg_queue);
			assert(proc);
			pfile->numProcWriteBlocked--;
			proc->blocked_queue = NULL;
			proc->state = ACTIVABLE;// A VOIR ou ACTIF selon la prio
			addProcActivable(proc);// A VOIR ou ACTIF selon la prio
			// ordonnance();
		}
		// else {
		proc = pidToProc(getpid()); // A MODIF
		proc->state = WAITMSG; // A VOIR
		proc->msg_count = &pfile->numProcReadBlocked;
		proc->blocked_queue = &pfile->listProcReadBlocked;
		queue_add(proc, &pfile->listProcReadBlocked,
			  Process, msg_queue, prio);
		pfile->numProcReadBlocked++;
		ordonnance();
		// }

		//si preset ou pdelete qui rend ce processus activable retour -1 ?? //A MODIF
		if(pfile->isDeleted || proc->msg_reset) {
			proc->msg_reset = false;
// printf("preceive OUT1\n");
			return ERR;
		}

		//Un processus bloqué sur file vide et dont la priorité est changée par chprio, est considéré comme le dernier processus (le plus jeune) de sa nouvelle priorité. //A MODIF
	}
	// printf("preceive_end : %c, count = %d\n", pfile->messages[pfile->nextMessage], pfile->sizeMessageUsed);

	pfile->sizeMessageUsed--;
	assert(pfile->nextMessage < pfile->sizeMessage);

	if (message != NULL) {

		//La lecture du bon message
		*message = pfile->messages[pfile->nextMessage];
		pfile->nextMessage = (pfile->nextMessage + 1) % pfile->sizeMessage;
// printf("preceive %d\n", *message);
	// }else {
// printf("preceive no message\n");
	}

	//Si la file était pleine avant la lecture précédente
	if(pfile->sizeMessageUsed+1 == pfile->sizeMessage){
// printf("réveil écrivain pre\n");
// printf("pfile->numProcWriteBlocked %d\n", pfile->numProcWriteBlocked);
		if(pfile->numProcWriteBlocked != 0){
// printf("réveil écrivain\n");
			Process *proc;
			proc = queue_out(&pfile->listProcWriteBlocked, 
					 Process, msg_queue);
			assert(proc);
			pfile->numProcWriteBlocked--;
			proc->blocked_queue = NULL;
			proc->state = ACTIVABLE;// A VOIR ou ACTIF selon la prio
			addProcActivable(proc);// A VOIR ou ACTIF selon la prio
			ordonnance();
			return 0;
		}
	}

	return 0;
}

//réinitialise une file
int preset(int fid)
{
	if (fid<0 || fid >= NBQUEUE){
		return ERR;
	}

	File *pfile = files[fid];
	if (pfile == NULL){
		return ERR;
	}
	if(pfile->isDeleted){
		return ERR;
	}

	pfile->isDeleted = false;
	pfile->isReseted = true;
	pfile->sizeMessageUsed = 0;
	pfile->numProcReadBlocked = 0;
	pfile->numProcWriteBlocked = 0;
	
	Process *proc = NULL, *del = NULL;
	queue_for_each(proc, &pfile->listProcReadBlocked, Process, msg_queue) {// A VOIR
		queue_del_safe(&del, msg_queue);

		if (proc != NULL) {
			proc->state = ACTIVABLE;
			proc->blocked_queue = NULL;
			proc->msg_reset = true;
			addProcActivable(proc);// A VOIR
			del = proc;
		}
	}
	queue_del_safe(&del, msg_queue);

	proc = NULL;
	queue_for_each(proc, &pfile->listProcWriteBlocked, Process, msg_queue) {// A VOIR
		queue_del_safe(&del, msg_queue);

		if (proc != NULL) {
			proc->state = ACTIVABLE;
			proc->blocked_queue = NULL;
			proc->msg_reset = true;
			addProcActivable(proc);// A VOIR
			del = proc;
		}
	}
	queue_del_safe(&del, msg_queue);

	ordonnance();

	//si c'était soi-même à libérer?? //normelement non, car ce processus actuel n'est pas bloqué

	//le retour pour tous les processus libérés?? //A MODIF c'est fait car après chaque STI on a faitdes tests
	
	return 0;
}

//renvoie l'état courant d'une fil
int pcount(int fid, int *count)
{
	if (fid < 0 || fid >= NBQUEUE) {
		return ERR;
	}

	File *pfile = files[fid];

	if (pfile == NULL || pfile->isDeleted) {
		return ERR;
	}

	if (count != NULL) {
		if (pfile->numProcReadBlocked > 0)
			*count = -pfile->numProcReadBlocked;

		else
			*count = pfile->sizeMessageUsed + pfile->numProcWriteBlocked;
	}

	return 0;
}

