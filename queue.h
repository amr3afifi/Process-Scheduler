// C program for array implementation of queue 
#include <stdio.h> 
#include <stdlib.h> 
#include <limits.h> 

// A structure to represent a queue 
struct processData
{
	int starttime;
	long arrivaltime;
	long priority;
    long runningtime;
	int remainingtime;
    long id;
	int syspid;
	long startddress;
	int memsize;
};

struct Queue 
{ 
	int front, rear, size; 
	unsigned capacity; 
	struct processData* array; 
}; 


struct Queue* createQueue(unsigned capacity) 
{ 
	struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue)); 
	queue->capacity = capacity; 
	queue->front = queue->size = 0; 
	queue->rear = capacity - 1; // This is important, see the enqueue 
	queue->array = (struct processData*) malloc(queue->capacity * sizeof(struct processData)); 
	return queue; 
} 

// Queue is full when size becomes equal to the capacity 
int isFull(struct Queue* queue) 
{ return (queue->size == queue->capacity); } 

// Queue is empty when size is 0 
int isEmpty(struct Queue* queue) 
{ return (queue->size == 0); } 

// Function to add an item to the queue. 
// It changes rear and size 
void enqueue(struct Queue* queue,struct processData item) 
{ 
	if (isFull(queue)) 
		return; 
	queue->rear = (queue->rear + 1)%queue->capacity; 
	queue->array[queue->rear] = item; 
	queue->size = queue->size + 1; 
//	printf("%d enqueued to queue\n", item); 
} 

// Function to remove an item from queue. 
// It changes front and size 
struct processData dequeue(struct Queue* queue) 
{ 
	struct processData item;
	item.arrivaltime=-1;item.id=-1;item.priority=-1;item.runningtime=-1;

	if (isEmpty(queue)) 
		return item; 
	item = queue->array[queue->front]; 
	queue->front = (queue->front + 1)%queue->capacity; 
	queue->size = queue->size - 1; 
	return item; 
} 

// Function to get front of queue 
struct processData front(struct Queue* queue) 
{ 
	struct processData item;
	item.arrivaltime=-1;item.id=-1;item.priority=-1;item.runningtime=-1;

	if (isEmpty(queue)) 
		return item; 
	return queue->array[queue->front]; 
} 

// Function to get rear of queue 
struct processData rear(struct Queue* queue) 
{ 
	struct processData item;
	item.arrivaltime=-1;item.id=-1;item.priority=-1;item.runningtime=-1;

	if (isEmpty(queue)) 
		return item; 
	return queue->array[queue->rear]; 
} 
