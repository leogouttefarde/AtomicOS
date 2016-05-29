
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include "string.h"
#include "cpu.h"
#include "messages.h"
#include "mem.h"
#include "process.h"
#include "queue.h"

#define NB_QUEUES 100
#define MAX_QUEUE_SIZE 0x4000000

// MsgQueue
typedef struct MsgQueue_ {
	int id;

	bool is_dead;
	int *messages;

	int rindex;
	int windex;

	int size;
	int nb_messages;

	int nb_readers;
	int nb_writers;

	link readers;
	link writers;

	struct MsgQueue_ *next;
} MsgQueue;

// Liste complete de files
static MsgQueue *queues[NB_QUEUES];

// Nombre de file créées depuis le début
static int created_queues = 0;

// Nombre de files disponibles
static int remaining_queues = NB_QUEUES;

typedef struct MsgList_ {
	MsgQueue *head;
	MsgQueue *tail;
} MsgList;

//liste des files disponibles après la détruction
static MsgList list_queues = { NULL, NULL };

static inline MsgQueue *extractHead (MsgList *list)
{
	if (list == NULL || list->head == NULL) {
		return NULL;
	}

	MsgQueue *current = list->head;

	list->head = current->next;
	current->next = NULL;

	if (current == list->tail) {
		list->tail = NULL;
	}

	return current;
}

static inline void insertTail (MsgList *list, MsgQueue *queue)
{
	if (list != NULL) {
		queue->next = NULL;

		if (list->tail != NULL) {
			list->tail->next = queue;  
		}

		list->tail = queue;  

		if (list->head == NULL) {
			list->head = list->tail;
		}
	}
}

static inline bool is_invalid_fid(int fid)
{
	return (fid < 0 || fid >= NB_QUEUES);
}

static inline bool is_invalid_queue(MsgQueue *queue)
{
	return (queue == NULL || queue->is_dead);
}

static inline MsgQueue *get_queue(int fid)
{
	if (is_invalid_fid(fid)) {
		return NULL;
	}

	MsgQueue *queue = queues[fid];

	if (is_invalid_queue(queue)) {
		return NULL;
	}

	return queue;
}

void init_messages()
{
	memset(&queues, 0, sizeof(queues));
}

//crée une file de messages
int pcreate(int count)
{
	// On vérifie la validité de la capacité
	if (count <= 0 || count > MAX_QUEUE_SIZE) {
		return -1;
	}

	// Plus de files disponibles
	if (remaining_queues <= 0) {
		return -1;
	}

	MsgQueue *queue = NULL;

	// On réutilise en priorité les anciennes files
	if (list_queues.head != NULL) {
		queue = extractHead(&list_queues);
		queue->messages = mem_alloc(count * sizeof(int));

		if (queue->messages == NULL) {
			insertTail(&list_queues, queue);
			return -1;
		}

	} else if (created_queues < NB_QUEUES) {
		queue = mem_alloc(sizeof(MsgQueue));

		if(queue == NULL) {
			return -1;
		}

		queue->messages = mem_alloc(count * sizeof(int));

		if (queue->messages == NULL) {
			mem_free_nolength(queue);
			return -1;
		}

		queue->id = created_queues;
		queues[created_queues++] = queue;
	}
	else {
		return -1;
	}

	queue->is_dead = false;
	queue->nb_messages = 0;
	queue->size = count;
	queue->rindex = 0;
	queue->windex = 0;

	queue->nb_readers = 0;
	INIT_LIST_HEAD(&queue->readers);

	queue->nb_writers = 0;
	INIT_LIST_HEAD(&queue->writers);

	queue->next = NULL;

	remaining_queues--;

	return queue->id;
}

static inline void force_reload_proc(Process *proc)
{
	if (proc != NULL) {
		proc->has_msg_error = true;
		proc->blocked_queue = NULL;
		add_proc_activable(proc);
	}
}

//détruit une file de messages
int pdestroy(int fid, bool is_delete)
{
	MsgQueue *queue = get_queue(fid);
	Process *proc = NULL;

	if (queue == NULL) {
		return -1;
	}

	queue->is_dead = is_delete;
	queue->nb_messages = 0;
	queue->nb_readers = 0;
	queue->nb_writers = 0;

	if (is_delete) {
		mem_free_nolength(queue->messages);
		queue->messages = NULL;
		queue->size = 0;

		insertTail(&list_queues, queue);
		remaining_queues++;
	}

	queue_for_each(proc, &queue->readers, Process, msg_queue)
		force_reload_proc(proc);

	queue_for_each(proc, &queue->writers, Process, msg_queue)
		force_reload_proc(proc);

	queue_clean(&queue->readers, Process, msg_queue);
	queue_clean(&queue->writers, Process, msg_queue);

	ordonnance();

	return 0;
}

//réinitialise une file
int preset(int fid)
{
	return pdestroy(fid, false);
}

int pdelete(int fid)
{
	return pdestroy(fid, true);
}

static inline int check_errors()
{
	if(get_cur_proc()->has_msg_error) {
		get_cur_proc()->has_msg_error = false;
		return -1;
	}

	return 0;
}

static int pop_reader(MsgQueue *queue, int message)
{
	Process *proc;
	proc = queue_out(&queue->readers,
			 Process, msg_queue);

	assert(proc);
	if (proc != NULL) {
		proc->message = message;
		proc->blocked_queue = NULL;

		add_proc_activable(proc);
		queue->nb_readers--;

		ordonnance();

		return check_errors();
	}

	return 0;
}

static void pop_writer(MsgQueue *queue, int *message)
{
	Process *proc = queue_out(&queue->writers, 
			 Process, msg_queue);

	assert(proc);
	if (proc != NULL) {
		queue->nb_writers--;
		proc->blocked_queue = NULL;

		if (message != NULL)
			*message = proc->message;

		add_proc_activable(proc);
	}
}

static inline void queue_wait(link *queue, int *count)
{
	Process *proc = get_cur_proc();

	proc->state = WAITMSG;
	proc->msg_counter = count;
	proc->blocked_queue = queue;

	queue_add(proc, queue, Process, msg_queue, prio);
	(*count)++;

	ordonnance();
}

static inline void write_message(MsgQueue *queue, int message)
{
	const int iw = queue->windex;
	const int size = queue->size;

	assert(iw < size);
	if (iw < size) {
		queue->messages[iw] = message;
		queue->windex = (iw + 1) % size;
		queue->nb_messages++;
	}
}

//dépose un message dans une file
int psend(int fid, int message)
{
	MsgQueue *queue = get_queue(fid);
	int ret = 0;

	if (queue == NULL) {
		return -1;
	}

	if(queue->nb_messages == queue->size) {

		if (queue->nb_readers != 0) {
			ret = pop_reader(queue, message);
		}
		else {
			get_cur_proc()->message = message;
			queue_wait(&queue->writers, &queue->nb_writers);

			ret = check_errors();
		}
	}

	else if (queue->nb_messages == 0 && queue->nb_readers) {
		ret = pop_reader(queue, message);
	}

	else {
		//A cet état, le processus actuel (qui peut être celui qui vient d'être débloqué)
		//peut maintenant déposer un message
		write_message(queue, message);
	}


	return ret;
}

//retire un message d'une file
int preceive(int fid, int *message)
{
	MsgQueue *queue = get_queue(fid);

	if (queue == NULL) {
		return -1;
	}

	//Si pas de message, le processus actuel passe à l'état WAITMSG et
	//on passe au processus activable suivant
	if(queue->nb_messages == 0) {

		if(queue->nb_writers != 0) {
			pop_writer(queue, message);
			ordonnance();
		}
		else {
			queue_wait(&queue->readers, &queue->nb_readers);

			if (message != NULL)
				*message = get_cur_proc()->message;
		}

		return check_errors();
	}

	if (message != NULL) {
		assert(queue->rindex < queue->size);

		// Lecture du bon message
		*message = queue->messages[queue->rindex];
		queue->rindex = (queue->rindex + 1) % queue->size;
	}

	queue->nb_messages--;

	//Si la file était pleine avant la lecture précédente
	if(queue->nb_messages+1 == queue->size && queue->nb_writers) {
		int message;

		pop_writer(queue, &message);
		write_message(queue, message);

		ordonnance();

		return check_errors();
	}

	return 0;
}

//renvoie l'état courant d'une fil
int pcount(int fid, int *count)
{
	MsgQueue *queue = get_queue(fid);

	if (queue == NULL) {
		return -1;
	}

	if (count != NULL) {
		if (queue->nb_readers > 0)
			*count = -queue->nb_readers;

		else
			*count = queue->nb_messages + queue->nb_writers;
	}

	return 0;
}

