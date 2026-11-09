#include <crtcli/port_store.h>
#include "store_lock.h"
#include <algorithm>
#include <cstring>
#include <set>
#include <tuple>

namespace crtcli {
namespace {

constexpr uint64_t kMaxAuditedFileBytes = 512ull << 20;   // a file larger than this is refused rather than skimmed
constexpr size_t kMaxDynamicEntries = 65536;

bool is_hex64(const std::string& value) {
    return value.size() == 64 && std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isdigit(c) || (c >= 'a' && c <= 'f'); });
}

// ---- little-endian field access with bounds -----------------------------------------------------------------------------
struct Bytes {
    const std::string& data;
    uint64_t at(uint64_t offset, unsigned width, const char* what) const {
        if (offset > data.size() || width > data.size() - offset) throw Error(std::string("ELF is truncated at ") + what);
        uint64_t value = 0;
        for (unsigned i = 0; i < width; ++i) value |= static_cast<uint64_t>(static_cast<unsigned char>(data[offset + i])) << (8 * i);
        return value;
    }
};

bool under(const fs::path& path, const fs::path& base) {
    auto relative = path.lexically_normal().lexically_relative(base.lexically_normal());
    return !relative.empty() && *relative.begin() != "..";
}

std::vector<std::string> split_path_list(const std::string& value) {
    std::vector<std::string> parts;
    size_t start = 0;
    for (;;) {
        auto colon = value.find(':', start);
        parts.push_back(value.substr(start, colon == std::string::npos ? std::string::npos : colon - start));
        if (colon == std::string::npos) break;
        start = colon + 1;
    }
    return parts;
}

std::string replace_all(std::string text, const std::string& from, const std::string& to, size_t& count) {
    count = 0;
    if (from.empty()) return text;
    for (size_t at = 0; (at = text.find(from, at)) != std::string::npos; at += to.size()) { text.replace(at, from.size(), to); ++count; }
    return text;
}

ElfDynamic parse_elf(const std::string& data) {
    Bytes in{data};
    if (data.size() < 64) throw Error("ELF header is truncated");
    if (static_cast<unsigned char>(data[4]) != 2) throw Error("only 64-bit ELF is supported");
    if (static_cast<unsigned char>(data[5]) != 1) throw Error("only little-endian ELF is supported");
    if (static_cast<unsigned char>(data[6]) != 1) throw Error("unsupported ELF version");
    ElfDynamic info;
    auto type = in.at(16, 2, "e_type");
    info.kind = type == 3 ? "shared" : type == 2 ? "executable" : type == 1 ? "relocatable" : "";
    if (info.kind.empty()) throw Error("unsupported ELF type " + std::to_string(type));
    info.machine = static_cast<unsigned>(in.at(18, 2, "e_machine"));
    auto phoff = in.at(32, 8, "e_phoff");
    auto phentsize = in.at(54, 2, "e_phentsize"), phnum = in.at(56, 2, "e_phnum");
    if (phnum == 0) return info;
    if (phentsize != 56) throw Error("unexpected ELF program header size");
    struct Load { uint64_t offset, vaddr, filesz; };
    std::vector<Load> loads;
    bool has_dynamic = false;
    uint64_t dynamic_offset = 0, dynamic_size = 0;
    for (uint64_t i = 0; i < phnum; ++i) {
        auto base = phoff + i * 56;
        auto ptype = in.at(base, 4, "p_type");
        auto offset = in.at(base + 8, 8, "p_offset"), vaddr = in.at(base + 16, 8, "p_vaddr"), filesz = in.at(base + 32, 8, "p_filesz");
        if (ptype == 1) loads.push_back({offset, vaddr, filesz});
        if (ptype == 2) {
            if (has_dynamic) throw Error("ELF has two PT_DYNAMIC segments");
            has_dynamic = true; dynamic_offset = offset; dynamic_size = filesz;
        }
    }
    if (!has_dynamic) return info;
    if (dynamic_offset > data.size() || dynamic_size > data.size() - dynamic_offset) throw Error("PT_DYNAMIC lies outside the file");
    if (dynamic_size / 16 > kMaxDynamicEntries) throw Error("PT_DYNAMIC has too many entries");

    std::vector<uint64_t> needed, rpath, runpath;
    std::optional<uint64_t> soname, strtab, strsz;
    bool terminated = false;
    for (uint64_t at = 0; at + 16 <= dynamic_size; at += 16) {
        auto tag = in.at(dynamic_offset + at, 8, "d_tag"), value = in.at(dynamic_offset + at + 8, 8, "d_val");
        if (tag == 0) { terminated = true; break; }
        if (tag == 1) needed.push_back(value);
        else if (tag == 5) strtab = value;
        else if (tag == 10) strsz = value;
        else if (tag == 14) soname = value;
        else if (tag == 15) rpath.push_back(value);
        else if (tag == 29) runpath.push_back(value);
    }
    if (!terminated) throw Error("the dynamic section has no DT_NULL");
    if (needed.empty() && !soname && rpath.empty() && runpath.empty()) return info;
    if (!strtab || !strsz) throw Error("the dynamic section names strings but has no string table");
    uint64_t table = 0;
    bool mapped = false;
    for (const auto& load : loads)
        if (*strtab >= load.vaddr && *strtab - load.vaddr < load.filesz) { table = load.offset + (*strtab - load.vaddr); mapped = true; break; }
    if (!mapped) throw Error("the string table is not inside a loaded segment");
    if (table > data.size() || *strsz > data.size() - table) throw Error("the string table lies outside the file");
    auto string_at = [&](uint64_t index) {
        if (index >= *strsz) throw Error("a dynamic string index is outside the string table");
        auto begin = data.data() + table + index;
        auto limit = static_cast<size_t>(*strsz - index);
        auto end = static_cast<const char*>(memchr(begin, 0, limit));
        if (!end) throw Error("a dynamic string is not terminated");
        return std::string(begin, end);
    };
    for (auto index : needed) info.needed.push_back(string_at(index));
    if (soname) info.soname = string_at(*soname);
    for (auto index : rpath) info.rpath.push_back(string_at(index));
    for (auto index : runpath) info.runpath.push_back(string_at(index));
    return info;
}

// ---- Mach-O --------------------------------------------------------------------------------------------------------------
// 64-bit little-endian images only (arm64 and x86_64 macOS). Every offset is bounds-checked; a load command this reader
// cannot account for the length of is an error. Segments, symbol tables and signatures are not interpreted: the audit looks
// at what the dynamic loader will consume (install name, dependencies, run-path entries).
struct MachoBytes {
    const std::string& data;
    uint64_t at(uint64_t offset, unsigned width, const char* what) const {
        if (offset > data.size() || width > data.size() - offset) throw Error(std::string("Mach-O is truncated at ") + what);
        uint64_t value = 0;
        for (unsigned i = 0; i < width; ++i) value |= static_cast<uint64_t>(static_cast<unsigned char>(data[offset + i])) << (8 * i);
        return value;
    }
};

constexpr uint32_t kMachoMagic64 = 0xfeedfacf;
constexpr uint32_t kLcLoadDylib = 0xc, kLcIdDylib = 0xd, kLcRpath = 0x8000001c, kLcLoadWeakDylib = 0x80000018,
                   kLcReexportDylib = 0x8000001f, kLcLoadUpwardDylib = 0x80000023;

bool is_macho_magic(const std::string& data) {
    if (data.size() < 4) return false;
    uint32_t magic = 0;
    for (int i = 3; i >= 0; --i) magic = (magic << 8) | static_cast<unsigned char>(data[i]);
    // The 64-bit and 32-bit images in both byte orders, and the fat headers (stored big-endian): all are "a Mach-O" here, and
    // all but the first are then refused by the parser instead of being treated as plain data.
    return magic == kMachoMagic64 || magic == 0xfeedface || magic == 0xcffaedfe || magic == 0xcefaedfe || magic == 0xbebafeca || magic == 0xcafebabe ||
           magic == 0xbfbafeca || magic == 0xcafebabf;
}

MachoImage parse_macho(const std::string& data) {
    MachoBytes in{data};
    if (data.size() < 32) throw Error("Mach-O header is truncated");
    auto magic = in.at(0, 4, "magic");
    if (magic == 0xbebafeca || magic == 0xcafebabe || magic == 0xbfbafeca || magic == 0xcafebabf) throw Error("universal (fat) Mach-O files are not supported");
    if (magic != kMachoMagic64) throw Error("only 64-bit little-endian Mach-O is supported");
    MachoImage image;
    image.cpu_type = static_cast<unsigned>(in.at(4, 4, "cputype"));
    auto type = in.at(12, 4, "filetype");
    image.kind = type == 6 ? "shared" : type == 2 ? "executable" : type == 1 ? "relocatable" : "";
    if (image.kind.empty()) throw Error("unsupported Mach-O file type " + std::to_string(type));
    auto count = in.at(16, 4, "ncmds"), total = in.at(20, 4, "sizeofcmds");
    if (total > data.size() - 32) throw Error("the Mach-O load commands lie outside the file");
    if (count > total / 8) throw Error("the Mach-O header promises more load commands than fit");
    uint64_t offset = 32;
    const uint64_t end = 32 + total;
    bool has_id = false;
    for (uint64_t i = 0; i < count; ++i) {
        auto command = in.at(offset, 4, "cmd"), size = in.at(offset + 4, 4, "cmdsize");
        if (size < 8 || size % 8 != 0 || size > end - offset) throw Error("a Mach-O load command has an impossible size");
        auto name_at = [&](unsigned minimum) {
            if (size < minimum) throw Error("a Mach-O string command is shorter than its header");
            auto field = in.at(offset + 8, 4, "name offset");
            if (field < minimum || field >= size) throw Error("a Mach-O load command names a string outside itself");
            auto begin = data.data() + offset + field;
            auto limit = static_cast<size_t>(size - field);
            auto stop = static_cast<const char*>(memchr(begin, 0, limit));
            if (!stop) throw Error("a Mach-O load command string is not terminated");
            return std::string(begin, stop);
        };
        if (command == kLcIdDylib) {
            if (has_id) throw Error("Mach-O has two LC_ID_DYLIB commands");
            has_id = true;
            image.install_name = name_at(24);
        } else if (command == kLcLoadDylib || command == kLcLoadWeakDylib || command == kLcReexportDylib || command == kLcLoadUpwardDylib) {
            image.needed.push_back(name_at(24));
        } else if (command == kLcRpath) {
            image.rpaths.push_back(name_at(12));
        }
        offset += size;
    }
    if (offset != end) throw Error("the Mach-O load commands do not fill sizeofcmds");
    if (has_id && image.kind != "shared") throw Error("a Mach-O image that is not a dylib has an install name");
    return image;
}

unsigned macho_cpu_for(const std::string& arch) { return arch == "x86_64" ? 0x01000007u : arch == "aarch64" ? 0x0100000cu : 0u; }

unsigned machine_for(const std::string& arch) { return arch == "x86_64" ? 62 : arch == "aarch64" ? 183 : 0; }

std::string tool_name(const fs::path& path) { return path.filename().string(); }

Json adapter_json(const PortAdapter& adapter, const std::string& os) {
    // crt_review is deliberately absent: recording a review does not change what is built.
    Json json{{"recipe_sha256", adapter.recipe_sha256}, {"name", adapter.name}, {"version", adapter.version},
              {"host_tools", adapter.host_tools}, {"target_ports", adapter.target_ports}, {"variants", adapter.variants},
              {"sdk_stage", adapter.sdk_stage}, {"test_sources", adapter.test_sources},
              {"expected_files", adapter.expected_files}, {"expected_sonames", adapter.expected_sonames}};
    // Other hosts name their own layout; the Linux entry stays exactly as before so existing Linux keys do not change.
    if (os != "linux") {
        auto layout = port_layout(adapter, os);
        json = {{"recipe_sha256", adapter.recipe_sha256}, {"name", adapter.name}, {"version", adapter.version},
                {"host_tools", adapter.host_tools}, {"target_ports", adapter.target_ports}, {"variants", adapter.variants},
                {"sdk_stage", adapter.sdk_stage}, {"test_sources", adapter.test_sources}, {"os", os},
                {"expected_files", layout.expected_files}, {"expected_names", layout.expected_names},
                {"static_library", layout.static_library}, {"shared_library", layout.shared_library}};
    }
    return json;
}

struct Cleanup {
    fs::path path;
    ~Cleanup() { std::error_code ignored; fs::remove_all(path, ignored); }
};

}  // namespace

std::optional<ElfDynamic> read_elf_dynamic(const fs::path& file) {
    require(fs::is_regular_file(file), "not a regular file: " + file.string());
    require(fs::file_size(file) <= kMaxAuditedFileBytes, "file is too large to audit: " + file.string());
    auto data = read_text(file);
    if (data.size() < 4 || memcmp(data.data(), "\x7f" "ELF", 4) != 0) return std::nullopt;
    return parse_elf(data);
}

std::optional<MachoImage> read_macho_image(const fs::path& file) {
    require(fs::is_regular_file(file), "not a regular file: " + file.string());
    require(fs::file_size(file) <= kMaxAuditedFileBytes, "file is too large to audit: " + file.string());
    auto data = read_text(file);
    if (!is_macho_magic(data)) return std::nullopt;
    return parse_macho(data);
}

Json PortAudit::to_json() const {
    Json found = Json::array();
    for (const auto& finding : findings) found.push_back(finding.to_json());
    return {{"findings", found}, {"libraries", libraries}};
}

namespace {
std::string hex_value(unsigned value) {
    char buffer[16];
    snprintf(buffer, sizeof buffer, "%08x", value);
    return buffer;
}
}  // namespace

PortAudit audit_port_item(const PortAuditRequest& request) {
    PortAudit audit;
    auto add = [&](const std::string& path, const std::string& rule, const std::string& detail) { audit.findings.push_back({path, rule, detail}); };
    require(fs::is_directory(request.root) && !fs::is_symlink(request.root), "audit root is not a directory: " + request.root.string());
    auto root = fs::canonical(request.root);
    std::error_code error;
    auto sdk_canonical = fs::canonical(request.sdk, error);
    std::set<std::string> skip(request.skip_top_level.begin(), request.skip_top_level.end());

    struct Elf { std::string relative; fs::path directory; ElfDynamic info; };
    struct Macho { std::string relative; fs::path directory; MachoImage info; };
    std::vector<Elf> elves;
    std::vector<Macho> machos;
    std::set<std::string> sonames;
    const bool darwin = request.target_os == "macos";
    if (!darwin && request.target_os != "linux") throw Error("the binary audit supports linux and macos targets, not " + request.target_os);
    auto expected_machine = machine_for(request.target_arch);
    auto expected_cpu = macho_cpu_for(request.target_arch);
    if (!(darwin ? expected_cpu : expected_machine)) add("", "target", "unsupported target architecture '" + request.target_arch + "'");
    const auto layout = port_layout(request.adapter, request.target_os);

    for (fs::recursive_directory_iterator it(root, fs::directory_options::none), end; it != end; ++it) {
        const auto& path = it->path();
        auto relative = path.lexically_relative(root).generic_string();
        if (it.depth() == 0 && skip.count(path.filename().string())) { it.disable_recursion_pending(); continue; }
        if (it->is_symlink()) {
            auto target = fs::read_symlink(path);
            if (target.is_absolute()) { add(relative, "symlink-absolute", "points at " + target.string()); continue; }
            auto resolved = (path.parent_path() / target).lexically_normal();
            if (!under(resolved, root)) { add(relative, "symlink-escapes", "points at " + target.generic_string()); continue; }
            std::error_code chase;
            auto real = fs::canonical(path, chase);
            if (chase || !under(real, root)) { add(relative, "symlink-dangling", "does not resolve to a file inside the item"); continue; }
            if (!fs::is_regular_file(real)) add(relative, "symlink-target", "does not resolve to a regular file");
            continue;
        }
        if (it->is_directory()) continue;
        if (!it->is_regular_file()) { add(relative, "special-file", "neither a regular file, a directory nor a symlink"); continue; }
        auto permissions = fs::status(path).permissions();
        if ((permissions & (fs::perms::set_uid | fs::perms::set_gid | fs::perms::sticky_bit)) != fs::perms::none)
            add(relative, "special-mode", "setuid, setgid or sticky bit");
        if (fs::file_size(path) > kMaxAuditedFileBytes) { add(relative, "too-large", "larger than the audit limit"); continue; }
        auto data = read_text(path);
        for (const auto& needle : request.forbidden)
            if (!needle.empty() && data.find(needle) != std::string::npos) add(relative, "host-path-reference", "contains a build-job path");
        if (darwin && data.size() >= 4 && memcmp(data.data(), "\x7f" "ELF", 4) == 0) {
            add(relative, "foreign-format", "an ELF image in a macos item");
        } else if (data.size() >= 4 && memcmp(data.data(), "\x7f" "ELF", 4) == 0) {
            try {
                auto info = parse_elf(data);
                if (expected_machine && info.machine != expected_machine)
                    add(relative, "machine", "e_machine " + std::to_string(info.machine) + " is not the target's " + std::to_string(expected_machine));
                if (info.kind == "shared" && !info.soname.empty()) sonames.insert(info.soname);
                audit.libraries.push_back({{"path", relative}, {"kind", info.kind}, {"soname", info.soname}, {"needed", info.needed},
                                           {"runpath", info.runpath}, {"rpath", info.rpath}});
                elves.push_back({relative, path.parent_path(), std::move(info)});
            } catch (const Error& problem) {
                add(relative, "elf-unreadable", problem.what());
            }
        } else if (is_macho_magic(data)) {
            try {
                auto info = parse_macho(data);
                if (expected_cpu && info.cpu_type != expected_cpu)
                    add(relative, "machine", "cputype 0x" + hex_value(info.cpu_type) + " is not the target's 0x" + hex_value(expected_cpu));
                if (!darwin) add(relative, "foreign-format", "a Mach-O image in a " + request.target_os + " item");
                audit.libraries.push_back({{"path", relative}, {"kind", info.kind}, {"install_name", info.install_name}, {"needed", info.needed},
                                           {"rpath", info.rpaths}});
                machos.push_back({relative, path.parent_path(), std::move(info)});
            } catch (const Error& problem) {
                add(relative, "macho-unreadable", problem.what());
            }
        }
    }

    // Mach-O: what the dynamic loader will consume. Install names provided by this item satisfy @rpath dependencies.
    std::set<std::string> install_names;
    for (const auto& image : machos)
        if (image.info.kind == "shared" && !image.info.install_name.empty()) install_names.insert(image.info.install_name);
    for (const auto& image : machos) {
        for (const auto& name : image.info.needed) {
            bool ok = name == "/usr/lib/libSystem.B.dylib";
            for (const char* prefix : {"@rpath/", "@loader_path/"}) {
                if (name.rfind(prefix, 0) != 0) continue;
                auto base = name.substr(std::strlen(prefix));
                ok = !base.empty() && base.find('/') == std::string::npos &&
                     (fs::exists(request.sdk / "lib" / base) || install_names.count("@rpath/" + base) || install_names.count("@loader_path/" + base));
            }
            if (!ok) add(image.relative, "needed-not-in-sdk", "dependency " + name + " is neither libSystem, in the SDK lib/, nor provided by this item");
        }
        auto origin_directory = request.installed_at / fs::path(image.relative).parent_path();
        for (auto entry : image.info.rpaths) {
            if (entry.empty()) { add(image.relative, "runpath", "empty LC_RPATH"); continue; }
            if (entry.rfind("@loader_path", 0) == 0 && (entry.size() == 12 || entry[12] == '/')) entry = origin_directory.string() + entry.substr(12);
            else if (entry.rfind("@executable_path", 0) == 0) { add(image.relative, "runpath", "@executable_path entry " + entry + " cannot be resolved for a library"); continue; }
            if (entry.find('@') != std::string::npos) { add(image.relative, "runpath", "unexpanded token in " + entry); continue; }
            fs::path resolved = entry;
            if (!resolved.is_absolute()) { add(image.relative, "runpath", "relative entry " + entry); continue; }
            bool inside_item = under(resolved, request.installed_at) || resolved.lexically_normal() == request.installed_at.lexically_normal();
            bool inside_sdk = under(resolved, request.sdk) || (!sdk_canonical.empty() && under(resolved, sdk_canonical));
            if (!inside_item && !inside_sdk) add(image.relative, "runpath", entry + " is neither inside the item nor the SDK");
        }
        if (image.info.kind == "shared") {
            const auto& name = image.info.install_name;
            if (name.rfind("@rpath/", 0) != 0 || name.find('/', 7) != std::string::npos || name.size() == 7)
                add(image.relative, "install-name", "install name '" + name + "' is not @rpath/<file>");
            else if (!fs::is_regular_file(image.directory / name.substr(7)))
                add(image.relative, "install-name-alias", "no file or symlink named " + name.substr(7) + " beside it");
        }
    }

    for (const auto& elf : elves) {
        for (const auto& name : elf.info.needed) {
            bool ok = name.find('/') == std::string::npos &&
                      (fs::exists(request.sdk / "lib" / name) || sonames.count(name));
            if (!ok) add(elf.relative, "needed-not-in-sdk", "DT_NEEDED " + name + " is neither in the SDK lib/ nor provided by this item");
        }
        auto origin_directory = request.installed_at / fs::path(elf.relative).parent_path();
        for (const auto& list : {elf.info.rpath, elf.info.runpath})
            for (const auto& entries : list)
                for (auto entry : split_path_list(entries)) {
                    if (entry.empty()) { add(elf.relative, "runpath", "empty entry"); continue; }
                    for (const char* token : {"${ORIGIN}", "$ORIGIN"})
                        if (entry.rfind(token, 0) == 0) { entry = origin_directory.string() + entry.substr(std::strlen(token)); break; }
                    if (entry.find('$') != std::string::npos) { add(elf.relative, "runpath", "unexpanded token in " + entry); continue; }
                    fs::path resolved = entry;
                    if (!resolved.is_absolute()) { add(elf.relative, "runpath", "relative entry " + entry); continue; }
                    bool inside_item = under(resolved, request.installed_at);
                    bool inside_sdk = under(resolved, request.sdk) || (!sdk_canonical.empty() && under(resolved, sdk_canonical));
                    if (!inside_item && !inside_sdk) add(elf.relative, "runpath", entry + " is neither inside the item nor the SDK");
                }
        if (elf.info.kind == "shared" && !elf.info.soname.empty()) {
            auto alias = elf.directory / elf.info.soname;
            if (!fs::is_regular_file(alias)) add(elf.relative, "soname-alias", "no file or symlink named " + elf.info.soname + " beside it");
        }
    }
    for (const auto& name : layout.expected_names) {
        bool present = darwin ? install_names.count(name) > 0 : sonames.count(name) > 0;
        if (!present) add("", "expected-soname", std::string("no shared object carries ") + (darwin ? "install name " : "SONAME ") + name);
    }
    for (const auto& file : layout.expected_files)
        if (!fs::is_regular_file(root / file)) add(file, "expected-file", "missing from the install");
    std::sort(audit.findings.begin(), audit.findings.end(), [](const PortAuditFinding& a, const PortAuditFinding& b) {
        return std::tie(a.path, a.rule, a.detail) < std::tie(b.path, b.rule, b.detail);
    });
    return audit;
}

// ---- build key ----------------------------------------------------------------------------------------------------------
Json port_key_inputs(const PortBuildRequest& request) {
    require(request.toolchain && request.tools, "the build key needs a toolchain and host tools");
    const auto& recipe = request.recipe;
    const auto& adapter = port_adapter_for(recipe.recipe_sha256);
    require(adapter.target_ports.empty(), "target-port dependencies are not part of this slice");
    std::map<std::string, std::string> digests;   // canonical binary -> SHA-256 (cc and cxx are often one file)
    auto tool = [&](const fs::path& path) {
        auto real = fs::canonical(path).string();
        auto found = digests.find(real);
        if (found == digests.end()) found = digests.emplace(real, file_sha256(real)).first;
        return Json{{"name", tool_name(path)}, {"sha256", found->second}};
    };
    const auto& toolchain = *request.toolchain;
    auto identity = read_identity(toolchain.sdk);
    Json inputs = {{"format", 0}, {"executor", "crt-cli.port-executor.0"},
            {"recipe", {{"name", recipe.name}, {"version", recipe.version}, {"sha256", recipe.recipe_sha256}}},
            {"source", {{"sha256", recipe.source.sha256}}},
            {"adapter", sha256(adapter_json(adapter, recipe.target_os).dump())},
            {"sdk", identity.to_json()},
            {"target", native_target()},
            {"toolchain", {{"cc", tool(toolchain.cc)}, {"cxx", tool(toolchain.cxx)}, {"linker", tool(toolchain.linker)},
                           {"archiver", tool(toolchain.archiver)}, {"ranlib", tool(port_ranlib_for(toolchain.archiver))}}},
            {"host_tools", request.tools->identity()},
            {"variants", adapter.variants},
            {"environment", port_declared_environment(recipe)},
            {"dependencies", Json::array()}};
    if (native_target().at("os") == "macos") {
        // The OS SDK is outside the CRT SDK, so its identity is part of what the build depends on: SDKSettings.json names its
        // version and variant. The location is local state and is not recorded.
        auto settings = port_os_sdk(toolchain) / "SDKSettings.json";
        require(fs::is_regular_file(settings), "the configured macOS SDK has no SDKSettings.json");
        inputs["os_sdk"] = {{"kind", "macos-xcode-sdk"}, {"settings_sha256", file_sha256(settings)}};
    }
    return inputs;
}

std::string port_build_key(const Json& inputs) { return sha256(inputs.dump()); }

// ---- store --------------------------------------------------------------------------------------------------------------
PortStore::PortStore(fs::path root) : root_(fs::absolute(std::move(root)).lexically_normal()) {}

std::vector<std::string> PortStore::list() const {
    std::vector<std::string> keys;
    auto store = root_ / "store";
    if (!fs::is_directory(store)) return keys;
    for (const auto& entry : fs::directory_iterator(store))
        if (entry.is_directory() && !entry.is_symlink() && is_hex64(entry.path().filename().string())) keys.push_back(entry.path().filename().string());
    std::sort(keys.begin(), keys.end());
    return keys;
}

std::vector<std::string> PortStore::recover() {
    std::vector<std::string> removed;
    std::error_code error;
    for (const char* area : {".staging", ".locks"}) {
        auto directory = root_ / area;
        if (!fs::is_directory(directory)) continue;
        for (const auto& entry : fs::directory_iterator(directory, error)) {
            long owner = 0;
            // A staging directory is named <pid>-<tick>, so its owner is known from the moment it exists (txn.json is
            // written a moment later and must not decide whether a live publication is removed).
            try {
                if (std::string(area) == ".staging") owner = std::stol(entry.path().filename().string());
                else owner = std::stol(trim_line_end(read_text(entry.path() / "owner")));
            } catch (const std::exception&) { owner = 0; }
            if (process_alive(owner)) continue;
            std::error_code ignored;
            fs::remove_all(entry.path(), ignored);
            removed.push_back(std::string(area) + "/" + entry.path().filename().string());
        }
    }
    return removed;
}

uint64_t PortStore::verify(const std::string& build_key, const fs::path& sdk) const {
    require(is_hex64(build_key), "not a build key: " + build_key);
    auto item = item_path(build_key);
    require(fs::is_directory(item) && !fs::is_symlink(item), "no such port store item: " + build_key);
    auto manifest = read_json(item / "manifest.json");
    require(manifest.value("format", -1) == 0 && manifest.value("kind", "") == "crt.experimental.port-item", "unsupported port item manifest");
    require(manifest.value("build_key", "") == build_key, "the manifest names another build key");
    require(manifest.contains("key_inputs") && port_build_key(manifest.at("key_inputs")) == build_key, "the build key does not match its recorded inputs");
    auto actual = inventory(item);
    Json without = Json::array();
    for (const auto& entry : actual) if (entry.at(0) != "manifest.json") without.push_back(entry);
    require(manifest.contains("files") && manifest.at("files").is_array(), "the manifest has no file inventory");
    if (without != manifest.at("files")) {
        // Name the first difference: both lists are sorted by path.
        const auto& recorded = manifest.at("files");
        std::string where = "(lists differ in length: " + std::to_string(without.size()) + " found, " + std::to_string(recorded.size()) + " recorded)";
        for (size_t i = 0; i < without.size() && i < recorded.size(); ++i)
            if (without[i] != recorded[i]) { where = without[i].at(0).get<std::string>() + " / " + recorded[i].at(0).get<std::string>(); break; }
        throw Error("the item's files differ from its manifest inventory: " + where);
    }
    const auto& adapter = port_adapter_for(manifest.at("key_inputs").at("recipe").at("sha256").get<std::string>());
    PortAuditRequest audit{item, item, sdk, adapter, {}, {"evidence", "manifest.json"}, native_target().at("arch").get<std::string>(), native_target().at("os").get<std::string>()};
    auto result = audit_port_item(audit);
    if (!result.findings.empty())
        throw Error("port item " + build_key + " fails its audit: " + result.findings[0].path + " [" + result.findings[0].rule + "] " + result.findings[0].detail);
    return actual.size();
}

PortPublishResult PortStore::publish(const PortBuildRequest& request, const PortBuildResult& result, const Json& inputs) {
    require(request.toolchain && request.tools, "publish needs the request the build ran with");
    const auto& recipe = request.recipe;
    const auto& adapter = port_adapter_for(recipe.recipe_sha256);
    auto key = port_build_key(inputs);
    require(inputs.value("recipe", Json::object()).value("sha256", "") == recipe.recipe_sha256, "the key inputs describe another recipe");
    require(read_identity(request.toolchain->sdk).content_sha256 == inputs.at("sdk").at("content_sha256").get<std::string>(),
            "the SDK changed since the build key was computed");
    const auto& record = result.record;
    require(record.at("recipe").at("sha256") == recipe.recipe_sha256, "the build result is for another recipe");
    require(record.at("tests").size() == recipe.tests.size(), "the build result does not hold every recipe test");
    for (const auto& test : record.at("tests")) require(test.at("passed") == true, "recipe test did not pass: " + test.at("name").get<std::string>());
    require(fs::is_directory(result.prefix) && !fs::is_symlink(result.prefix), "the build result has no prefix directory");

    std::error_code error;
    for (const char* area : {"store", ".staging", ".locks"}) {
        require(!fs::is_symlink(root_ / area), "port store directory must not be a symlink: " + (root_ / area).string());
        fs::create_directories(root_ / area, error);
        require(!error, "cannot create " + (root_ / area).string() + ": " + error.message());
    }
    recover();
    auto transaction = std::to_string(current_pid()) + "-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    auto staging = root_ / ".staging" / transaction;
    fs::create_directories(staging, error);
    require(!error, "cannot create staging " + staging.string() + ": " + error.message());
    Cleanup cleanup{staging};
    write_text(staging / "txn.json", Json{{"pid", current_pid()}, {"build_key", key}, {"state", "staging"}}.dump(2) + "\n");

    auto item = staging / "item";
    fs::copy(result.prefix, item, fs::copy_options::recursive | fs::copy_options::copy_symlinks);
    auto final_path = item_path(key);

    // The only rewrite: pkg-config files record the prefix they were installed to, which is a job directory. They are text,
    // and a path inside them is data for a consumer's tools, so the staged copy names the item's real location. Any other
    // file naming the job is an audit failure below, not something to patch.
    Json relocations = Json::array();
    auto prefix_text = result.prefix.string();
    for (const auto& entry : fs::recursive_directory_iterator(item)) {
        if (entry.is_symlink() || !entry.is_regular_file() || entry.path().extension() != ".pc") continue;
        auto text = read_text(entry.path());
        if (text.find(prefix_text) == std::string::npos) continue;
        require(valid_utf8(text) && text.find('\0') == std::string::npos, "pkg-config file is not text: " + entry.path().string());
        size_t count = 0;
        write_text(entry.path(), replace_all(text, prefix_text, final_path.string(), count));
        relocations.push_back({{"file", entry.path().lexically_relative(item).generic_string()}, {"replacements", count}});
    }

    PortAuditRequest audit_request{item, final_path, request.toolchain->sdk, adapter, {prefix_text, result.prefix.parent_path().string()},
                                   {}, native_target().at("arch").get<std::string>(), native_target().at("os").get<std::string>()};
    auto audit = audit_port_item(audit_request);
    if (!audit.findings.empty()) {
        std::string message = "port build " + recipe.name + " fails the binary audit (" + std::to_string(audit.findings.size()) + " finding(s)):";
        for (size_t i = 0; i < audit.findings.size() && i < 10; ++i)
            message += "\n  " + audit.findings[i].path + " [" + audit.findings[i].rule + "] " + audit.findings[i].detail;
        throw Error(message);
    }

    fs::create_directories(item / "evidence");
    if (fs::is_directory(result.logs)) fs::copy(result.logs, item / "evidence" / "logs", fs::copy_options::recursive);
    Json steps = Json::array(), tests = Json::array();
    for (const auto& step : record.at("steps")) steps.push_back({{"name", step.at("name")}, {"exit_code", step.at("exit_code")}});
    for (const auto& test : record.at("tests")) tests.push_back({{"name", test.at("name")}, {"expected", test.at("expected")}, {"actual", test.at("actual")}, {"passed", test.at("passed")}});
    const bool darwin = native_target().at("os") == "macos";
    Json unsupported_scope = darwin
        ? Json::array({"macOS/arm64 only", "no other target port is consumed", "static and shared are produced together", "no license/SBOM data yet",
                       "audit covers Mach-O install names, dependencies, LC_RPATH and job-path leaks only", "images are not code-signed or notarized"})
        : Json::array({"Linux/x86_64 only", "no other target port is consumed", "static and shared are produced together",
                       "no license/SBOM data yet", "audit covers ELF DT_NEEDED/SONAME/runpath and job-path leaks only"});
    Json manifest{{"format", 0}, {"kind", "crt.experimental.port-item"}, {"build_key", key}, {"key_inputs", inputs},
                  {"recipe", record.at("recipe")}, {"files", inventory(item)}, {"libraries", audit.libraries},
                  {"relocations", relocations}, {"patches", record.at("patches")}, {"steps", steps}, {"tests", tests},
                  {"crt_review", adapter.crt_review},
                  {"unsupported_scope", unsupported_scope}};
    write_text(item / "manifest.json", manifest.dump(2) + "\n");

    Lock lock(root_ / ".locks" / (key + ".lock"), lock_wait_seconds, "another publication of this port build is in progress");
    if (fs::exists(final_path) || fs::is_symlink(final_path)) {
        // Same key: the same inputs. An existing item is trusted only after it re-validates; a damaged one is an error
        // for the user to resolve, never silently replaced.
        verify(key, request.toolchain->sdk);
        return {key, final_path, true, read_json(final_path / "manifest.json")};
    }
    write_text(staging / "txn.json", Json{{"pid", current_pid()}, {"build_key", key}, {"state", "publishing"}}.dump(2) + "\n");
    fs::rename(item, final_path);
    return {key, final_path, false, manifest};
}

}  // namespace crtcli
