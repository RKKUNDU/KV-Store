#include "ds_defs.h"

// function to create a queue 
// of given capacity. 
// It initializes size of queue as 0 
struct Queue* createQueue() 
{ 
	struct Queue* queue = (struct Queue*)malloc(sizeof(struct Queue)); 
	queue->size = 0; 
	queue->front = queue->rear = NULL;
	return queue; 
}

// Queue is full when size becomes 
// equal to the capacity 
// int isFull(struct Queue* queue) 
// { 
// 	return (queue->size == queue->capacity); 
// } 

// Queue is empty when size is 0 
int isEmpty(struct Queue* queue) 
{ 
	return (queue->size == 0); 
} 

// Function to add an item to the queue. 
// It changes rear and size 
void add(struct Queue* queue, char *req, int clientFd) 
{  
    struct QueueNode* node = (struct QueueNode*)malloc(sizeof(struct QueueNode));
    node->clientFd = clientFd;
    node->req = strdup(req);
    if (queue->rear == NULL) { 
        queue->front = queue->rear = node; 
        queue->size++;
        return;
    }
    node->next = NULL;
    queue->rear->next = node;
    queue->rear = node;
    queue->size++;
} 

// Function to remove an item from queue. 
// It changes front and size 
void pop(struct Queue* queue) 
{ 
    if (queue->rear == NULL) {
        return;
    }
	struct QueueNode *temp= queue->front;
    queue->front = temp->next;
    free(temp);
    queue->size--;
    if (queue->size==0) {
        queue->front=NULL;
        queue->rear=NULL;
    }
    
} 

// Function to get front of queue 
struct QueueNode *top(struct Queue* queue) 
{ 
	return queue->front; 
} 

// Function to get rear of queue 
struct QueueNode *bottom(struct Queue* queue) 
{ 
	return queue->rear; 
}

int size(struct Queue *queue){
    return queue->size;
}

void print_queue(struct Queue *Q){
    struct QueueNode *temp = Q->front;
    while (temp) {
        printf("Data:%s\n", temp->req);
        temp=temp->next;
    }
}