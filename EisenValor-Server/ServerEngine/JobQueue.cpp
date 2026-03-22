#include "pch.h"
#include "JobQueue.h"

void GameServerEngine::JobQueue::FlushJobQueue()
{
    MoveReservedJobs();

    if(m_jobs.empty())
        return;

    std::shared_ptr<Job> job;
    while(m_jobs.try_pop(job)) {
        if(job)
            job->Execute();
    }
}

void GameServerEngine::JobQueue::MoveReservedJobs()
{
    if(m_reservedJobs.empty())
        return;

    const auto now = Clock::now();

    ReservedJob topJob;

    while(m_reservedJobs.try_pop(topJob)) {
        if(topJob.executeTime <= now) {
            m_jobs.push(topJob.job);
        }
        else {
            m_reservedJobs.push(std::move(topJob));
            break;
        }
    }
}