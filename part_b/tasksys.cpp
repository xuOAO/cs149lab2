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
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemSerial::sync() {
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
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
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

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
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
: ITaskSystem(num_threads) {
    //
    // TODO: CS149 student implementations may decide to perform setup
    // operations (such as thread pool construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    num_threads = num_threads > 2 ? num_threads : 2;
    thread_pool.resize(num_threads);
    thread_pool[0] = std::thread(&TaskSystemParallelThreadPoolSleeping::thread_dispatch_func, this);
    for(int i = 1; i < num_threads; ++i) {
        thread_pool[i] = std::thread(&TaskSystemParallelThreadPoolSleeping::thread_execute_func, this);
    }
}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    //
    // TODO: CS149 student implementations may decide to perform cleanup
    // operations (such as thread pool shutdown construction) here.
    // Implementations are free to add new class member variables
    // (requiring changes to tasksys.h).
    //
    pool_state.set_fin();
    exec_thread_state.mmutex.lock();
    exec_thread_state.can_running.notify_all();
    exec_thread_state.mmutex.unlock();
    for(auto& pthread : thread_pool) {
        pthread.join();
    }
}

void TaskSystemParallelThreadPoolSleeping::thread_execute_func() {
    while(true) {
        {
            //need return?
            std::unique_lock<std::mutex> lk_pool_state(pool_state.mmutex);
            std::unique_lock<std::mutex> lk_dispatch_info(dispatch_info.mmutex);
            if(pool_state.is_finished == true and
            dispatch_info.next_id == dispatch_info.num_finished_job) { 
                return;
            }
        }
        
        {
            //need wait?
            std::unique_lock<std::mutex> lk_exec_thread_state(exec_thread_state.mmutex);
            if(exec_thread_state.have_task == false) {
                exec_thread_state.can_running.wait(lk_exec_thread_state);
                continue;
            }
        }

        TaskDe my_task_de;
        {
            //fetch the task
            std::unique_lock<std::mutex> lk_tasks_queue(tasks_queue.mmutex);
            if(tasks_queue.queue.empty()) continue;
            my_task_de = tasks_queue.queue.front(); tasks_queue.queue.pop();
            //if empty
            if(tasks_queue.queue.empty()) {
                std::unique_lock<std::mutex> lk_exec_thread_state(exec_thread_state.mmutex);
                exec_thread_state.have_task = false;
            }
        }
        auto& job_id = my_task_de.job_id;
        auto& runnable = my_task_de.runnable;
        auto& index = my_task_de.index;
        auto& num_total_tasks = my_task_de.num_total_tasks;
        runnable->runTask(index, num_total_tasks);

        {
            //push the job_id into the finished_queue
            std::unique_lock<std::mutex> lk_finished_queue(finished_queue.mmutex);
            finished_queue.queue.push(job_id);
        }
    }
}

void TaskSystemParallelThreadPoolSleeping::thread_dispatch_func() {
    while(true) {
        {
            //need return?
            std::unique_lock<std::mutex> lk_pool_state(pool_state.mmutex);
            std::unique_lock<std::mutex> lk_dispatch_info(dispatch_info.mmutex);
            if(pool_state.is_finished == true and
            dispatch_info.next_id == dispatch_info.num_finished_job) {
                return;
            }
        }

        std::vector<TaskID> finished_id;
        {
            //fetch the finished_task_id
            std::unique_lock<std::mutex> lk_finsihed_queue(finished_queue.mmutex);
            while(finished_queue.queue.size()) {
                finished_id.push_back(finished_queue.queue.front());
                finished_queue.queue.pop();
            }
        }

        std::vector<std::tuple<TaskID, IRunnable*, int>> ready_jobs;
        {
            //update the job_info_table
            std::unique_lock<std::mutex> lk_dispatch_info(dispatch_info.mmutex);
            std::vector<JobInfo>& tb = dispatch_info.job_info_table;
            for(auto& job_id : finished_id) {
                tb[job_id].num_finished_tasks++;
                if(tb[job_id].num_finished_tasks == tb[job_id].num_total_tasks) {
                    //job is finished
                    tb[job_id].is_finished = true;
                    dispatch_info.num_finished_job++;
                    //update the DAG
                    std::vector<std::vector<TaskID>>& g = dispatch_info.graph;
                    for(auto& vid : g[job_id]) {
                        if(--tb[vid].num_deps == 0) {
                            //if deps meet zero, ready to run
                            ready_jobs.push_back({vid, tb[vid].runnable, tb[vid].num_total_tasks});
                        }
                    }
                }
            }
        }

        {
            //push the ready_task_de into the queue
            std::unique_lock<std::mutex> lk_tasks_queue(tasks_queue.mmutex);
            std::unique_lock<std::mutex> lk_exec_thread_state(exec_thread_state.mmutex);
            for(auto& ready_job : ready_jobs) {
                auto& job_id = std::get<0>(ready_job);
                auto& runnable = std::get<1>(ready_job);
                auto& num_total_tasks = std::get<2>(ready_job);
                for(int i = 0; i < num_total_tasks; ++i) {
                    tasks_queue.queue.push({job_id, runnable, i, num_total_tasks});
                }
            }
            //if not empty
            if(tasks_queue.queue.size()) {
                exec_thread_state.have_task = true;
                exec_thread_state.can_running.notify_all();
            }
        }
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {


    //
    // TODO: CS149 students will modify the implementation of this
    // method in Parts A and B.  The implementation provided below runs all
    // tasks sequentially on the calling thread.
    //

    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //
    TaskID task_id;
    bool is_ready = false;
    {
        std::unique_lock<std::mutex> lk_dispatch_info(dispatch_info.mmutex);
        //asign an new task_id
        task_id = dispatch_info.next_id++;
        //create an new job_info
        dispatch_info.job_info_table.push_back({runnable, num_total_tasks, 0, 0, false});
        std::vector<JobInfo>& de = dispatch_info.job_info_table;
        for(auto dep_id : deps) {
            if(de[dep_id].is_finished == false) de[task_id].num_deps++;
        }
        if(de[task_id].num_deps == 0) is_ready = true;
        //update the DAG
        std::vector<std::vector<TaskID>>& g = dispatch_info.graph;
        g.push_back({});
        for(auto dep_id : deps) {
            g[dep_id].push_back(task_id); 
        }
    }

    if(is_ready){
        std::unique_lock<std::mutex> lk_tasks_queue(tasks_queue.mmutex);
        for(int i = 0; i < num_total_tasks; ++i) {
            tasks_queue.queue.push({task_id, runnable, i, num_total_tasks});
        }
        lk_tasks_queue.unlock();

        std::unique_lock<std::mutex> lk_exec_thread_state(exec_thread_state.mmutex);
        exec_thread_state.have_task = true;
        exec_thread_state.can_running.notify_all();
        lk_exec_thread_state.unlock();
    }
    return task_id;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //
    while(true) {
        dispatch_info.mmutex.lock();
        if(dispatch_info.next_id == dispatch_info.num_finished_job) {
            dispatch_info.mmutex.unlock();
            break;
        }
        dispatch_info.mmutex.unlock();
    }
    return;
}
