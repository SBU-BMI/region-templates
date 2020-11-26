/*
 * TasksQueue.h
 *
 *  Created on: Aug 17, 2011
 *      Author: george
 */

#ifndef TASKSQUEUE_H_
#define TASKSQUEUE_H_

#include <semaphore.h>
#include <list>
#include <map>
#include "ExecEngineConstants.h"
#include "Task.h"

class Task;

class TasksQueue {
  protected:
    pthread_mutex_t queueLock;
    sem_t tasksToBeProcessed;
    int cpuThreads;
    int gpuThreads;

  public:
    TasksQueue();
    virtual ~TasksQueue();

    // These methods are implemented in subclasses according to the type of
    // queue chosen
    virtual bool insertTask(Task* task);
    // Remove task if available and returns a pointer
    virtual Task* getTask(int procType = ExecEngineConstants::CPU);
    virtual Task* getTask(int availableCpus, int availableGpus) {
        return NULL;
    };  // only for halide queues to overwrite
    // Remove task if found and returns a pointer
    virtual Task* getByTaskId(int id);

    // Unlock threads that may be waiting at the getTask function
    void releaseThreads(int numThreads);

    void incrementTasksToProcess();

    // Return the number of tasks queued for execution
    virtual int getSize();

    virtual void getFrontBackSpeedup(float& front, float& back){};
};

class TasksQueueFCFS : public TasksQueue {
  private:
    list<Task*> tasksQueue;

  public:
    TasksQueueFCFS(int cpuThreads, int gpuThreads) {
        this->cpuThreads = cpuThreads;
        this->gpuThreads = gpuThreads;
    }
    bool insertTask(Task* task);
    Task* getTask(int procType = ExecEngineConstants::CPU);
    int getSize();
    Task* getByTaskId(int id);
};

class TasksQueuePriority : public TasksQueue {
  private:
    multimap<float, Task*> tasksQueue;

  public:
    TasksQueuePriority(int cpuThreads, int gpuThreads) {
        this->cpuThreads = cpuThreads;
        this->gpuThreads = gpuThreads;
    }
    bool insertTask(Task* task);
    Task* getTask(int procType = ExecEngineConstants::CPU);
    int getSize();
    void getFrontBackSpeedup(float& front, float& back);
};

class TasksQueueHalide : public TasksQueue {
  private:
    // Tasks queue associated with the task's internal ID
    std::map<int, Task*> allTasksQueue;

    // Maps of tasks' IDs per target processor
    std::map<int, std::list<int>> tasksPerTarget;

    // Counter of tasks per target resource
    std::map<int, int> threadsPerTarget;

  public:
    TasksQueueHalide(int cpuThreads, int gpuThreads) {
        this->cpuThreads = cpuThreads;
        this->gpuThreads = gpuThreads;

        this->threadsPerTarget[ExecEngineConstants::CPU] = cpuThreads;
        this->threadsPerTarget[ExecEngineConstants::GPU] = gpuThreads;
    }
    bool insertTask(Task* task);
    Task* getTask(int procType = ExecEngineConstants::CPU);
    Task* getTask(int availableCpus, int availableGpus);
    int getSize();
    Task* getByTaskId(int id);
};

#endif /* TASKSQUEUE_H_ */
