#include "pch.h"
#include "JobQueue.h"

#include "Job.h"
void ServerEngine::JobQueue::FlushJobQueue()
{
    MoveReservedJobs();

    std::shared_ptr<Job> job;
    while(m_jobs.try_pop(job)) {
        if(job)
            job->Execute();
    }
}

void ServerEngine::JobQueue::MoveReservedJobs()
{
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
