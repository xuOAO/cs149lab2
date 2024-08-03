#include "tasksys.h"


IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemSerial::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    thread_pool.resize(num_threads);
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::thread_func() {
    int old_val;
    task_info& t = *task_info_ptr;
    while(true) {
        {
            std::lock_guard<std::mutex> lk(t.index.mmutex);
            old_val = t.index.val; 
            if(old_val == t.num_total_tasks) break;
            else t.index.val++;
        }
        t.runnable->runTask(old_val, t.num_total_tasks);
    }
}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    task_info_ptr = new task_info{runnable, num_total_tasks};

    for(auto& pthread : thread_pool) {
        pthread = std::thread(&TaskSystemParallelSpawn::thread_func, this);
    }
    
    for(auto& pthread : thread_pool) {
        pthread.join();
    }

    delete task_info_ptr;
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads)
: ITaskSystem(num_threads), thread_pool(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    task_info_ptr = new task_info_ThreadPoolSpinning;
    for(auto& pthread : thread_pool) {
        pthread = std::thread(&TaskSystemParallelThreadPoolSpinning::thread_func, this);
    }
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    thread_state.mmutex.lock();
    thread_state.killed = true;
    thread_state.mmutex.unlock();
    for(auto& pthread : thread_pool) {
        pthread.join();
    }
    delete task_info_ptr;
}

void TaskSystemParallelThreadPoolSpinning::thread_func() {
    int old_value;
    task_info_ThreadPoolSpinning& t = *task_info_ptr;
    while(true) {
        thread_state.mmutex.lock();
        if(thread_state.killed == true) {
            thread_state.mmutex.unlock();
            break;
        }
        if(thread_state.have_task == false) {
            thread_state.mmutex.unlock();
            continue;
        }
        thread_state.mmutex.unlock();

        t.mmutex.lock();
        old_value = t.index;
        if(old_value == t.num_total_tasks) {
            t.mmutex.unlock();
            continue;
        } else {
            t.index++;
            t.mmutex.unlock();
            t.runnable->runTask(old_value, t.num_total_tasks);
            t.mmutex.lock();
            t.finished++;
            t.mmutex.unlock();
        }
    }
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Part A.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //
    task_info_ThreadPoolSpinning& t = *task_info_ptr;
    t.mmutex.lock();
    thread_state.mmutex.lock();
    thread_state.have_task = true;
    t.runnable = runnable;
    t.num_total_tasks = num_total_tasks;
    t.index = 0;
    t.finished = 0;
    thread_state.mmutex.unlock();
    t.mmutex.unlock();

    while(true) {
        t.mmutex.lock();
        if(t.finished == t.num_total_tasks) {
            t.mmutex.unlock();
            thread_state.mmutex.lock();
            thread_state.have_task = false;
            thread_state.mmutex.unlock();
            break; 
        }
        t.mmutex.unlock();
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads)
: ITaskSystem(num_threads), thread_pool(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    task_info_ptr = new task_info_ThreadPoolSleeping;
    for(auto& pthread : thread_pool) {
        pthread = std::thread(&TaskSystemParallelThreadPoolSleeping::thread_func, this);
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    thread_state.mmutex.lock();
    thread_state.killed = true;
    thread_state.mmutex.unlock();
    thread_state.can_running.notify_all();

    for(auto& pthread : thread_pool) {
        pthread.join();
    }

    delete task_info_ptr;
}

void TaskSystemParallelThreadPoolSleeping::thread_func() {
    int old_value;
    task_info_ThreadPoolSleeping& t = *task_info_ptr;
    while(true) {
        {
            std::unique_lock<std::mutex> lk(thread_state.mmutex);
            if(thread_state.killed == true) {
                break;
            }
            if(thread_state.have_task == false) {
                thread_state.can_running.wait(lk);
            }
        }

        t.mmutex.lock();
        old_value = t.index;
        if(old_value == t.num_total_tasks) {
            t.mmutex.unlock();
            continue;
        } else {
            t.index++;
            t.mmutex.unlock();
            t.runnable->runTask(old_value, t.num_total_tasks);
            t.mmutex.lock();
            t.finished++;
            t.mmutex.unlock();
        }
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    task_info_ThreadPoolSleeping& t = *task_info_ptr;
    t.mmutex.lock();
    thread_state.mmutex.lock();
    thread_state.have_task = true;
    t.runnable = runnable;
    t.num_total_tasks = num_total_tasks;
    t.index = 0;
    t.finished = 0;
    thread_state.mmutex.unlock();
    t.mmutex.unlock();
    thread_state.can_running.notify_all();
    
    while(true) {
        t.mmutex.lock();
        if(t.finished == t.num_total_tasks) {
            t.mmutex.unlock();
            thread_state.mmutex.lock();
            thread_state.have_task = false;
            thread_state.mmutex.unlock();
            break;
        }
        t.mmutex.unlock();
    }
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
