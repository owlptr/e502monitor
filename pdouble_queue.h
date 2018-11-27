/*
    Author: Gapeev Maksim
*/

#ifndef PDOUBLE_QUEUE_H
#define PDOUBLE_QUEUE_H

#include <pthread.h>

typedef struct {
    double* data;
    int size;

    int first_lch; // number of logical channel
                        // to which correspond first
                        // countodwn in the array <<data>>
                        // in the next pdq_node

    struct pdq_node* next;

}pdq_node;

typedef struct{
    pdq_node* head;
    pdq_node* tail;

    int size;

    pthread_mutex_t mutex;
}pdouble_queue;

pdouble_queue* create_pdouble_queue();

void push_to_pdqueue(pdouble_queue *pd_queue,
                     double** data,
                     int size,
                     int first_lch);

void pop_from_pdqueue(pdouble_queue *pd_queue,
                      double** data,
                      int *size,
                      int *first_lch);

void destroy_pdouble_queue(pdouble_queue **pd_queue);

#endif // PDOUBLE_QUEUE_H
