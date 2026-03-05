#pragma once

#include "Job.h"

namespace ServerEngine {
    class Job;
    using Clock = std::chrono::high_resolution_clock;

    class JobQueue {
        struct ReservedJob {
            Clock::time_point executeTime;
            std::shared_ptr<Job> job;

            bool operator>(const ReservedJob& other) const
            {
                return executeTime > other.executeTime;
            }
        };
    public:
        virtual ~JobQueue() = default;

        template<typename T, typename Ret, typename... FArgs, typename... CallArgs>
        void PushJob(Ret(T::* memFunc)(FArgs...), CallArgs&&... args)
        {
            m_jobs.push(MakeJob(memFunc, std::forward<CallArgs>(args)...));
        }

        template<typename T, typename Ret, typename... FArgs, typename... CallArgs>
        void PushJobAfter(std::chrono::milliseconds ms, Ret(T::* memFunc)(FArgs...), CallArgs&&... args)
        {
            auto job = MakeJob(memFunc, std::forward<CallArgs>(args)...);
            auto executeTime = Clock::now() + ms;

            m_reservedJobs.push({ executeTime, std::move(job) });
        }

    protected:
        void FlushJobQueue();

    private:
        template<typename T, typename Ret, typename... FArgs, typename... CallArgs>
        std::shared_ptr<Job> MakeJob(Ret(T::* memFunc)(FArgs...), CallArgs&&... args)
        {
            return std::make_shared<Job>(static_cast<T*>(this), memFunc, std::forward<CallArgs>(args)...);
            // return ObjectPool<Job>::MakeShared(owner, memFunc, std::forward<CallArgs>(args)...);
        }   
        
        void MoveReservedJobs();

    private:
        tbb::concurrent_queue<std::shared_ptr<Job>> m_jobs;
        tbb::concurrent_priority_queue<ReservedJob, std::greater<ReservedJob>> m_reservedJobs;
    };
}


