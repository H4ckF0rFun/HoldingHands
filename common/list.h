#ifndef  _LIST_H
#define _LIST_H


struct list_node
{
	struct list_node * next;
	struct list_node * prev;
};


#define list_add_node(list,node) \
	do{							 \
		(node)->next = (list);  \
		(node)->prev = (list)->prev;  \
		(node)->next->prev = (node); \
		(node)->prev->next = (node); \
								}while(0);				\


#define list_del_node(list,node) \
	do{							 \
		(node)->next->prev = (node)->prev; \
		(node)->prev->next = (node)->next; \
		(node)->prev = (node); \
		(node)->next = (node); \
				}while(0);				\


#define list_init(list)    \
	do{					   \
		(list)->next = (list); \
		(list)->prev = (list); \
			} while (0);	       \


#endif


