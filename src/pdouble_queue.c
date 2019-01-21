/*
    Author: Gapeev Maksim
*/

#include "pdouble_queue.h"

#include <stdlib.h>
#include <stdio.h>

pdouble_queue* create_pdouble_queue()
{
    pdouble_queue* pd_queue = (pdouble_queue*)malloc(sizeof(pdouble_queue));
    
    pd_queue->head = NULL;
    pd_queue->tail = NULL;
    pd_queue->size = 0;

    pthread_mutex_init(&(pd_queue->mutex), NULL);

    return pd_queue;
}

void push_to_pdqueue(pdouble_queue *pd_queue,
                     double** data,
                     int size,
                     int first_lch,
                     int last_buffer_index)
{
    pthread_mutex_lock(&(pd_queue->mutex));
    pdq_node* pdn = (pdq_node*)malloc(sizeof(pdq_node));

    pdn->next = NULL;
    pdn->data = *data;
    pdn->size = size;
    pdn->first_lch = first_lch;
    pdn->last_buffer_index = last_buffer_index;

    if(pd_queue->head == NULL)
    {
        pd_queue->head = pdn;
        pd_queue->tail = pdn; 
    } else
    {
        pd_queue->tail->next = pdn;
        pd_queue->tail = pdn;
    }

    pd_queue->size++;

    // printf("pd_queue->size = %d\n", pd_queue->size);

    pthread_mutex_unlock(&(pd_queue->mutex));
}

void pop_from_pdqueue(pdouble_queue *pd_queue,
                      double **data,
                      int *size,
                      int *first_lch,
                      int *last_buffer_index)
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

        *first_lch = pop_node->first_lch;
        *last_buffer_index = pop_node->last_buffer_index;
        
        pd_queue->size--;

        free(pop_node);
    }

    pthread_mutex_unlock(&(pd_queue->mutex));
}

void destroy_pdouble_queue(pdouble_queue **pd_queue)
{

    while((*pd_queue)->head != NULL)
    {
        pdq_node* pop_node = (*pd_queue)->head;

        (*pd_queue)->head = (*pd_queue)->head->next; 

        free( pop_node->data );

        (*pd_queue)->size--;

        free(pop_node);
    }

    pthread_mutex_destroy(&((*pd_queue)->mutex));
    free( (*pd_queue) );
}