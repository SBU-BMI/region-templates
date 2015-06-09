
#ifndef __QUEUE_H_
#define __QUEUE_H_

#include <stdlib.h>

struct queue_node {
	struct queue_node *next;
	void *obj;
};

struct queue {
	int num_elem;
	struct queue_node *head;
	struct queue_node *tail;
};

static void queue_init(struct queue *q)
{
	q->num_elem = 0;
	q->head = NULL;
	q->tail = NULL;
}

static void queue_enqueue(struct queue *q, void *obj)
{
	struct queue_node *qn;

	qn = malloc(sizeof(struct queue_node));
	qn->obj = obj;
	qn->next = NULL;

	if(q->num_elem == 0)
		q->head = q->tail = qn;
	else {
		q->tail->next = qn;
		q->tail = qn;
	}
	q->num_elem++;
}

static void *queue_dequeue(struct queue *q)
{
	struct queue_node *qn = q->head;
	void *obj;

	if(q->num_elem == 0)
		return NULL;

	obj = qn->obj;
	q->head = qn->next;
	free(qn);

	q->num_elem--;
	if(q->num_elem == 0)
		q->tail = NULL;

	return obj;
}

static inline int queue_is_empty(struct queue *q)
{
	return (q->num_elem == 0);
}

static inline int queue_size(struct queue *q)
{
	return q->num_elem;
}

#endif /* __QUEUE_H_ */
