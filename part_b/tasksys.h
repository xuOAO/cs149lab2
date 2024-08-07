#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

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
class TaskSystemParallelSpawn: public ITaskSystem {
    public:
        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
struct JobInfo {
    IRunnable* runnable;
    int num_total_tasks;
    int num_finished_tasks;
    int num_deps;
    bool is_finished;
};

class DispatchInfo {
    public:
        DispatchInfo() : next_id{}, num_finished_job{} {}
        std::vector<std::vector<TaskID>> graph;
        std::vector<JobInfo> job_info_table;
        int next_id;
        int num_finished_job;
        std::mutex mmutex;
};

class TaskDe {
    public:
        TaskID job_id;
        IRunnable* runnable;
        int index;
        int num_total_tasks;
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
        void thread_execute_func();
        void thread_dispatch_func();
    private:
        std::vector<std::thread> thread_pool;
        //maintain the dispatch info which has DAG, job_info_table and some states.
        DispatchInfo dispatch_info;
        //the dispatch_thread dispatch the ready_tasks into the queue,
        //the execute_thread fetch the task and execute it.
        struct {
            std::queue<TaskDe> queue;
            std::mutex mmutex;
        }tasks_queue;
        //the execute_thread send the task_id into the queue,
        //the dispatch_thread address the task_id and update the corresponding job_info.
        struct {
            std::queue<TaskID> queue;
            std::mutex mmutex;
        }finished_queue;
        
        struct {
            bool is_finished = false;
            std::mutex mmutex;
            inline void set_fin() {
                mmutex.lock();
                is_finished = true;
                mmutex.unlock();
            }
        }pool_state;

        struct {
            bool have_task = false;
            std::mutex mmutex;
            std::condition_variable can_running;
        } exec_thread_state;
};

#endif
