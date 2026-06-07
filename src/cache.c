#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "cache.h"

// Initialize the cache
void initCache(StaticCache* cache) {
    memset(cache, 0, sizeof(StaticCache));
}

// Check if the cache is empty
bool isEmpty(StaticCache* cache) {
    return cache->size == 0;
}

// Check if the cache is full
bool isFull(StaticCache* cache) {
    return cache->size >= MAX_CAPACITY;
}

// Add a new element to the cache
void put(StaticCache* cache, uint32_t key, CachVal value) {
    // If the cache is full, remove the least recently used element
    if (isFull(cache)) {
        StaticCacheNode* temp = cache->rear;
        cache->rear = cache->rear->prev;
        if (cache->rear)
            cache->rear->next = NULL;
        else
            cache->front = NULL;
        free(temp);
        cache->size--;
    }

    // Add the new element to the front of the cache
    StaticCacheNode* newNode = (StaticCacheNode*)malloc(sizeof(StaticCacheNode));
    newNode->key = key;
    newNode->value = value;
    newNode->last_dl_time = time(NULL);
    newNode->counter = 0;
    newNode->next = cache->front;
    newNode->prev = NULL;
    if (cache->front)
        cache->front->prev = newNode;
    else
        cache->rear = newNode;
    cache->front = newNode;
    cache->size++;
}

// Retrieve the value based on the key from the cache
int get(StaticCache* cache, uint32_t key, CachVal ** cv) {
return 0;
    if (!isEmpty(cache)) {
        StaticCacheNode* current = cache->front;
        while (current != NULL) {
            if (current->key == key) {
                if ((time(NULL) - current->last_dl_time) > 1 || current->counter > MAX_CACHE_COUNT) {
                    // Remove the least recently used element from the cache
                    if (current->prev) {
                        current->prev->next = current->next;
                    } else {
                        cache->front = current->next;
                    }

                    if (current->next) {
                        current->next->prev = current->prev;
                    } else {
                        cache->rear = current->prev;
                    }

                    // Free the memory of the least recently used element
                    free(current);
                    cache->size--;

                    // Update pointers in the current element
                    current->next = NULL;
                    current->prev = NULL;

                    return 0;
                }
                *cv = &(current->value); // Assign pointer to the value
                current->last_dl_time = time(NULL);
                current->counter++;
                // If the element is not the first, move it to the front of the cache
                if (current != cache->front) {
                    if (current->prev)
                        current->prev->next = current->next;
                    if (current->next)
                        current->next->prev = current->prev;
                    else
                        cache->rear = current->prev;

                    current->next = cache->front;
                    current->prev = NULL;
                    cache->front->prev = current;
                    cache->front = current;
                }
                return 1;
            }
            current = current->next;
        }
    }
    return 0; // Key not found
}

void resetCache(StaticCache* cache) {
    StaticCacheNode* current = cache->front;
    while (current != NULL) {
        StaticCacheNode* temp = current;
        current = current->next;
        free(temp);
    }
    cache->front = NULL;
    cache->rear = NULL;
    cache->size = 0;
}
