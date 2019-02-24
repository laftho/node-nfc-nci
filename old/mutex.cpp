/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2019  <copyright holder> <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pthread.h>
#include "mutex.h"

Mutex::Mutex()
{
  pthread_mutex_init(lock, NULL);
  pthread_cond_init(cond, NULL);
}

Mutex::~Mutex()
{
  pthread_mutex_destroy(lock);
  pthread_cond_destroy(cond);
}

int Mutex::Lock()
{
  return pthread_mutex_lock(lock);
}

int Mutex::Unlock()
{
  return pthread_mutex_unlock(lock);
}

void Mutex::Wait(bool needLock)
{
  if (needLock) {
    Lock();
  }
  
  pthread_cond_wait(cond, lock);
  
  if (needLock) {
    Unlock();
  }
}

void Mutex::Notify(bool needLock)
{
  if (needLock) {
    Lock();
  }
  
  pthread_cond_broadcast(cond);
  
  if (needLock) {
    Unlock();
  }
}
