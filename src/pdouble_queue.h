/*
    "pdouble_queue.h" contains declaration of
    functions to use and create safe-thread queue
    contained double data array.

    Author: Gapeev Maksim
*/

#ifndef PDOUBLE_QUEUE_H
#define PDOUBLE_QUEUE_H

#include <pthread.h>

#define LAST_BUFFER 1
#define NOT_LAST_BUFFER 2

typedef struct {
    double* data;
    int size;

    int first_lch; // number of logical channel
                        // to which correspond first
                        // countodwn in the array <<data>>
                        // in the next pdq_node
    
    int last_buffer_index; 
    
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
                     int first_lch,
                     int last_buffer_index);

void pop_from_pdqueue(pdouble_queue *pd_queue,
                      double** data,
                      int *size,
                      int *first_lch,
                      int *last_buffer_index);

void destroy_pdouble_queue(pdouble_queue **pd_queue);

#endif // PDOUBLE_QUEUE_H
