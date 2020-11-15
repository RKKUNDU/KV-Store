#include "ds_defs.h"

int getHashingIndex(char *key) {
    unsigned char *str = (unsigned char *)key;
    unsigned long hash = 5381;
    int c;
    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash % BUCKETS;
}

  struct node * createNode(char *key) {
        struct node *newnode;
        newnode = (struct node *)malloc(sizeof(struct node));
        newnode->key = strdup(key);
        newnode->next = NULL;
        return newnode;
  }

  void insertToHash(struct hash* hashTable, char *key) {
        int hashIndex = getHashingIndex(key);
        struct node *newnode =  createNode(key);
        /* head of list for the bucket with index "hashIndex" */
        if (!hashTable[hashIndex].head) {
                hashTable[hashIndex].head = newnode;
                hashTable[hashIndex].count = 1;
                return;
        }
        /* adding new node to the list */
        newnode->next = (hashTable[hashIndex].head);
        /*
         * update the head of the list and no of
         * nodes in the current bucket
         */
        hashTable[hashIndex].head = newnode;
        hashTable[hashIndex].count++;
        return;
  }

  void deleteFromHash(struct hash* hashTable,char *key) {
        /* find the bucket using hash index */
        int hashIndex = getHashingIndex(key);
        
        struct node *temp, *myNode;
        /* get the list head from current bucket */
        myNode = hashTable[hashIndex].head;
        if (!myNode) {
                return;
        }
        temp = myNode;
        while (myNode != NULL) {
                /* delete the node with given key */
                if (strcmp(myNode->key ,key)==0) {
                        if (myNode == hashTable[hashIndex].head)
                                hashTable[hashIndex].head = myNode->next;
                        else
                                temp->next = myNode->next;
                        hashTable[hashIndex].count--;
                        free(myNode);
                        break;
                }
                temp = myNode;
                myNode = myNode->next;
        }
  }

char *searchInHash(struct hash* hashTable,char *key) {
        int hashIndex = getHashingIndex(key);
        struct node *myNode;
        myNode = hashTable[hashIndex].head;
        if (!myNode) {
                return NULL;
        }
        while (myNode != NULL) {
                if (strcmp(myNode->key, key)==0) {
                        return myNode->key;
                }
                myNode = myNode->next;
        }

        return NULL;
  }

struct hash* createHashTable() { 
	struct hash* hashTable = (struct hash*)malloc(BUCKETS*sizeof(struct hash)); 
        struct hash* temp = hashTable;
        for (size_t i = 0; i < BUCKETS; i++)
        {
                temp[i].count =0;
                temp[i].head = NULL;
        }
	return hashTable; 
} 