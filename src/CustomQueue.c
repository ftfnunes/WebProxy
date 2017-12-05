#include "CustomQueue.h"

// Remove o elemento do inicio da fila.
char *pop(Queue *queue) {
    QueueNode *first = queue->first;
    char *key = NULL;

    if (first != NULL) {
        key = first->key;
        queue->first = first->qNext;
        if (first->qNext != NULL) {
            first->qNext->qPrev = NULL;
        } else {
            queue->last = NULL;
        }

        removeFromHashtable(queue->hashtable, first);

        free(first);
    }

    return key;
}

// Insere um elemento no final da fila. Se o elemento já existir no meio da fila,
// ele é removido e depois adicionado ao final.
int push(Queue *queue, char *key) {
    QueueNode *node = getFromHashtable(queue->hashtable, key);
    QueueNode *last = queue->last;

    if (node != NULL) {
        if (node->qNext != NULL) {
            node->qNext->qPrev = node->qPrev;
        } else {
            return TRUE;
        }
        if (node->qPrev != NULL) {
            node->qPrev->qNext = node->qNext;
        } else {
            queue->first = node->qNext;
        }
    } else {
        node = createNode(key);
        if (node == NULL) {
            return FALSE;
        }
        insertInHashtable(queue->hashtable, node);
    }

    if (last != NULL) {
        last->qNext = node;
    } else {
        queue->first = node;
    }

    node->qPrev = last;
    node->qNext = NULL;
    queue->last = node;

    return TRUE;
}

// Remove o elemento de seu bucket na hashtable
void removeFromHashtable(Hashtable hashtable, QueueNode* node) {
    if (node->tbPrev != NULL) {
        node->tbPrev->tbNext = node->tbNext;
    } else {
        hashtable[getHashFromKey(node->key)] = node->tbNext;
    }
    if (node->tbNext != NULL) {
        node->tbNext->tbPrev = node->tbPrev;
    }
}

// Recupera um elemento específico, acessando-o pela hashtable.
QueueNode *getFromHashtable(Hashtable hashtable, char *key) {
    QueueNode* bucket = hashtable[getHashFromKey(key)];
    QueueNode* cur = NULL;

    for (cur = bucket; cur != NULL; cur = cur->tbNext) {
        if (strcmp(key, cur->key) == 0) {
            return cur;
        }
    }

    return NULL;
}

// Insere um elemento na hashtable
void insertInHashtable(Hashtable hashtable, QueueNode *node) {
    long int index = getHashFromKey(node->key);
    QueueNode* bucket = hashtable[index];

    if (bucket != NULL) {
        bucket->tbPrev = node;
    }

    node->tbNext = bucket;
    node->tbPrev = NULL;
    hashtable[index] = node;
}

QueueNode *createNode(char *key) {
    QueueNode *node = (QueueNode *) malloc(sizeof(QueueNode));
    if (node == NULL) {
        return NULL;
    }

    node->key = malloc((strlen(key)+1)*sizeof(char));
    strcpy(node->key, key);

    node->qPrev = NULL;
    node->qNext = NULL;
    node->tbPrev = NULL;
    node->tbNext = NULL;

    return node;
}

long int getHashFromKey(char *key) {
    int stringIndex = strlen(key)-2*sizeof(long int);
    return (strtol(key+stringIndex, NULL, 16) % HASHTABLE_SIZE);
}

// inicializa fila
Queue *initializeQueue() {
    Queue *queue = (Queue *)malloc(sizeof(Queue));

    queue->hashtable = (Hashtable)calloc(HASHTABLE_SIZE, sizeof(QueueNode *));
    if (queue->hashtable == NULL) {
        exit(1);
    }
    queue->first = NULL;
    queue->last = NULL;

    return queue;
}
