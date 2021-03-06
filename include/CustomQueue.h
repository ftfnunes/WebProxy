#include <stdlib.h>
#include <stdio.h>
#include "Common.h"

#ifndef QUEUE_INC
#define QUEUE_INC

#define HASHTABLE_SIZE 1000000

// Um nó possui referências para os elementos sucessores e antecessores na fila
// e na sua lista de nós pertencentes à um bucket na hashtable.
struct qnode {
    char *key;
    struct qnode *qNext;
    struct qnode *qPrev;
    struct qnode *tbNext;
    struct qnode *tbPrev;
};

typedef struct qnode QueueNode;

typedef QueueNode** Hashtable;

// A fila possui referência para seu início e para o seu fim. A hashtable é
// utilizada para se localizar com baixa complexidade um elemento na fila.
struct queue {
    QueueNode *first;
    QueueNode *last;
    Hashtable hashtable;
};

typedef struct queue Queue;

char *pop(Queue *queue);

int push(Queue *queue, char *key);

void removeFromHashtable(Hashtable hashtable, QueueNode* node);

QueueNode *getFromHashtable(Hashtable hashtable, char *key);

void insertInHashtable(Hashtable hashtable, QueueNode *node);

QueueNode *createNode(char *key);

long int getHashFromKey(char *key);

Queue *initializeQueue();

#endif
