#pragma once
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace sim {
struct ParallelCancelled : std::exception {
    const char* what() const noexcept override { return "Cancelled"; }
};

// The calling thread participates, so total compute threads never exceed the
// configured budget. Nested search jobs share these workers rather than spawning
// another pool. Nested waits help leaf jobs, never recurse into other episodes.
class ParallelExecutor {
    struct Group {
        std::atomic<size_t> remaining;
        std::exception_ptr error;
        explicit Group(size_t count) : remaining(count) {}
    };
    struct Job { std::function<void()> run; bool nested; };
    std::mutex mutex;
    std::condition_variable available;
    std::deque<Job> jobs;
    std::vector<std::thread> workers;
    bool stopping = false;
    inline static thread_local ParallelExecutor* current = nullptr;

    bool take(Job& job, bool nested_only) {
        for (auto it = jobs.end(); it != jobs.begin();) {
            --it;
            if (!nested_only || it->nested) {
                job = std::move(*it);
                jobs.erase(it);
                return true;
            }
        }
        return false;
    }
    void execute(Job& job) {
        auto previous = current;
        current = this;
        job.run();
        current = previous;
    }
    void stop() noexcept {
        { std::lock_guard lock(mutex); stopping = true; }
        available.notify_all();
        for (auto& worker : workers) if (worker.joinable()) worker.join();
    }
public:
    explicit ParallelExecutor(unsigned threads = 1) {
        threads = std::max(1u, threads);
        try {
            for (unsigned i = 1; i < threads; ++i) workers.emplace_back([this] {
                while (true) {
                    Job job;
                    {
                        std::unique_lock lock(mutex);
                        available.wait(lock, [&] { return stopping || !jobs.empty(); });
                        if (stopping && jobs.empty()) return;
                        take(job, false);
                    }
                    execute(job);
                }
            });
        } catch (...) { stop(); throw; }
    }
    ~ParallelExecutor() { stop(); }
    ParallelExecutor(const ParallelExecutor&) = delete;
    ParallelExecutor& operator=(const ParallelExecutor&) = delete;
    unsigned concurrency() const { return static_cast<unsigned>(workers.size() + 1); }

    template <typename Function>
    void parallel_for(size_t count, const Function& function) {
        if (count == 0) return;
        if (workers.empty()) {
            for (size_t i = 0; i < count; ++i) function(i);
            return;
        }
        const bool nested = current == this;
        auto group = std::make_shared<Group>(count);
        {
            std::lock_guard lock(mutex);
            const size_t original_size = jobs.size();
            try {
                for (size_t i = 0; i < count; ++i) jobs.push_back({[&, group, i] {
                    try { function(i); }
                    catch (...) {
                        std::lock_guard error_lock(mutex);
                        if (!group->error) group->error = std::current_exception();
                    }
                    { std::lock_guard completion_lock(mutex); group->remaining.fetch_sub(1); }
                    available.notify_all();
                }, nested});
            } catch (...) {
                // No worker can take these jobs while the lock is held. Remove
                // partial submissions before the referenced function unwinds.
                while (jobs.size() > original_size) jobs.pop_back();
                throw;
            }
        }
        available.notify_all();
        while (group->remaining.load() != 0) {
            Job job;
            {
                std::unique_lock lock(mutex);
                available.wait(lock, [&] {
                    return group->remaining.load() == 0 || std::any_of(jobs.begin(), jobs.end(),
                        [&](const Job& item) { return !nested || item.nested; });
                });
                if (group->remaining.load() == 0) break;
                take(job, nested);
            }
            execute(job);
        }
        if (group->error) std::rethrow_exception(group->error);
    }
};
}
