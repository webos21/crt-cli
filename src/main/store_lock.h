#pragma once
// Process-ownership helpers shared by the stores (SDK store, port store): a lock is a directory whose atomic creation is the
// lock, with the owner's PID inside. A lock whose owner is gone is stolen; a live owner's lock never is.
#include <crtcli/core.h>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <thread>
#include <unistd.h>

namespace crtcli {

// A process is alive when signal 0 reaches it. Another user's process counts as alive (EPERM): a lock must never be
// stolen from a process that may still be running.
inline bool process_alive(long pid) {
    if (pid <= 0) return false;
    if (kill(static_cast<pid_t>(pid), 0) == 0) return true;
    return errno == EPERM;
}

inline long current_pid() { return static_cast<long>(getpid()); }

inline std::string trim_line_end(std::string value) {
    while (!value.empty() && (value.back() == '\n' || value.back() == '\r' || value.back() == ' ')) value.pop_back();
    return value;
}

class Lock {
public:
    Lock(const fs::path& directory, unsigned wait_seconds, const std::string& busy_message = "another installation of this SDK is in progress")
        : directory_(directory) {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(wait_seconds);
        for (;;) {
            std::error_code error;
            if (fs::create_directory(directory_, error)) {
                write_text(directory_ / "owner", std::to_string(current_pid()) + "\n");
                held_ = true;
                return;
            }
            require(!error, "cannot create lock " + directory_.string() + ": " + error.message());
            if (steal_if_dead()) continue;
            if (std::chrono::steady_clock::now() >= deadline)
                throw Error(busy_message + " (lock " + directory_.string() + ")");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    ~Lock() { release(); }
    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;
    void release() {
        if (!held_) return;
        std::error_code ignored;
        fs::remove_all(directory_, ignored);
        held_ = false;
    }
private:
    bool steal_if_dead() {
        long owner = 0;
        try { owner = std::stol(trim_line_end(read_text(directory_ / "owner"))); } catch (const std::exception&) { owner = 0; }
        if (process_alive(owner)) return false;
        // The owner is gone: remove the lock, then let the loop retry the atomic create. Two takers race on that create,
        // and exactly one of them wins it.
        std::error_code ignored;
        fs::remove_all(directory_, ignored);
        return true;
    }
    fs::path directory_;
    bool held_ = false;
};

}  // namespace crtcli
