/*=========================================================================*/
/* Fellow                                                                  */
/* List                                                                    */
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

#include "fellow/api/defs.h"
#include "LISTTREE.H"

/*==============*/
/* Generic list */
/*==============*/

felist::felist(void *nodeptr) : next(nullptr), prev(nullptr), node(nodeptr)
{
}

felist::felist(felist *prevptr, void *nodeptr) : next(nullptr), prev(prevptr), node(nodeptr)
{
}

/*==============================================================================*/
/* node = pointer to variable which must already be allocted in memory          */
/*==============================================================================*/
felist *listNew(void *node)
{
  return new felist(node);
}

felist *listNext(felist *l)
{
  return l != nullptr ? l->next : nullptr;
}

felist *listPrev(felist *l)
{
  return l != nullptr ? l->prev : nullptr;
}

felist *listLast(felist *l)
{
  felist *o;
  felist *t = l;
  while ((o = listNext(t)) != nullptr)
  {
    t = o;
  }
  return t;
}

void *listNode(felist *l)
{
  return l != nullptr ? l->node : nullptr;
}

felist *listAddFirst(felist *root, felist *l)
{
  if (l != nullptr)
  {
    l->next = root;
    l->prev = nullptr;
    if (root != nullptr)
    {
      if (root->prev != nullptr)
      {
        l->prev = root->prev;
        root->prev->next = l;
      }
      root->prev = l;
    }
    return l;
  }
  return root;
}

felist *listAddLast(felist *root, felist *l)
{
  if (root != nullptr)
  {
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
felist *listAddSorted(felist *root, felist *l, int (*comp)(void *, void *))
{
  felist *tmp = root;
  if (l == nullptr)
  {
    return tmp;
  }

  while (tmp != nullptr)
  {
    if (comp(listNode(tmp), listNode(l)) >= 0)
    {
      listAddFirst(tmp, l);
      if (tmp == root)
      {
        return l;
      }
      return root;
    }
    tmp = listNext(tmp);
  }

  return listAddLast(root, l);
}

felist *listCat(felist *l1, felist *l2)
{
  if (l1 == nullptr)
  {
    return l2;
  }

  if (l2 == nullptr)
  {
    return l1;
  }

  felist *t = listLast(l1);
  t->next = l2;
  l2->prev = t;
  return l1;
}

felist *listCopy(felist *srcfirstnode, size_t nodesize)
{
  if (srcfirstnode == nullptr)
  {
    return nullptr;
  }

  felist *prevnode = nullptr;
  felist *currnode = srcfirstnode;
  felist *result = nullptr;

  while (currnode != nullptr)
  {
    felist *newnode = new felist(prevnode, (void *)malloc(nodesize));
    if (prevnode == nullptr)
    {
      result = newnode;
    }
    else
    {
      prevnode->next = newnode;
    }
    memcpy(newnode->node, currnode->node, nodesize);

    prevnode = newnode;
    currnode = listNext(currnode);
  }
  return result;
}

unsigned int listCount(felist *l)
{
  felist *t = l;
  unsigned int i = 0;
  while (t != nullptr)
  {
    t = listNext(t);
    i++;
  }
  return i;
}

/*==============================================================================*/
/* listRemove removes the node from the list and connects the neighbours        */
/*==============================================================================*/
void listRemove(felist *l)
{
  if (l != nullptr)
  {
    if (l->prev != nullptr)
    {
      l->prev->next = l->next;
    }
    if (l->next != nullptr)
    {
      l->next->prev = l->prev;
    }
  }
}

/*==============================================================================*/
/* listFree removes the node from the list and memory                           */
/*==============================================================================*/
felist *listFree(felist *node)
{
  felist *t = nullptr;

  if (node != nullptr)
  {
    t = node->next;
  }
  listRemove(node);
  delete node;
  return t;
}

felist *listIndex(felist *l, unsigned int index)
{
  while (l != nullptr && index != 0)
  {
    l = listNext(l);
    index--;
  }
  if (index != 0)
  {
    return nullptr;
  }
  return l;
}

void listFreeAll(felist *l, bool freenodes)
{
  felist *tmp = l;
  while (tmp != nullptr)
  {
    if (freenodes && listNode(tmp) != nullptr)
    {
      free(listNode(tmp));
    }
    felist *tmp2 = listNext(tmp);
    listFree(tmp);
    tmp = tmp2;
  }
}
