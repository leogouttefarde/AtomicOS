/**
 *  @file       linkedlist.c
 *  @brief      Doubly linked list generic class.
 */

#include "linkedlist.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "mem.h"


void ll_init(linkedlist_t *list)
{
	if (list != NULL) {
		list->head = NULL;
		list->tail = NULL;
	}
}

void ll_add(linkedlist_t *list, elem_t elem)
{
	if (elem && list) {
		linked_elem_t *link = mem_alloc(sizeof(linked_elem_t));
		link->elem = elem;
		link->next = NULL;

		/* If there is a tail, chain after it */
		if (list->tail) {
			link->prev = list->tail;
			list->tail->next = link;
			list->tail = link;
		}

		/* Else, unique element */
		else {
			list->head = link;
			list->tail = link;
		}
	}
}

bool ll_has(linkedlist_t *list, elem_t elem)
{
	bool has = false;

	if (list) {
		linked_elem_t *link = list->head;

		/* Check if any element corresponds to the searched one */
		while (link && !has) {
			if (elem == link->elem)
				has = true;

			link = link->next;
		}
	}

	return has;
}

void ll_add_unique(linkedlist_t *list, elem_t elem)
{
	if (!ll_has(list, elem))
		ll_add(list, elem);
}

/**
 * \brief       The ll_pop_elem_link callback parameters.
 */
typedef struct linkedlist_pop_elem_t {
	linkedlist_t *list;
	elem_t elem;
	bool free_elem;
} linkedlist_pop_elem_t;

/**
 * \brief       Removes a link from a list if it matches a specific element.
 *
 * @param       link            The link to remove.
 * @param       user_param      A valid linkedlist_pop_elem_t pointer.
 *
 * @return                      Returns true once the element has been
 *                              removed, else false.
 */
static bool ll_pop_elem_link(linked_elem_t *link, void *user_param)
{
	bool done = false;

	linkedlist_pop_elem_t *params = (linkedlist_pop_elem_t*)user_param;
	if (link->elem == params->elem) {
		ll_pop_link(params->list, link, params->free_elem);
		done = true;
	}

	return done;
}

void ll_pop_elem(linkedlist_t *list, elem_t elem, bool free_elem)
{
	linkedlist_pop_elem_t params = { list, elem, free_elem };
	ll_applyfunc(list, ll_pop_elem_link, (void*)&params);
}

void ll_pop_link(linkedlist_t *list, linked_elem_t *link, bool free_elem)
{
	if (list && link) {
		linked_elem_t *prev = link->prev;
		linked_elem_t *next = link->next;


		// Random insider elem
		if (prev && next) {
			prev->next = next;
			next->prev = prev;
		}

		// Head
		else if (!prev && next) {

			// Check
			if (list->head == link) {
				next->prev = prev;
				list->head = next;
			}
		}

		// Tail
		else if (prev && !next) {

			// Check
			if (list->tail == link) {
				prev->next = NULL;
				list->tail = prev;
			}
		}

		// Both head and tail : unique elem
		else {
			list->head = NULL;
			list->tail = NULL;
		}

		// If unique element, remove all links
		if (list->head && (list->head == list->tail)) {
			list->head->next = NULL;
			list->head->prev = NULL;
		}

		if (free_elem)
			mem_free(link->elem, sizeof(Process));

		mem_free(link, sizeof(linked_elem_t));
	}
}

void ll_empty(linkedlist_t *list, bool free_elem)
{
	if (list) {
		linked_elem_t *link = list->head, *next = NULL;

		/* Free all links */
		while (link) {
			if (free_elem)
				mem_free(link->elem, sizeof(Process));

			next = link->next;
			mem_free(link, sizeof(linked_elem_t));
			link = next;
		}

		list->head = NULL;
		list->tail = NULL;
	}
}

void ll_applyfunc(linkedlist_t *list, function_t function, void *user_param)
{
	if (list) {
		bool done = false;
		linked_elem_t *link = list->head, *next = NULL;

		/* Apply the function to all links until true is returned */
		while (link && !done) {
			next = link->next;

			done = function(link, user_param);

			link = next;
		}
	}
}


