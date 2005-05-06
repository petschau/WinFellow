/*============================================================================*/
/* Fellow Amiga Emulator                                                      */
/* List and Tree                                                              */
/*                                                                            */
/* Author: Petter Schau (peschau@online.no)                                   */
/*                                                                            */
/* This file is under the GNU Public License (GPL)                            */
/*============================================================================*/


#include "portable.h"
#include "defs.h"
#include "listtree.h"


/*==============*/
/* Generic list */
/*==============*/

/*==============================================================================*/
/* node = pointer to variable which must already be allocted in memory          */
/*==============================================================================*/
felist *listNew(void *node) {
  felist *l;
  
  l = (felist *) malloc(sizeof(felist));
  l->next = l->prev = NULL;
  l->node = node;
  return l;
}

felist *listNext(felist *l) {
  return (l != NULL) ? l->next : NULL;
}

felist *listPrev(felist *l) {
  return (l != NULL) ? l->prev : NULL;
}

felist *listLast(felist *l) {
  felist *t, *o;

  t = l;
  while ((o = listNext(t)) != NULL) t = o;
  return t;
}  

void *listNode(felist *l) {
  return (l != NULL) ? l->node : NULL;
}

felist *listAddFirst(felist *root, felist *l) {
  if (l != NULL) {
    l->next = root;
    l->prev = NULL;
    if (root != NULL) {
      if (root->prev != NULL) {
	l->prev = root->prev;
	root->prev->next = l;
      }
      root->prev = l;
    }
    return l;
  }
  return root;
}

felist *listAddLast(felist *root, felist *l) {
  if (root != NULL) {
    felist *tmp = listLast(root);
    tmp->next = l;
    l->prev = tmp;
    return root;
  }
  return l;
}

/*==============================================================================*/
/* root = pointer to list object in which we want to add an object              */
/* l = pointer to list object which we want to add                              */
/* comp = pointer to a function which perform the compare for the sort          */
/*==============================================================================*/
felist *listAddSorted(felist *root, felist *l, int (*comp)(void *, void *)) {
  felist *tmp;

  tmp = root;
  if (l == NULL) return tmp;
  while (tmp != NULL) {
    if (comp(listNode(tmp), listNode(l)) >= 0) {
      listAddFirst(tmp, l);
      if (tmp == root) return l;
      else return root;
    }
    tmp = listNext(tmp);
  }
  return listAddLast(root, l);
}

felist *listCat(felist *l1, felist *l2) {
  felist *t;
  
  if (l1 == NULL) return l2;
  else if (l2 == NULL) return l1;
  else {
    t = listLast(l1);
    t->next = l2;
    l2->prev = t;
  }
  return l1;
}

ULO listCount(felist *l) {
  felist *t;
  ULO i;

  t = l;
  i = 0;
  while (t != NULL) {
    t = listNext(t);
    i++;
  }
  return i;
}

/*==============================================================================*/
/* listRemove removes the node from the list and connects the neighbours        */
/*==============================================================================*/
void listRemove(felist *l) {
  if (l != NULL) {
    if (l->prev != NULL) l->prev->next = l->next;
    if (l->next != NULL) l->next->prev = l->prev;
  }
}

/*==============================================================================*/
/* listFree removes the node from the list and memory                           */
/*==============================================================================*/
felist *listFree(felist *node) {
	felist *t;

	if (node != NULL)
		t = node->next;
  listRemove(node);
  if (node != NULL) free(node);
	return t;
}

felist *listIndex(felist *l, ULO index) {
  while ((l != NULL) && (index != 0)) {
    l = listNext(l);
    index--;
  }
  if (index != 0)
    return NULL;
  return l;
}

void listFreeAll(felist *l, BOOLE freenodes) {
  felist *tmp, *tmp2;

  tmp = l;
  while (tmp != NULL) {
    if (freenodes && (listNode(tmp) != NULL))
      free(listNode(tmp));
    tmp2 = listNext(tmp);    
    listFree(tmp);
    tmp = tmp2;
  }
}


/*==============*/
/* Generic tree */
/*==============*/

tree *treeNew(tree *parent, void *node) {
  tree *t;
  
  t = (tree *) malloc(sizeof(tree));
  t->parent = parent;
  t->node = node;
  t->child = NULL;
  t->lnode = listNew(t);
  return t;
}

void treeChildAdd(tree *t, tree *c) {
  if (t != NULL && c != NULL)
    t->child = (tree *) listNode(listAddLast((t->child != NULL) ?
					     t->child->lnode : NULL,
					     c->lnode));
}

tree *treeChild(tree *t) {
  return (t != NULL) ? t->child : NULL;
}

tree *treeSisterNext(tree *t) {
  return (tree *) listNode(listNext((t != NULL) ? t->lnode : NULL));
}

tree *treeSisterPrev(tree *t) {
  return (tree *) listNode(listPrev((t != NULL) ? t->lnode : NULL));
}

void *treeNode(tree *t) {
  return (t != NULL) ? t->node : NULL;
}

tree *treeParent(tree *t) {
  return (t != NULL) ? t->parent : NULL;
}

ULO treeChildCount(tree *t) {
  return listCount(t->child->lnode);
}


