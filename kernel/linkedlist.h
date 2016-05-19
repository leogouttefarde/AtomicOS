/**
 *  @file       linkedlist.h
 *  @brief      Doubly linked list generic class.
 */

#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <stdbool.h>
#include "process.h"

/**
 * @brief       The stored element's pointer.
 */
typedef Process* elem_t;

/**
 * @brief       An element and previous / next pointers to create a doubly linked list.
 */
typedef struct linked_elem_t {
	elem_t elem;
	struct linked_elem_t *next;
	struct linked_elem_t *prev;
} linked_elem_t;

/**
 * @brief       A generic linked list type.
 */
typedef struct linked_list_t {
	linked_elem_t *head;
	linked_elem_t *tail;
} linkedlist_t;


/**
 * \brief       linkedlist_t callback function.
 *              Useful to automatically call a function on each list element.
 *
 * @param       link            Linked list element
 * @param       arg             Custom argument
 *
 * @return                      A boolean
 */
typedef bool	(*function_t) (linked_elem_t *link, void *arg);

/**
 * \brief       Initializes a list.
 *
 * @param       list            List to initialize
 */
void ll_init(linkedlist_t *list);

/**
 * \brief       Adds an element to a list.
 *
 * @param       list            The list
 * @param       elem            The element to add
 */
void ll_add(linkedlist_t *list, elem_t elem);

/**
 * \brief       Adds an element to a list, but only if it is not already in it.
 *
 * @param       list            The list
 * @param       elem            The element to add
 */
void ll_add_unique(linkedlist_t *list, elem_t elem);

/**
 * \brief       Removes an element from a list.
 *
 * @param       list            The list
 * @param       elem            The element to remove
 * @param       free_elem       Free the contained element ?
 */
void ll_pop_elem(linkedlist_t *list, elem_t elem, bool free_elem);

/**
 * \brief       Removes a linked element from a list.
 *
 * @param       list            The list
 * @param       link            The linked element to remove
 * @param       free_elem       Free the contained element ?
 */
void ll_pop_link(linkedlist_t *list, linked_elem_t *link, bool free_elem);

/**
 * \brief       Empties a list.
 *
 * @param       list            The list.
 * @param       free_elem       Free the contained element ?
 */
void ll_empty(linkedlist_t *list, bool free_elem);

/**
 * \brief       Indicates if a list contains a given element.
 *
 * @param       list            The list.
 * @param       elem            The element.
 */
bool ll_has(linkedlist_t *list, elem_t elem);

/**
 * \brief       Applies a function to each element of a list, stops if the function returns true.
 *              Continues as long as it returns false.
 *
 * @param       list            The list.
 * @param       function        The element.
 * @param       user_param      User parameters
 */
void ll_applyfunc(linkedlist_t *list, function_t function, void *user_param);


#endif
