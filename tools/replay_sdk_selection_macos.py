#!/usr/bin/env python3
"""Native macOS selection/first-build adapter; Python/Clang/Seatbelt are test-host tools."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

from replay_first_build_cases import first_build_cases
from replay_sdk_selection_cases import selection_cases
from replay_macos_sandbox import MacSandbox, host_compile


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('cli', 'release-dir', 'output', 'work-parent', 'cc', 'cxx', 'linker', 'ar'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--hide', type=Path, action='append', default=[])
    parser.add_argument('--allow-dirty-sdk', action='store_true')
    parser.add_argument('--cases', choices=('selection', 'first-build'), default='selection')
    args = parser.parse_args()
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    args.work_parent.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix='selection space 한글 ', dir=args.work_parent.resolve()))
    hidden = [path.resolve() for path in args.hide]
    assert all(not work.is_relative_to(path) and not path.is_relative_to(work) for path in hidden)
    tools = {role: getattr(args, role).absolute() for role in ('cc', 'cxx', 'linker', 'ar')}
    cli = work / 'crt-cli'; shutil.copy2(args.cli, cli)
    inputs = work / 'inputs'; inputs.mkdir()
    manifest_path = next(args.release_dir.glob('release-manifest-dev-macos-*.json'))
    manifest = json.loads(manifest_path.read_bytes())
    asset = next(entry for entry in manifest['assets'] if 'install' in entry)
    for name in (manifest_path.name, asset['name'], asset['contract']):
        shutil.copy2(args.release_dir / name, inputs / name)
    inputs_before = {path.name: digest(path) for path in inputs.iterdir()}
    scratch = work / 'scratch'; scratch.mkdir()
    home = scratch / 'home space 한글'; home.mkdir()
    store = home / '.crt/sdks'
    poison = scratch / 'hostile cwd and PATH 한글'; poison.mkdir()
    sentinel = scratch / 'unexpected-tool-execution'
    trap_source = work / 'trap.c'
    trap_source.write_text('#include <stdio.h>\nint main(void) { FILE *f=fopen(' + json.dumps(str(sentinel), ensure_ascii=False) +
                           ', "w"); if(f) { fputs("unexpected tool", f); fclose(f); } return 97; }\n')
    host_compile(tools['cc'], trap_source, poison / 'clang')
    for name in ('clang++', 'ld', 'ld.lld', 'ar', 'llvm-ar', 'crt-cc', 'crt-c++', 'python3', 'sh'):
        shutil.copy2(poison / 'clang', poison / name)
    for name in ('libc.dylib', 'libc++.dylib', 'libc++.1.dylib', 'libz.dylib', 'liblzma.dylib'):
        (poison / name).write_bytes(b'not a loadable runtime\n')
    for name in ('stdio.h', 'stdlib.h', 'stddef.h', 'vector'):
        (poison / name).write_text('#error unexpected inherited include search path\n')
    sandbox = MacSandbox(); sandbox.prepare(work, scratch, cli, tools, hidden, hostile=poison)
    results, commands = [], []

    def record(case, passed, detail=''):
        results.append({'case': case, 'passed': bool(passed), 'detail': str(detail)[:500]})
        print(('PASS ' if passed else 'FAIL ') + case, flush=True)

    def call(label, tail, expect_ok=True, expect_text=None, readonly_paths=()):
        argv = sandbox.command(tail, home, readonly_paths=readonly_paths)
        proc = subprocess.run(argv, capture_output=True, text=True, timeout=600, **sandbox.process_options(home))
        number = len(commands)
        (output / f'call-{number:03}.stdout').write_text(proc.stdout)
        (output / f'call-{number:03}.stderr').write_text(proc.stderr)
        commands.append({'case': label, 'argv': argv, 'exit': proc.returncode})
        try:
            reply = json.loads(proc.stdout)
        except ValueError:
            reply = {}
        detail = json.dumps(reply.get('data', {}), ensure_ascii=False)
        passed = (proc.returncode == 0 if expect_ok else proc.returncode > 0) and reply.get('ok') is expect_ok
        if expect_text:
            passed = passed and expect_text in detail
        record(label, passed, detail)
        return reply

    try:
        call('install the original pinned release', ['sdk', 'install', '--archive', str(inputs / asset['name']),
             '--release-manifest', str(inputs / manifest_path.name), '--release-manifest-sha256', digest(inputs / manifest_path.name)])
        sdk_dir = asset['install']['version'] + '-' + asset['install']['content_sha256'][:12]
        before = {str(path.relative_to(store / sdk_dir)): digest(path) for path in (store / sdk_dir).rglob('*') if path.is_file()}
        aliases = scratch / 'explicit LLVM 한글'; aliases.mkdir()
        for role, tool in tools.items():
            alias = aliases / (tool.name if role in ('linker', 'ar') else role + ' compiler')
            alias.symlink_to(tool); tools[role] = alias
        if args.cases == 'selection':
            selection_cases(call=call, record=record, store=store, sdk_dir=sdk_dir, scratch=scratch, tools=tools,
                            allow_dirty_sdk=args.allow_dirty_sdk)
        else:
            def publication_failure(project, argv):
                call('read-only lock publication is refused', argv, expect_ok=False, readonly_paths=(project / 'crt.lock',))
            first_build_cases(call=call, record=record, store=store, sdk_dir=sdk_dir, scratch=scratch, tools=tools,
                              allow_dirty_sdk=args.allow_dirty_sdk, publication_failure=publication_failure)
        after = {str(path.relative_to(store / sdk_dir)): digest(path) for path in (store / sdk_dir).rglob('*') if path.is_file()}
        record('original SDK file bytes are unchanged', before == after)
        call('final original SDK verification', ['sdk', 'verify', sdk_dir])
    except Exception as error:
        record('selection replay completed without a harness exception', False, repr(error))
    finally:
        record('no colliding PATH tool was executed', not sentinel.exists())
        record('release inputs are unchanged', inputs_before == {path.name: digest(path) for path in inputs.iterdir()})
        sandbox.verify(record)
        summary = {'passed': bool(results) and all(case['passed'] for case in results), 'cases': results, 'work': str(work),
                   'cli_sha256': digest(cli), 'archive_sha256': digest(inputs / asset['name']),
                   'manifest_sha256': digest(inputs / manifest_path.name), 'allow_dirty_sdk': args.allow_dirty_sdk,
                   'suite': args.cases, 'isolation': sandbox.isolation}
        (output / 'result.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2) + '\n')
        (output / 'commands.json').write_text(json.dumps(commands, ensure_ascii=False, indent=2) + '\n')
    print(f"{sum(case['passed'] for case in results)}/{len(results)} passed")
    return 0 if summary['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
