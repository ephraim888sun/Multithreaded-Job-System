#include "jobworkerthread.h"
#include "jobsystem.h"

JobWorkerThread::JobWorkerThread(const char *uniqueName, unsigned long workerJobChannels, JobSystem *jobSystem) : 
    m_uniqueName(uniqueName),
    m_workerJobChannels(workerJobChannels),
    m_jobSystem(jobSystem)
{
}

JobWorkerThread::~JobWorkerThread(){
    // If we haven't already signal thread that we should exit as soon as it can(ater its current job if any)
    ShutDown();

    // Block on the thread's main until it has a chance to finish it current job and exit
    m_thread->join();
    delete m_thread;
    m_thread = nullptr;
};

void JobWorkerThread::StartUp(){
    m_thread = new std::thread(WorkerThreadMain, this);
};

void JobWorkerThread::Work()
{
    while(!IsStoppping()){
        m_workerStatusMutex.lock();
        unsigned long workerJobChannels = m_workerJobChannels;
        m_workerStatusMutex.unlock();

        Job *job = m_jobSystem->ClaimAJob(m_workerJobChannels);
        if(job){
            job->Execute();
            m_jobSystem->OnJobCompleted(job);
        }

        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
};

void JobWorkerThread::ShutDown()
{
    m_workerStatusMutex.lock();
    m_isStopping = true;
    m_workerStatusMutex.unlock();
};

bool JobWorkerThread::IsStoppping() const
{
    m_workerStatusMutex.lock();
    bool shouldClose = m_isStopping;
    m_workerStatusMutex.unlock();
    return shouldClose;
};

void JobWorkerThread::SetWorkerJobChannels(unsigned long workerJobChannels)
{
    m_workerStatusMutex.lock();
    m_workerJobChannels = workerJobChannels;
    m_workerStatusMutex.unlock();
}

void JobWorkerThread::WorkerThreadMain(void *workerThreadObject)
{
    JobWorkerThread *thisWorker = (JobWorkerThread *)workerThreadObject;
    thisWorker->Work();
}
