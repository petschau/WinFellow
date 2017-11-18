/* @(#) $Id: LISTTREE.C,v 1.5 2009-07-25 10:23:59 peschau Exp $ */
/*=========================================================================*/
/* Fellow                                                                  */
/* List and Tree                                                           */
/*                                                                         */
/* Author: Petter Schau                                                    */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2, or (at your option)     */
/* any later version.                                                      */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software Foundation, */
/* Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.          */
/*=========================================================================*/

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

felist *listCopy(felist *srcfirstnode, size_t nodesize) {
  felist *prevnode = NULL;
  felist *currnode = srcfirstnode;
  felist *result = NULL;

  if(srcfirstnode == NULL) return NULL;
  
  while(currnode != NULL) {
    felist *newnode = (felist *) malloc(sizeof(felist));
    newnode->prev = prevnode;
    if(prevnode == NULL) { 
      result = newnode;
    }
    else {
      prevnode->next = newnode;
    }
    newnode->next = NULL;
    newnode->node = (void *) malloc(nodesize);
    memcpy(newnode->node, currnode->node, nodesize);
  
    prevnode = newnode;
    currnode = listNext(currnode);
  }
  return result;
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
  felist *t = NULL;

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
