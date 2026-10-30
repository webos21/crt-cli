#pragma once
#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>
#include <vector>
namespace crtcli {
using Json = nlohmann::json;
namespace fs = std::filesystem;
struct Error : std::runtime_error {
    int code;
    explicit Error(const std::string& message, int status = 1) : std::runtime_error(message), code(status) {}
};
struct ProcessResult { int exit_code; bool timed_out; std::string output; bool output_truncated; };
struct Reply { int code; std::string command; Json data; std::string message; };
std::string read_text(const fs::path& path);
void write_text(const fs::path& path, const std::string& value);
Json read_json(const fs::path& path);
std::string sha256(const std::string& bytes);
std::string file_sha256(const fs::path& path);
Json inventory(const fs::path& root);
Json native_target();
fs::path sdk_store_root();
fs::path inside(const fs::path& root, const std::string& relative);
void require(bool condition, const std::string& message);
void native_executable(const fs::path& path);
// host_tool marks an external native program (compiler, linker, archiver): on Windows CRT's posix_spawn only redirects
// stdio for a non-CRT child when told so (CRT_SPAWN_NATIVE_WINDOWS); CRT-built children need no hint.
// runtime_path lists directories the child's dynamic images are loaded from. Only Windows needs it (no RPATH there, and the
// per-child PATH is otherwise empty); the other hosts resolve the SDK runtime through the linked image itself.
ProcessResult execute(const std::vector<std::string>& argv, unsigned timeout_seconds = 180, bool relay = true, bool host_tool = false,
                      const std::vector<std::string>& runtime_path = {});
int print_reply(const Reply& reply, bool json);
Reply workflow(const std::vector<std::string>& args);
Reply fetch(const std::vector<std::string>& args);
Reply bootstrap_doctor();
}
