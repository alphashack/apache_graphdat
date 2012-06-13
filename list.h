/********************************************************************************
 * list.h
 */

#ifndef LIST_H
#define LIST_H

typedef void * list_t;

typedef void (*list_delegate_t)(void * item);

list_t listNew();
void listDel(list_t lst, list_delegate_t dlg);
void listAppendBack(list_t lst, void *);
void *listRemoveFront(list_t lst);
int listCount(list_t lst);

#endif /* LIST_H_ */
