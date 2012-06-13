/********************************************************************************
 * list.c
 */
#include "list.h"
#include <unistd.h>
#include <stdlib.h>

typedef struct list_node_s list_node_t;

struct list_node_s
{
	void * pdata;
	list_node_t * pnext;
	list_node_t * pprev;
};

typedef struct
{
	list_node_t * phead;
	list_node_t * ptail;
	int count;
} listi_t;

void listFreeNode(
	list_node_t * pn);
list_node_t * listAllocNode();
void listFreeNode(
	list_node_t * pn);
list_node_t * listUnlinkNode(
	listi_t * plst,
	list_node_t *pn);

/****************************************
 * listNew
 */
list_t listNew()
{
	listi_t * plst = malloc(sizeof(listi_t));

	plst->phead = NULL;
	plst->ptail = NULL;
	plst->count = 0;

	return (list_t) plst;
}

/****************************************
 * listAllocNode
 */
list_node_t * listAllocNode()
{
	list_node_t * pn = malloc(sizeof(list_node_t));

	pn->pdata = NULL;
	pn->pnext = NULL;
	pn->pprev = NULL;

	return pn;
}

/****************************************
 * listFreeNode
 *
 * Deletes all items from the list, calls delegate if specified for each data item
 */
void listFreeNode(
	list_node_t * pn)
{
	free(pn);
}

/****************************************
 * listDel
 *
 */
void listDel(
	list_t lst,
	list_delegate_t dlg)
{
	listi_t * plst = (listi_t *) lst;

	if (plst == NULL)
		return;

	list_node_t * pnext;
	list_node_t * p;
	for (p = plst->phead; p != NULL; p = pnext)
	{
		// Call delegate if needed
		if (dlg != NULL)
			dlg(p->pdata);

		pnext = p->pnext;

		listFreeNode(p);
	}

	free(plst);
}

/****************************************
 * listAppendBack
 */
void listAppendBack(
	list_t lst,
        void * item)
{
	listi_t * plst = (listi_t *) lst;

	list_node_t * pn = listAllocNode();

	pn->pdata = item;
	pn->pnext = NULL;
	pn->pprev = plst->ptail;

	if (plst->ptail != NULL)
		plst->ptail->pnext = pn;

	plst->ptail = pn;
	if (plst->phead == NULL)
		plst->phead = pn;

	plst->count++;
}

/****************************************
 * listUnlinkNode
 */
list_node_t * listUnlinkNode(
	listi_t * plst,
	list_node_t *pn)
{
	if (pn->pprev != NULL)
		pn->pprev->pnext = pn->pnext;
	if (pn->pnext != NULL)
		pn->pnext->pprev = pn->pprev;

	if (plst->phead == pn)
		plst->phead = pn->pnext;
	if (plst->ptail == pn)
		plst->ptail = pn->pprev;

	if (plst->phead == NULL)
		plst->phead = plst->ptail;
	if (plst->ptail == NULL)
		plst->ptail = plst->phead;

	pn->pnext = NULL;
	pn->pprev = NULL;

	plst->count--;

	return pn;
}

/****************************************
 * listRemove
 */
void *int_listRemove(
                     list_t lst,
                     list_node_t *pnode)
{
	listi_t * plst = (listi_t *) lst;
    
	if (plst == NULL || plst->phead == NULL)
		return NULL;
    
	list_node_t * pn = listUnlinkNode(plst, pnode);
    
	void * item = pn->pdata;
    
	listFreeNode(pn);
    
	return item;
}

/****************************************
 * listRemoveFront
 */
void *listRemoveFront(
	list_t lst)
{
	listi_t * plst = (listi_t *) lst;
    return int_listRemove(lst, plst->phead);
}

/****************************************
 * listCount
 */
int listCount(
	list_t lst)
{
	listi_t * plst = (listi_t *) lst;

	if (plst == NULL)
		return 0;

	return plst->count;
}

