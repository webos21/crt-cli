#!/usr/bin/env python3
"""Native Windows external C++17 consumer replay of an installed Dev SDK's platform-persona contract, through the CLI.

Producer item: CRT Windows C++ P1-01 (the Dev profile's `consumer_compile_options` used to undefine `_WIN32` for every consumer, so a
consumer's libc++ `std::filesystem::path` was a `char` string against the packaged `wchar_t` libc++: it linked and returned wrong paths).
The corrected SDK passes no consumer option and ships `curl/curl.h` and `mbedtls/platform.h` shims that drop the Windows macros only while
the upstream header is parsed. This driver proves that from outside the producer tree: the consumer is the native crt-cli with a cleared
environment, a private HOME and explicit LLVM tools; the SDK is installed from its release manifest (a pinned digest) into a private store; a C++17
project includes `<curl/curl.h>` and `<mbedtls/platform.h>` FIRST (the order that exposed the defect) and then exercises `std::filesystem`
lexically and against the real file system, `curl_socket_t`, libcurl and mbedtls linkage, for a static and a shared runtime. The consumer is built
with `build --sdk` and no configure step; its source is inside this file, not a producer input. The first case records the SDK profile's
`consumer_compile_options` (it must be empty); the same driver against an SDK that still injects `-U_WIN32` is a failing baseline, not a pass.
This is a cleared-environment replay, not the restricted-token adapter; nothing is hidden from the consumer and no network is denied.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

CONSUMER = r'''// External C++17 consumer: no platform-persona option, curl and mbedtls headers first on purpose.
#include <curl/curl.h>
#include <mbedtls/platform.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>

namespace fs = std::filesystem;

static int failures = 0;
static void check(bool ok, const char* what) {
  std::printf("%s %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) ++failures;
}

int main() {
  fs::path file("dir/sub/file.txt");
  check(file.filename().string() == "file.txt", "path filename");
  check(file.parent_path().generic_string() == "dir/sub", "path parent");
  check((fs::path("a") / "b" / "c.txt").generic_string() == "a/b/c.txt", "path append");
  check(file.extension().string() == ".txt" && file.stem().string() == "file", "path extension and stem");
  check(file.replace_extension(".md").filename().string() == "file.md", "path replace_extension");
  // The value type is the packaged libc++'s: a header parsed in the wrong persona made this a char string with no link error.
  check(std::is_same<fs::path::value_type, wchar_t>::value, "path value_type is the wchar_t of the packaged libc++");
  std::printf("path value_type bytes: %zu\n", sizeof(fs::path::value_type));

  fs::path root = fs::temp_directory_path() / "crt_cpp17_consumer_probe";
  std::error_code ec;
  fs::remove_all(root, ec);
  check(fs::create_directories(root / "inner" / "leaf", ec) && !ec, "create_directories");
  // Opened with the path itself: the std::fstream family's path constructors and open() compile and work in this persona since CRT PAL-05
  // (an SDK without that fix does not compile this line, which is the baseline for it).
  { std::ofstream out(root / "inner" / "data.bin", std::ios::binary); out << "0123456789"; }
  { std::ifstream in(root / "inner" / "data.bin", std::ios::binary); std::string text; in >> text; check(in.eof() && text == "0123456789", "ifstream(path) reads what ofstream(path) wrote"); }
  { std::ofstream more; more.open(root / "inner" / "data.bin", std::ios::binary | std::ios::app); more << "ab"; }
  check(fs::file_size(root / "inner" / "data.bin", ec) == 12, "ofstream::open(path, app) appends");
  { std::ofstream out(root / "inner" / "data.bin", std::ios::binary | std::ios::trunc); out << "0123456789"; }
  check(fs::exists(root / "inner" / "data.bin", ec) && fs::file_size(root / "inner" / "data.bin", ec) == 10, "exists and file_size");
  std::set<std::string> names;
  for (const auto& entry : fs::directory_iterator(root / "inner", ec)) names.insert(entry.path().filename().string());
  check(names == std::set<std::string>({"data.bin", "leaf"}), "directory_iterator names");
  fs::path through = fs::weakly_canonical(root / "inner" / ".." / "inner" / "data.bin", ec);
  check(!ec && through.filename().string() == "data.bin", "weakly_canonical");
  check(fs::remove_all(root, ec) >= 3 && !fs::exists(root, ec), "remove_all");

  check(sizeof(curl_socket_t) == sizeof(int), "curl_socket_t is the POSIX int the library was built with");
  curl_version_info_data* info = curl_version_info(CURLVERSION_NOW);
  check(info != nullptr && info->version != nullptr && std::strlen(info->version) > 0, "libcurl links and reports a version");
  char text[32];
  mbedtls_snprintf(text, sizeof(text), "%d-%s", 7, "ok");
  check(std::strcmp(text, "7-ok") == 0, "mbedtls_snprintf links and formats");
  std::printf(failures ? "cpp17_consumer: FAILED\n" : "cpp17_consumer: ok\n");
  return failures ? 1 : 0;
}
'''


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cli', type=Path, required=True)
    parser.add_argument('--release-dir', type=Path, required=True, help='directory with the SDK release manifest, archive and contract')
    parser.add_argument('--manifest-name', required=True)
    parser.add_argument('--manifest-sha256', required=True, help='the independently obtained manifest pin')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True, help='fresh work directory (must not exist)')
    parser.add_argument('--host-arch', required=True, choices=('x86_64', 'aarch64'), help='the architecture this machine natively runs (recorded, checked against the SDK)')
    for name in ('cc', 'cxx', 'linker', 'ar', 'readobj'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('requires native Windows')
    output, work = args.output.resolve(), args.work.resolve()
    output.mkdir(parents=True)
    work.mkdir(parents=True)
    home, projects, inputs = work / 'home', work / 'projects', work / 'inputs'
    for path in (home, projects, inputs, work / 'tmp'):
        path.mkdir()
    cli = work / 'crt-cli.exe'
    shutil.copy2(args.cli, cli)
    for source in sorted(args.release_dir.iterdir()):
        if source.is_file():
            shutil.copy2(source, inputs / source.name)
    manifest_path = inputs / args.manifest_name
    assert digest(manifest_path) == args.manifest_sha256, 'the supplied pin is not this manifest'
    manifest = json.loads(manifest_path.read_bytes())
    archive = next(a for a in manifest['assets'] if 'install' in a)
    content = archive['install']['content_sha256']
    sdk_dir = home / '.crt' / 'sdks' / (archive['install']['version'] + '-' + content[:12])
    environment = {'HOME': str(home), 'USERPROFILE': str(home), 'PATH': '', 'SystemRoot': os.environ['SystemRoot'], 'LC_ALL': 'C',
                   'TEMP': str(work / 'tmp'), 'TMP': str(work / 'tmp'), 'CRT_WINDOWS_SDK_LIBPATH': os.environ['CRT_WINDOWS_SDK_LIBPATH']}
    tool_flags = ['--cc', str(args.cc.absolute()), '--cxx', str(args.cxx.absolute()), '--linker', str(args.linker.absolute()), '--ar', str(args.ar.absolute())]
    results, commands = [], []

    def record(case, passed, detail=''):
        results.append({'case': case, 'passed': bool(passed), 'detail': str(detail)[:500]})
        print(('PASS ' if passed else 'FAIL ') + case, flush=True)

    def run(label, argv, env, timeout=1800):
        index = len(commands)
        done = subprocess.run([str(a) for a in argv], capture_output=True, text=True, encoding='utf-8', errors='replace', env=env, cwd=work,
                              timeout=timeout, stdin=subprocess.DEVNULL)
        (output / f'call-{index:03}.stdout').write_text(done.stdout, encoding='utf-8')
        (output / f'call-{index:03}.stderr').write_text(done.stderr, encoding='utf-8')
        commands.append({'case': label, 'argv': [str(a) for a in argv], 'exit': done.returncode})
        return done

    def cli_json(label, tail, expect_ok=True):
        done = run(label, [cli, '--json', *tail], environment)
        try:
            reply = json.loads(done.stdout)
        except ValueError:
            reply = {}
        message = json.dumps(reply.get('data', {}), ensure_ascii=False)
        passed = (done.returncode == 0 and reply.get('ok') is True) if expect_ok else (done.returncode > 0 and reply.get('ok') is False)
        record(label, passed, message[:300])
        return reply, passed

    record('the consumer source is an identified input of this driver', True, 'cpp17_consumer sha256 ' + hashlib.sha256(CONSUMER.encode('utf-8')).hexdigest()[:16])
    cli_json('sdk install from the pinned release manifest', ['sdk', 'install', '--archive', inputs / archive['name'], '--release-manifest', manifest_path,
                                                              '--release-manifest-sha256', args.manifest_sha256])
    record('the SDK landed in the private store', sdk_dir.is_dir(), str(sdk_dir))
    system = Path(os.environ['SystemRoot']) / 'System32'
    with_sdk = {'SystemRoot': os.environ['SystemRoot'], 'PATH': str(sdk_dir / 'bin') + os.pathsep + str(system), 'TEMP': str(work / 'tmp'), 'TMP': str(work / 'tmp')}

    # The SDK states its own consumer options in its development contract: a corrected SDK passes none to any consumer.
    contract = json.loads((sdk_dir / 'share' / 'crt' / 'contracts' / 'development.json').read_text(encoding='utf-8'))
    options = []

    def collect(node):
        if isinstance(node, dict):
            for key, value in node.items():
                if key == 'consumer_compile_options':
                    options.extend(value)
                collect(value)
        elif isinstance(node, list):
            for value in node:
                collect(value)

    collect(contract)
    record('the SDK contract declares no consumer persona option (no -U_WIN32)', not options, f'consumer_compile_options={options}')
    for shim in ('curl/curl.h', 'mbedtls/platform.h'):
        record('the SDK ships the CRT-owned persona shim ' + shim, (sdk_dir / 'include' / shim).is_file() and 'crt' in (sdk_dir / 'include' / shim).read_text(encoding='utf-8', errors='replace').lower(), shim)

    def project(name, linkage):
        root = projects / name
        cli_json('new ' + name, ['new', root, '--language', 'cxx'])
        spec = {'manifest_format': 1, 'package': {'name': name.replace('-', '_'), 'version': '0.1.0'},
                'sdk': {'profile': 'dev', 'version': '*', 'target': 'native', 'storage': 'shared-immutable'}, 'include_dirs': ['include'],
                'targets': [{'name': 'app', 'kind': 'executable', 'runtime_linkage': linkage, 'sources': ['src/main/main.cpp'],
                             'sdk_libraries': ['curl', 'mbedtls', 'mbedx509', 'mbedcrypto', 'z'], 'sdk_library_linkage': 'static'}]}
        (root / 'crt.json').write_text(json.dumps(spec, indent=2) + '\n', encoding='utf-8')
        for folder in ('src/main', 'src/test'):
            for stale in (root / folder).iterdir():
                stale.unlink()
        (root / 'src/main/main.cpp').write_text(CONSUMER, encoding='utf-8')
        return root

    for linkage in ('static', 'shared'):
        name = 'cpp17-' + linkage
        root = project(name, linkage)
        reply, ok = cli_json(name + ': cold build', ['build', '--project', root, '--sdk', content, *tool_flags])
        cold = reply.get('data', {}).get('cache', {})
        reply, _ = cli_json(name + ': warm build executes nothing', ['build', '--project', root])
        warm = reply.get('data', {}).get('cache', {})
        record(name + ': the cold build executed commands and the warm build none', cold.get('executed', 0) > 0 and warm.get('executed') == 0, f'cold={cold} warm={warm}')
        exe = next(iter(sorted(root.rglob('app.exe'))), None)
        record(name + ': the executable exists', exe is not None)
        if exe is None:
            continue
        done = run(name + ': run with the SDK bin on PATH', [exe], with_sdk)
        passed = done.returncode == 0 and 'cpp17_consumer: ok' in done.stdout and 'FAIL' not in done.stdout
        record(name + ': every filesystem, curl and mbedtls check passes', passed, done.stdout.replace('\n', ' | ')[-400:])
        value_bytes = [line for line in done.stdout.splitlines() if line.startswith('path value_type bytes:')]
        record(name + ': path::value_type is the wchar_t of the packaged libc++ (CRT builds wchar_t as int, so 4 bytes)', value_bytes == ['path value_type bytes: 4'], value_bytes)
        cli_json(name + ': crt-cli run', ['run', '--project', root])

    summary = {'passed': all(r['passed'] for r in results), 'cases': results, 'host_arch': args.host_arch, 'cli_sha256': digest(cli),
               'manifest_sha256': args.manifest_sha256, 'sdk_content_sha256': content, 'sdk_directory': str(sdk_dir),
               'consumer_source_sha256': hashlib.sha256(CONSUMER.encode('utf-8')).hexdigest(),
               'isolation': 'cleared environment, private HOME and explicit LLVM tools; not the restricted-token adapter; nothing hidden, no network denial'}
    (output / 'result.json').write_text(json.dumps(summary, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    (output / 'commands.json').write_text(json.dumps(commands, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    print(f"{sum(r['passed'] for r in results)}/{len(results)} cases passed")
    return 0 if summary['passed'] else 1


if __name__ == '__main__':
    sys.exit(main())
