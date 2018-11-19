#ifndef PDOUBLE_QUEUE_H
#define PDOUBLE_QUEUE_H

#include <pthread.h>

typedef struct {
    double* data;
    int size;

    struct pdq_node* next;

}pdq_node ;

typedef struct{
    pdq_node* head;
    pdq_node* tail;

    int size;

    pthread_mutex_t mutex;
}pdouble_queue;

pdouble_queue* create_pdouble_queue();

void push(pdouble_queue *pd_queue, double** data, int size);
void pop(pdouble_queue *pd_queue, double** data, int *size);

void destroy_pdouble_queue(pdouble_queue **pd_queue);

#endif // PDOUBLE_QUEUE_H
