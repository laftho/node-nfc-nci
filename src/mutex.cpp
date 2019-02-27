#include <pthread.h>
#include "mutex.h"

Mutex::Mutex()
{
  pthread_mutex_init(&lock, NULL);
  pthread_cond_init(&cond, NULL);
}

Mutex::~Mutex()
{
  pthread_mutex_destroy(&lock);
  pthread_cond_destroy(&cond);
}

int Mutex::Lock()
{
  return pthread_mutex_lock(&lock);
}

int Mutex::Unlock()
{
  return pthread_mutex_unlock(&lock);
}

void Mutex::Wait(bool needLock)
{
  if (needLock) {
    Lock();
  }
  
  pthread_cond_wait(&cond, &lock);
  
  if (needLock) {
    Unlock();
  }
}

void Mutex::Notify(bool needLock)
{
  if (needLock) {
    Lock();
  }
  
  pthread_cond_broadcast(&cond);
  
  if (needLock) {
    Unlock();
  }
}
