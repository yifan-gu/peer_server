/*
 *
@brief
  Doubly linked List data structure implementation for this web server project.

@author Hongchao Deng (Timber) <hongchad@andrew.cmu.edu>

@bugs No known bugs
 *
 */
#include <malloc.h>

#include <linkedlist.h>

ll_Node* new_ll_Node(void *item) {
    ll_Node *node = malloc(sizeof(ll_Node));
    if(node == NULL)
        return NULL;

    node->item = item;
    return node;
}

void init_linkedlist(Linlist *ll){
    ll_Node *head = &ll->head;
    head->next = head;
    head->prev = head;
    ll->count = 0;
}

void ll_delete_allnodes(Linlist *ll, void (*del_item)(void *)){
    ll_Node *next;
    ll_Node *head = & ll->head;
    ll_Node *iter = head->next;
    while(iter != head) {
        next = iter->next;
        del_item(iter->item);
        free(iter);
        iter = next;
    }
}

int ll_count(Linlist *ll) {
    return ll->count;
}

void ll_insert_last(Linlist *ll, ll_Node *node) {
    ll_Node *head, *last;

    head = &ll->head;
    last = head->prev;

    last->next = node;
    node->prev = last;
    node->next = head;
    head->prev = node;

    ll->count ++;
}

void ll_remove(Linlist *ll, ll_Node *node) {
    ll_Node *next, *prev;
    prev = node->prev;
    next = node->next;

    next->prev = prev;
    prev->next = next;

    ll->count --;
}

ll_Node *ll_start(Linlist *ll) {
    ll_Node* head = & ll->head;
    return head->next;
}
ll_Node *ll_next(ll_Node *node) {
    return node->next;
}
ll_Node *ll_end(Linlist *ll) {
    return &ll->head;
}
