#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>

class Mutex
{
private:
  pthread_mutex_t lock;
  pthread_cond_t cond;

public:
  Mutex();
  ~Mutex();

  int Lock();
  int Unlock();
  void Wait(bool needLock);
  void Notify(bool needLock);
};

#endif // MUTEX_H
