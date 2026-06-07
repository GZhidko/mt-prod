#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include "src/lhash.h"  // Include the lhash.h header file for the LHASH functions
#include "src/lhstat.h"
#include "src/debug.h"

#define NUM_THREADS 30
#define NUM_OPERATIONS 5000

// Define a global LHASH for testing
LHASH *hash_table;



int custom_compare(const void *a, const void *b) {
    const char *key1 = (const char *)a;
    const char *key2 = (const char *)b;

    return strcmp(key1, key2);
}

unsigned long simple_hash(const char *key) {
    unsigned long hash = 5381;
    int c;

    while ((c = *key++) != '\0') {
        hash = ((hash << 5) + hash) + c;
    }
    //printf("hash%d", hash);
    return hash;
}


// Function to perform insertions into the hash table
void* insert_operations(void* arg) {
    char key[16];
    spinlock * sp;
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        sprintf(key, "key%d", i);
        char* data = strdup(key);
        lh_insert(hash_table, data, &sp);
        spinlock_unlock(sp);
        usleep(1000); // Sleep for a short time to allow other threads to run
    }
    return NULL;
}

// Function to perform deletions from the hash table
void* delete_operations(void* arg) {
    char key[16];
    spinlock * sp;
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        sprintf(key, "key%d", i);
        lh_delete(hash_table, key, &sp);
        spinlock_unlock(sp);
        usleep(1000); // Sleep for a short time to allow other threads to run
    }
    return NULL;
}

// Function to perform retrievals from the hash table
void* retrieve_operations(void* arg) {
    char key[16];
    spinlock * sp = NULL;
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        sprintf(key, "key%d", i);
        char* data = lh_retrieve(hash_table, key, &sp);
        if (data != NULL) {
            printf("Retrieved: %s\n", data);
        } else {
            printf("Not found: %s\n", key);
        }
        spinlock_unlock(sp);
        usleep(1000); // Sleep for a short time to allow other threads to run
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    sw_log_init("test.log");
    //FILE fd = open(global_logfile, O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
    // Initialize the hash table
    sw_debug_create("all");
    hash_table = lh_new(simple_hash, custom_compare,100);
    sw_lhstat_create();
    sw_lhstat_new("stat_hassh",hash_table);
    time_t now = time(NULL);
    sw_lhstat_do(now);
    // Create threads for insertions, deletions, and retrievals
    for (int i = 0; i < NUM_THREADS; i++) {
        if (i % 3 == 0) {
            pthread_create(&threads[i], NULL, insert_operations, NULL);
        } else if (i % 3 == 1) {
            pthread_create(&threads[i], NULL, delete_operations, NULL);
        } else {
            pthread_create(&threads[i], NULL, retrieve_operations, NULL);
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    now = time(NULL);
    sw_lhstat_do(now);
    // Clean up and free the hash table
    lh_free(hash_table);

    return 0;
}
