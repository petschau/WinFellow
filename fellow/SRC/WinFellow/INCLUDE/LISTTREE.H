#ifndef LISTTREE_H
#define LISTTREE_H

/* Generic list */

class felist
{
public:
  felist *next;
  felist *prev;
  void *node;

  felist(void* nodeptr);
  felist(felist* prevptr, void* nodeptr);
};

felist *listNew(void *node);
felist *listAddFirst(felist *root, felist *l);
felist *listAddLast(felist *root, felist *l);
felist *listAddSorted(felist *root, felist *l, int (*comp)(void *, void *));
felist *listNext(felist *l);
felist *listPrev(felist *l);
felist *listLast(felist *l);
felist *listCat(felist *l1, felist *l2);
felist *listCopy(felist *, size_t);
unsigned int listCount(felist *l);
void *listNode(felist *l);
void listRemove(felist *l);
felist *listFree(felist *node);
felist *listIndex(felist *l, unsigned int index);
void listFreeAll(felist *l, bool freenodes);

#endif
