#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial: public ITaskSystem {
    public:
        TaskSystemSerial(int num_threads);
        ~TaskSystemSerial();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class task_info {
    public:
        task_info(IRunnable* r, int n) : runnable{r}, num_total_tasks{n}, index{} {}
        IRunnable* runnable;
        int num_total_tasks;
        struct {
            int val;
            std::mutex mmutex;
        } index;
};
class TaskSystemParallelSpawn: public ITaskSystem {
    public:
        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
        void thread_func();
    private:
        std::vector<std::thread> thread_pool;
        task_info* task_info_ptr;
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class task_info_ThreadPoolSpinning {
    public:
        IRunnable *runnable;
        int num_total_tasks;
        int index = 0;
        int finished = 0;
        std::mutex mmutex;
};
class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
        void thread_func();
    private:
        std::vector<std::thread> thread_pool;
        task_info_ThreadPoolSpinning *task_info_ptr;
        struct {
            bool have_task = false;
            bool killed = false;
            std::mutex mmutex;
        }thread_state;
};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */

class task_info_ThreadPoolSleeping {
    public:
        IRunnable* runnable;
        int index;
        int finished;
        int num_total_tasks;
        std::mutex mmutex;
};
class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
        void thread_func();
    private:
        std::vector<std::thread> thread_pool;
        task_info_ThreadPoolSleeping* task_info_ptr;
        struct {
            bool have_task = false;
            bool killed = false;
            std::mutex mmutex; 
            std::condition_variable can_running; 
        } thread_state;
};

#endif
