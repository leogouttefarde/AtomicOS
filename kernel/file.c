
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
	int sizeMessage;
	int sizeMessageUsed;
	int numProcReadBlocked;
	link *listProcReadBlocked; //File des processus bloqués en lecture 
	                           //(à cause de la file vide)
	int numProcWriteBlocked;
	link *listProcWriteBlocked; //File des processus bloqués en écriture 
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
	if (count <= 0){
		ERREUR1("Erreur de creation\n");
		return ERR;
	}

	//Si on n'a pas de files disponibles
	if (numFileAvailable <= 0){
		return ERR;
	}

	File *pfile;
	if (numFileCreated < NBQUEUE){
		pfile = mem_alloc(sizeof(File));
		if(pfile == NULL){
			ERREUR1("Erreur de creation - Erreur d'allocation de mémoire\n");
			return ERR;
		}
		numFileCreated++;

		pfile->fid = numFileCreated;
		files[numFileCreated] = pfile;
	}else{
		//La réutilisation de la file déjà créée et disponible
		if (ListAvailable.head == NULL){
			ERREUR1("Erreur de creation - Erreur inattendue");
			return ERR;
		}
		pfile = extractHead(&ListAvailable);
	}	

	pfile->messages = mem_alloc(count*sizeof(int));
	if (pfile->messages == NULL){
		ERREUR1("Erreur de creation - Erreur d'allocation de mémoire\n");
		if (numFileCreated < NBQUEUE){
			mem_free_nolength(pfile);
			numFileCreated--;
		}
		return ERR;
	}

	pfile->isDeleted = false;
	pfile->isReseted = false;
        
	pfile->sizeMessage = count;
	pfile->sizeMessageUsed = 0;

	pfile->numProcReadBlocked = 0;
	
	pfile->listProcReadBlocked = mem_alloc(sizeof(link));
        INIT_LIST_HEAD(pfile->listProcReadBlocked);

	pfile->numProcWriteBlocked = 0;

	pfile->listProcWriteBlocked = mem_alloc(sizeof(link));
	INIT_LIST_HEAD(pfile->listProcWriteBlocked);

	pfile->next = NULL;

	//Le nombre de file disponible décrémente par un
	numFileAvailable--;

	return pfile->fid;
}
    
//détruit une file de messages
int pdelete(int fid)
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
        
	mem_free_nolength(pfile->messages);
	
	pfile->isDeleted = true;
	pfile->isReseted = false;
	pfile->messages = NULL;
	pfile->sizeMessage = 0;
	pfile->sizeMessageUsed = 0;
	pfile->numProcReadBlocked = 0;
	pfile->numProcWriteBlocked = 0;

	insertTail(&ListAvailable, pfile);
	
	Process *proc = NULL;
	queue_for_each(proc, pfile->listProcReadBlocked, Process, queueRead) {// A VOIR
		if (proc != NULL) {
			pfile->numProcReadBlocked--;
			queue_del(proc, queueRead);
			proc->state = ACTIVABLE;
			addProcActivable(proc);// A VOIR
		}
	}
	
	proc = NULL;
	queue_for_each(proc, pfile->listProcWriteBlocked, Process, queueWrite) {// A VOIR
		if (proc != NULL) {
			pfile->numProcWriteBlocked--;
			queue_del(proc, queueWrite);
			proc->state = ACTIVABLE;
			addProcActivable(proc);// A VOIR
		}
	}

	
	//si c'était soi-même à libérer?? //normelement non, car ce processus actuel n'est pas bloqué

	//le retour pour tous les processus libérés?? //A MODIF c'est fait car après chaque STI on a faitdes tests

	return 0;
}

//dépose un message dans une file
int psend(int fid, int message)
{
	//A MODIF -> A FACTORISER
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

	if(pfile->sizeMessageUsed == 0){
		if(pfile->numProcReadBlocked != 0){
			//A FACTORISER
			pfile->messages[pfile->sizeMessageUsed] = message;
			pfile->sizeMessageUsed++;

			Process *proc;
			proc = queue_out(pfile->listProcReadBlocked, 
					 Process, queueRead);
			assert(proc);
			pfile->numProcReadBlocked--;
			proc->state = ACTIVABLE;// A VOIR
			addProcActivable(proc);// A VOIR

			return 0;
		}
	}else if(pfile->sizeMessageUsed == pfile->sizeMessage){
		Process *proc = pidToProc(getpid()); // A MODIF
		proc->state = WAITMSG; // A VOIR
		queue_add(proc, pfile->listProcWriteBlocked,
			  Process, queueWrite, prio);
		pfile->numProcWriteBlocked++;
		ordonnance();

		//si preset ou pdelete qui rend ce processus activable retour -1 ?? //A MODIF
		if(pfile->isDeleted || pfile->isReseted){
			return ERR;
		}
	}
	
	//A cet état, le processus actuel (qui peut être celui qui vient d'être débloqué)
	//peut maintenant déposer un message
	pfile->messages[pfile->sizeMessageUsed]=message;
	pfile->sizeMessageUsed++;

	return 0;
}

//retire un message d'une file
int preceive(int fid,int *message)
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

	//Si pas de message, le processus actuel passe à l'état WAITMSG et
	//on passe au processus activable suivant
	if(pfile->sizeMessageUsed == 0){
		Process *proc = pidToProc(getpid()); // A MODIF
		proc->state = WAITMSG; // A VOIR
		queue_add(proc, pfile->listProcReadBlocked,
			  Process, queueRead, prio);
		pfile->numProcReadBlocked++;
		ordonnance();

		//si preset ou pdelete qui rend ce processus activable retour -1 ?? //A MODIF
		if(pfile->isDeleted || pfile->isReseted){
			return ERR;
		}

		//Un processus bloqué sur file vide et dont la priorité est changée par chprio, est considéré comme le dernier processus (le plus jeune) de sa nouvelle priorité. //A MODIF
	}
	
	//La lecture du message
	*message=pfile->messages[0];
	if(pfile->sizeMessage > 1){
		memmove(pfile->messages, (pfile->messages)+1, (pfile->sizeMessage-1)*(sizeof(int)));
	}
	(pfile->sizeMessageUsed)--;

	//Si la file était pleine avant la lecture précédente
	if(pfile->sizeMessageUsed+1 == pfile->sizeMessage){
		if(pfile->numProcWriteBlocked != 0){
			Process *proc;
			proc = queue_out(pfile->listProcWriteBlocked, 
					 Process, queueRead);
			assert(proc);
			pfile->numProcWriteBlocked--;
			proc->state = ACTIVABLE;// A VOIR ou ACTIF selon la prio
			addProcActivable(proc);// A VOIR ou ACTIF selon la prio
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
	
	Process *proc = NULL;
	queue_for_each(proc, pfile->listProcReadBlocked, Process, queueRead) {// A VOIR
		if (proc != NULL) {
			pfile->numProcReadBlocked--;
			queue_del(proc, queueRead);
			proc->state = ACTIVABLE;
			addProcActivable(proc);// A VOIR
		}
	}
	
	proc = NULL;
	queue_for_each(proc, pfile->listProcWriteBlocked, Process, queueWrite) {// A VOIR
		if (proc != NULL) {
			pfile->numProcWriteBlocked--;
			queue_del(proc, queueWrite);
			proc->state = ACTIVABLE;
			addProcActivable(proc);// A VOIR
		}
	}

	//si c'était soi-même à libérer?? //normelement non, car ce processus actuel n'est pas bloqué

	//le retour pour tous les processus libérés?? //A MODIF c'est fait car après chaque STI on a faitdes tests
	
	return 0;
}

//renvoie l'état courant d'une fil
int pcount(int fid, int *count)
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

	if (count != NULL){ // A MODIF
		if(pfile->sizeMessageUsed == 0){
			*count = -1*(pfile->numProcReadBlocked);
		}else{
			*count = (pfile->sizeMessageUsed)+(pfile->numProcWriteBlocked);
		}
	}

	return 0;
}

