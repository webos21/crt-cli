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
ProcessResult execute(const std::vector<std::string>& argv, unsigned timeout_seconds = 180, bool relay = true);
int print_reply(const Reply& reply, bool json);
Reply workflow(const std::vector<std::string>& args);
Reply bootstrap_doctor();
}
