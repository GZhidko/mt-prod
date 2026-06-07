#ifndef SPINLOCK_H
#define SPINLOCK_H
#include "portab.h"
#include "stdint.h"

typedef struct spinlock__{
    int locked;
} spinlock;

#define SPINLOCK_INIT { 0 };


void spinlock_lock(spinlock* spinlock);

void spinlock_unlock(spinlock*  spinlock);

#endif // SPINLOCK_H
