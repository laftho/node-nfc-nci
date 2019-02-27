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

#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>

/**
 * @todo write docs
 */
class Mutex
{
private:
  pthread_mutex_t lock;
  pthread_cond_t cond;

public:
    /**
     * Default constructor
     */
    Mutex();

    /**
     * Destructor
     */
    ~Mutex();

    int Lock();
    int Unlock();
    void Wait(bool needLock);
    void Notify(bool needLock);
};

#endif // MUTEX_H
