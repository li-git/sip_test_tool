#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include <sched.h>

#define CPU_PAUSE __asm__ ("pause")

typedef struct spinlock {
  volatile int m_isLocked;
} spinlock;

static inline void spinlock_init(spinlock *lock) {
  lock->m_isLocked = 0;
}

static inline void spinlock_lock(spinlock *lock) {
  if (__sync_lock_test_and_set(&lock->m_isLocked, 1)) {
    int spins = 0;
    while (lock->m_isLocked != 0 || __sync_lock_test_and_set(&lock->m_isLocked, 1)) {
      if ((++spins & 63) == 0) {
        sched_yield();
      }
      CPU_PAUSE;
    }
  }
}

static inline void spinlock_unlock(spinlock *lock) {
  __sync_lock_release(&lock->m_isLocked);
}

#endif