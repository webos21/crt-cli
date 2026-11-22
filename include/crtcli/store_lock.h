#pragma once
// Process-ownership helpers shared by the stores (SDK store, port store).
//
// A Lock is an exclusive OS lock (flock) on a small file. The operating system releases it when the holding process
// exits, however it exits, so no lock is ever "stale" and nothing is ever stolen: the earlier design (a directory whose
// creation is the lock, with the owner's PID inside) stole live locks whose owner was not yet written and treated
// create/remove races as fatal. The lock file itself is never removed; it holds only the owner's PID for diagnostics.
// A directory at the lock path is a leftover of that earlier layout: it is waited on while its owner may be alive and
// taken over only once its owner is gone, or its owner record has stayed unreadable for a grace period.
#include <crtcli/core.h>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <thread>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
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
    Lock(const fs::path& path, unsigned wait_seconds, const std::string& busy_message = "another installation of this SDK is in progress")
        : path_(path) {
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(wait_seconds);
        std::error_code error;
        fs::create_directories(path_.parent_path(), error);
        for (;;) {
            if (fs::is_directory(path_, error) && !fs::is_symlink(path_, error)) {
                if (take_over_legacy_directory()) continue;
            } else if (try_lock()) {
                return;
            }
            if (std::chrono::steady_clock::now() >= deadline)
                throw Error(busy_message + " (lock " + path_.string() + ")");
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
    ~Lock() { release(); }
    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;
    void release() {
        if (fd_ < 0) return;
        flock(fd_, LOCK_UN);
        close(fd_);
        fd_ = -1;
    }
private:
    // True when this process now holds the lock. A busy lock (held by another process) is not an error; an unusable path is.
    bool try_lock() {
        int fd = open(path_.string().c_str(), O_RDWR | O_CREAT, 0644);
        if (fd < 0) {
            if (errno == EINTR) return false;
            throw Error("cannot open lock " + path_.string() + ": " + std::strerror(errno));
        }
        if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
            int reason = errno;
            close(fd);
            // A conflicting lock reports EWOULDBLOCK/EAGAIN, or EACCES from record-lock emulations.
            if (reason == EWOULDBLOCK || reason == EAGAIN || reason == EACCES || reason == EINTR) return false;
            throw Error("cannot lock " + path_.string() + ": " + std::strerror(reason));
        }
        fd_ = fd;
        // Diagnostic only; the lock does not depend on it.
        const std::string owner = std::to_string(current_pid()) + "\n";
        if (ftruncate(fd_, 0) == 0 && lseek(fd_, 0, SEEK_SET) == 0) { auto ignored = write(fd_, owner.data(), owner.size()); (void)ignored; }
        return true;
    }
    // A directory lock from the earlier layout. Returns true when it was removed, so the caller retries at once.
    bool take_over_legacy_directory() {
        long owner = 0;
        try { owner = std::stol(trim_line_end(read_text(path_ / "owner"))); } catch (const std::exception&) { owner = 0; }
        if (process_alive(owner)) return false;
        if (owner == 0) {
            // The creator may not have written its owner yet: only a directory that stays ownerless for a while is abandoned.
            std::error_code error;
            auto age = fs::file_time_type::clock::now() - fs::last_write_time(path_, error);
            if (error || age < std::chrono::seconds(10)) return false;
        }
        // Remove only what the earlier layout created: its owner file, then the directory itself. rmdir refuses a
        // regular file, so a lock file that another process created at this path after the checks above survives
        // (remove_all would have deleted it and let two processes hold the lock). Anything else in the directory
        // is not ours to delete: leave it and keep waiting until the deadline.
        std::error_code ignored;
        fs::remove(path_ / "owner", ignored);
        if (rmdir(path_.string().c_str()) == 0) return true;
        return errno == ENOENT || errno == ENOTDIR;  // the path changed under us: re-examine it at once
    }
    fs::path path_;
    int fd_ = -1;
};

}  // namespace crtcli
