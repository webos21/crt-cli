#!/usr/bin/env python3
"""Run the extracted Windows CLI package SDK- and source-free under CRT's restricted-token trace. Python is a test-host tool.

The package directory (made by create_bootstrap_resources.py, packed and re-extracted by package_cli_archive.py) is copied into a
fresh private work directory. Every call runs under `crt_windows_trace`: a restricted token naming a per-run SID, an empty PATH,
redirected stdio and a job object that records every process the call starts (accounting, not denial: Windows offers no per-image
launch denial without changing ACLs). A native probe proves the producer, CLI and SDK trees unreadable under the SID, and the only
image any call may start is the package's own executable (plus the console host). Negative controls (wrong pin, one appended byte in a
notice, an undeclared extra file) must fail for their own stated reasons, and the restored package must pass again.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, required=True, help='the extracted package directory (holds manifest.json)')
    parser.add_argument('--manifest-sha256', required=True, help='the independently recorded manifest pin')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True, help='fresh private work directory (must not exist)')
    parser.add_argument('--probe', type=Path, required=True, help='SDK-built executable that must fail to read every --hide root')
    parser.add_argument('--crt-tools', type=Path, required=True)
    parser.add_argument('--hide', type=Path, action='append', default=[])
    args = parser.parse_args()
    if os.name != 'nt':
        parser.error('requires native Windows')
    sys.path.insert(0, str(args.crt_tools.resolve()))
    import crt_windows_external as sandbox
    import crt_windows_trace as trace

    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    work, sid = sandbox.create_work(args.work)
    hidden = sorted({p.resolve(strict=True) for p in args.hide})
    assert hidden and all(not work.is_relative_to(p) and not p.is_relative_to(work) for p in hidden), 'work must be outside hidden inputs'
    (work / 'tmp').mkdir(); (work / 'home').mkdir()
    package = work / 'package'
    shutil.copytree(args.package, package)
    probe = work / 'probe.exe'; shutil.copy2(args.probe, probe)
    manifest = json.loads((package / 'manifest.json').read_bytes())
    executable = package / manifest['executable']
    pin = args.manifest_sha256
    assert digest(package / 'manifest.json') == pin, 'the supplied pin is not this package manifest'
    environment = {'PATH': '', 'HOME': str(work / 'home'), 'USERPROFILE': str(work / 'home'), 'TEMP': str(work / 'tmp'),
                   'TMP': str(work / 'tmp'), 'SystemRoot': os.environ['SystemRoot'], 'LC_ALL': 'C'}
    allowed = {executable.resolve(), probe.resolve(), Path(os.environ['SystemRoot'], 'System32/conhost.exe').resolve()}
    results, commands = [], []

    def record(case, passed, detail=''):
        results.append({'case': case, 'passed': bool(passed), 'detail': str(detail)[:400]})
        print(('PASS ' if passed else 'FAIL ') + case, flush=True)

    def call(label, tail, expect_ok=True, expect_text=None, check=None):
        index = len(commands)
        out, err = output / f'call-{index:03}.stdout', output / f'call-{index:03}.stderr'
        result = trace.run([str(executable), '--json', *map(str, tail)], work=work, sid=sid, env=environment, cwd=work, stdout=out, stderr=err, timeout=300)
        stray = [p for p in result['processes'] if not p['image'] or Path(p['image']).resolve() not in allowed]
        commands.append({'case': label, 'argv': [str(x) for x in tail], 'exit': result['exit_code'], 'images': sorted({str(p['image']) for p in result['processes']})})
        try:
            reply = json.loads(out.read_bytes())
        except ValueError:
            reply = {'ok': False, 'data': {'message': out.read_text(errors='replace')[:300]}}
        message = json.dumps(reply.get('data', {}), ensure_ascii=False)
        passed = (result['exit_code'] == 0) == expect_ok and not result['timed_out'] and not stray
        if expect_text:
            passed = passed and expect_text in message
        if check:
            passed = passed and bool(check(reply))
        record(label, passed, message[:300] + (f' stray={stray[:2]}' if stray else ''))
        return reply

    boundary = [str(probe)]
    for path in hidden:
        boundary += ['--read', str(path)]
    probe_result = trace.run(boundary, work=work, sid=sid, env=environment, cwd=work, stdout=output / 'probe.stdout', stderr=output / 'probe.stderr', timeout=60)
    record('producer, CLI and SDK trees are unreadable under the run SID', probe_result['exit_code'] == 0, f'{len(hidden)} roots')
    assert probe_result['exit_code'] == 0, 'boundary probe failed'

    record('the extracted executable is the only native image', sum(1 for p in package.rglob('*') if p.is_file() and p.suffix.lower() in ('.exe', '.dll')) == 1)
    resources = ['doctor', '--scope', 'self', '--resources', str(package), '--resources-sha256', pin]

    version = trace.run([str(executable), '--version'], work=work, sid=sid, env=environment, cwd=work, stdout=output / 'version.stdout', stderr=output / 'version.stderr', timeout=60)
    record('--version', version['exit_code'] == 0 and '0.1.0-dev' in (output / 'version.stdout').read_text(errors='replace'), (output / 'version.stdout').read_text(errors='replace').strip())
    call('--help', ['--help'])
    bare = call('bare doctor --scope self', ['doctor', '--scope', 'self'])
    good = call('doctor --scope self with the pinned resources', resources,
                check=lambda r: r['data'].get('publisher_authentication') == 'not_verified' or 'not_verified' in json.dumps(r['data']))
    text = json.dumps(good.get('data', {}))
    record('the report states explicit-at-fetch trust and the compiled-in SDK input', 'explicit-at-fetch' in text and 'v0.0.1-preview.20261110.windows.1' in text, text[:300])

    # Negative controls: each must fail for its own reason, and the restored package must pass again.
    wrong = list(resources); wrong[wrong.index('--resources-sha256') + 1] = '0' * 64
    call('a wrong pin is refused', wrong, expect_ok=False, expect_text='manifest SHA-256 mismatch')
    notice = package / 'share/licenses/crt-cli/LICENSE'
    original = notice.read_bytes()
    notice.write_bytes(original + b'x')
    call('one appended byte in a notice is refused', resources, expect_ok=False, expect_text='inventory mismatch')
    notice.write_bytes(original)
    extra = package / 'share/undeclared.txt'
    extra.write_text('extra')
    call('an undeclared extra file is refused', resources, expect_ok=False, expect_text='inventory mismatch')
    extra.unlink()
    call('the restored package passes again', resources)
    record('the package is byte-for-byte unchanged', digest(package / 'manifest.json') == pin and notice.read_bytes() == original and not extra.exists())

    summary = {'passed': all(r['passed'] for r in results), 'cases': results, 'package_manifest_sha256': pin, 'executable_sha256': digest(executable),
               'work': str(work), 'isolation': 'Windows restricted token (per-run SID, empty PATH), private work directory, job-object process accounting; '
                                                'no network denial, no launch denial and no read-only root are claimed'}
    (output / 'result.json').write_text(json.dumps(summary, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    (output / 'commands.json').write_text(json.dumps(commands, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print('passed' if summary['passed'] else 'FAILED', f"{sum(r['passed'] for r in results)}/{len(results)}")
    return 0 if summary['passed'] else 1


if __name__ == '__main__':
    sys.exit(main())
