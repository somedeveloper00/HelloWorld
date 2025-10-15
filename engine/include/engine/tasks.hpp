#pragma once

#include "common/componentUtils.hpp"
#include "engine/app.hpp"
#include "engine/benchmark.hpp"
#include "engine/quickVector.hpp"
#include "errorHandling.hpp"
#include "thread.hpp"
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <tracy/Tracy.hpp>
#include <utility>

namespace engine
{
// multi-threading system for high performance scenarios and with minimal difficulties
struct tasks final
{
    // if -1, it'll be equal to the number of CPU
    static constexpr size_t ThreadCount = -1;

    // duration (in seconds) in which a thread will spin-lock until a new task arrives.
    // The more this value is, the faster the tasks are started, but the more CPU usage it'll cause.
    static constexpr float HighPerformanceSpinLockDuration = 0.1f;

    tasks() = delete;

    static inline void initialize()
    {
        ensureExecutesOnce();
        size_t count = ThreadCount == -1 ? std::thread::hardware_concurrency() : ThreadCount;
        while (--count > 0)
            new std::thread(threadPoolLoop_, count); // it's OK | we'll never want to delete these threads
        engine::application::postComponentHooks2.push_back([]() {
            // wait for all tasks to finish (also help them out)
            while (executeNextTaskIfAvailable())
            {
            }
            // reset for the next frame
            s_startedTasksCount = 0;
            s_tasksMutex.lock();
            s_tasks.clear();
            s_tasksMutex.unlock();
        });
    }

    // executes the function for the specified number of indices, guaranteed to finish by the end of this frame's application::postComponentHooks2.
    // Start is inclusive, End is exclusive.
    // the function must have only one parameter, being `size_t`; it'll refer to the index of the function.
    template <size_t Start, size_t End, typename Func>
        requires(Start <= End)
    static inline void executeLater(Func &&function)
    {
        if constexpr (Start == End)
            return;

        // delegate tasks to threads
        s_tasksMutex.lock();
        s_tasks.emplace_backRange(
            End - Start,
            /* taskEntry::function */ [&](const size_t) { return function; },
            /* taskEntry::index    */ [](const size_t i) { return Start + i; });
        s_tasksMutex.unlock();

        // wake up necessary threads
        s_taskWaitingLock.notify_all();
    }

    // executes the function for the specified number of indices right away (by the time it returns, all is done).
    // start is inclusive, end is exclusive.
    // the function must have only one parameter, being `size_t`; it'll refer to the index of the function.
    static inline void executeNow(const size_t start, const size_t end, Func &&function)
    {
#if DEBUG
        fatalAssert(start < end, "incorrect range was given.");
#endif

        // delegate tasks to threads (insert at a place so they'll get picked up next)
        s_tasksMutex.lock();
        const size_t finishedTasksCountEnd = s_startedTasksCount + end - start;
        tasks newTasks[end - start];
        for (size_t i = Start; i < End; i++)
            new (newTasks[i]) tasks{std::forward<Func>(function), i};
        s_tasks.insertRange(s_startedTasksCount, newTasks, end - start);
        s_tasksMutex.unlock();

        // wake up necessary threads
        s_taskWaitingLock.notify_all();

        // participate and check if all relevant tasks are started
        while (executeNextTaskIfAvailable() && s_startedTasksCount < finishedTasksCountEnd)
        {
        }

        // check tasks status (spin-lock | high performance)
    checkStatus:
        s_tasksMutex.lock();
        for (size_t i = start; i < end; i++)
            if (s_tasksStatus[i] != taskStatus::finished)
            {
                s_tasksMutex.unlock();
                goto checkStatus;
            }
        s_tasksMutex.unlock();
    }

  private:
    struct taskEntry
    {
#if TRACY_ENABLE
        const char *name;
        const size_t nameLength;
#endif
        const std::function<void(const size_t)> function;
        const size_t index;

        void executeOnThisThread() const noexcept
        {
#if TRACY_ENABLE
            benchDynamic(name, nameLength);
#endif
            function(index);
        }
    };

    struct taskGetResult
    {
        const bool found;

        // invalid if not found
        const taskEntry result;
    };

    enum taskStatus : uint8_t
    {
        notStarted,
        started,
        finished
    };

    // comment
    static inline void threadPoolLoop_(const size_t index)
    {
        threadInfo::setName(("task-thread-" + std::to_string(index)).c_str());
        log::logInfo("thread index {} started.", index);
        while (true) // never close | OS will handle closing
        {
            // execute until nothing's left
            while (executeNextTaskIfAvailable())
            {
                log::logInfo("thread index {} finished a task.", index);
            }

            {
                // wait until new task has arrived
                std::unique_lock<std::mutex> lock(s_tasksMutex);
                s_taskWaitingLock.wait(lock, []() {
                    return s_startedTasksCount < s_tasks.size();
                });
            }
        }
    }

    // executes the next task and returns true if any was executed, false if none was executed
    // also handles s_anyTasksLeftMutex
    static inline bool executeNextTaskIfAvailable()
    {
        s_tasksMutex.lock();
        const auto completed = s_startedTasksCount >= s_tasks.size();
        if (completed)
        {
            s_tasksMutex.unlock();
            return false;
        }

        // found. start new task at [s_startedTasksCount]
        const auto index = s_startedTasksCount++;
        const auto task = s_tasks[index];
        s_tasksStatus[index] = taskStatus::started;
        s_tasksMutex.unlock();

        {
            bench("task");
            log::logInfo("executing \"{}\"", task.name);
            task.executeOnThisThread();
        }

        // mark task status as finished
        s_tasksMutex.lock();
        s_tasksStatus[index] = taskStatus::finished;
        s_tasksMutex.unlock();

        return true;
    }

    // empties every frame
    static inline quickVectorZeroInitialize<taskEntry> s_tasks;
    static inline quickVectorZeroInitialize<taskStatus> s_tasksStatus;

    // for both s_tasks and s_tasksStatus
    static inline std::mutex s_tasksMutex;

    // correlates to s_tasks and resets every frame
    static inline std::atomic<size_t> s_startedTasksCount;

    // locks s_tasksMutex. used for threads to wait for new tasks (instead of spin-lock)
    static inline std::condition_variable s_taskWaitingLock;
};
} // namespace engine