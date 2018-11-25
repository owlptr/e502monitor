#include "pdouble_queue.h"

#include <stdlib.h>
#include <stdio.h>

pdouble_queue* create_pdouble_queue()
{
    pdouble_queue* pd_queue = (pdouble_queue*)malloc(sizeof(pdouble_queue));
    
    pd_queue->head = NULL;
    pd_queue->tail = NULL;

    pthread_mutex_init(&(pd_queue->mutex), NULL);

    return pd_queue;
}

void push_to_pdqueue(pdouble_queue *pd_queue, double** data, int size)
{
    pthread_mutex_lock(&(pd_queue->mutex));
    pdq_node* pdn = (pdq_node*)malloc(sizeof(pdq_node));

    pdn->next = NULL;
    pdn->data = *data;
    pdn->size = size;

    if(pd_queue->head == NULL)
    {
        pd_queue->head = pdn;
        pd_queue->tail = pdn; 
    } else
    {
        pd_queue->tail->next = pdn;
        pd_queue->tail = pdn;
    }

    pthread_mutex_unlock(&(pd_queue->mutex));
}

void pop_from_pdqueue(pdouble_queue *pd_queue, double **data, int *size)
{
    pthread_mutex_lock(&(pd_queue->mutex));
    
    if(pd_queue->head == NULL){
        *data = NULL;
        *size = -1;
    } else { 
        pdq_node* pop_node = pd_queue->head;
        pd_queue->head = pd_queue->head->next; 

        *data = pop_node->data;
        (*size) = pop_node->size;

        free(pop_node);
    }

    pthread_mutex_unlock(&(pd_queue->mutex));
}

void destroy_pdouble_queue(pdouble_queue **pd_queue)
{
    pthread_mutex_destroy(&((*pd_queue)->mutex));
    free(*pd_queue);
}