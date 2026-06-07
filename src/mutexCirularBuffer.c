#include "mutexCirularBuffer.h"
#include <stdlib.h>

// Initialize the circular buffer
void init_buffer(CircularBufferGauge *buffer) {
    buffer->head = NULL;
    buffer->tail = NULL;
    // Initialize the spin lock (replace with your actual spin lock initialization)
    //init_spinlock(&buffer->lock);
}

// Enqueue a value into the circular buffer
bool enqueue(CircularBufferGauge *buffer, gauge_tuple_t *value) {
    Node *new_node = malloc(sizeof(Node));
    if (!new_node) {
        return false; // Memory allocation failed
    }
    new_node->data = value;
    new_node->next = NULL;

    spinlock_lock(&buffer->lock);

    if (buffer->tail == NULL) {
        // If the buffer is empty, update both head and tail
        buffer->head = new_node;
        buffer->tail = new_node;
    } else {
        // Otherwise, update the tail node
        buffer->tail->next = new_node;
        buffer->tail = new_node;
    }

    spinlock_unlock(&buffer->lock);

    return true;
}

// Dequeue a value from the circular buffer
gauge_tuple_t* dequeue(CircularBufferGauge *buffer) {
    gauge_tuple_t *value = NULL;

    spinlock_lock(&buffer->lock);

    if (buffer->head != NULL) {
        // If the buffer is not empty, dequeue the head node
        Node *node_to_remove = buffer->head;
        buffer->head = buffer->head->next;
        if (buffer->head == NULL) {
            buffer->tail = NULL; // If the buffer becomes empty, update the tail
        }
        value = node_to_remove->data;
        free(node_to_remove);
    }

    spinlock_unlock(&buffer->lock);

    return value;
}
