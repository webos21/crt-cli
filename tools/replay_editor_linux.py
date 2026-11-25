#!/usr/bin/env python3
"""Native P0 consumer replay. Python, Bubblewrap and strace are test-host tools only."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('cli', 'test', 'project', 'home', 'output', 'work'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--hide', action='append', type=Path, default=[])
    args = parser.parse_args()
    for name in ('bwrap', 'strace', 'readelf'):
        if not shutil.which(name):
            parser.error('missing test-host tool: ' + name)
    project, home = args.project.resolve(), args.home.resolve()
    output, work = args.output.resolve(), args.work.resolve()
    if work.is_relative_to('/tmp'):
        parser.error('work must be outside the private /tmp')
    output.mkdir(parents=True, exist_ok=False)
    work.mkdir(parents=True, exist_ok=False)
    fixture = work / 'fixtures'
    fixture.mkdir()
    shutil.copyfile(project / 'src/test/data/editor_api.json', fixture / 'editor_api.json')
    scratch = work / 'scratch'; scratch.mkdir()
    evidence = output / 'test-evidence'; evidence.mkdir()
    (project / 'out/editor-test').mkdir(exist_ok=True)
    commands = []
    local = json.loads((project / '.crt/local.json').read_text())

    def command(label, argv, readonly_project=None):
        prefix = [shutil.which('bwrap'), '--unshare-user', '--unshare-ipc', '--unshare-uts', '--unshare-net',
                  '--die-with-parent', '--new-session', '--ro-bind', '/', '/', '--proc', '/proc', '--dev', '/dev',
                  '--tmpfs', '/tmp', '--clearenv', '--setenv', 'HOME', str(home), '--setenv', 'PATH', '/nonexistent',
                  '--setenv', 'TMPDIR', str(scratch), '--setenv', 'LC_ALL', 'C', '--bind', str(scratch), str(scratch),
                  '--bind', str(output), str(output), '--bind', str(evidence), str(project / 'out/editor-test'),
                  '--tmpfs', str(project / 'src'), '--dir', str(project / 'src/test'),
                  '--ro-bind', str(fixture), str(project / 'src/test/data'), '--tmpfs', str(project / 'include')]
        for path in args.hide:
            path = path.resolve()
            if any(p.is_relative_to(path) for p in (work, output, project, home, args.cli.resolve(), args.test.resolve())):
                raise ValueError('hidden input overlaps an active replay input')
            prefix += ['--tmpfs', str(path)]
        if readonly_project is not None:
            prefix += ['--ro-bind', str(readonly_project), str(readonly_project)]
        prefix += [shutil.which('strace'), '-f', '-qq', '-xx', '-s', '4096', '-e', 'trace=execve',
                   '-o', str(output / (label + '.execve')), '--', *map(str, argv)]
        done = subprocess.run(prefix, capture_output=True, text=True, timeout=1800)
        (output / (label + '.stdout')).write_text(done.stdout, encoding='utf-8')
        (output / (label + '.stderr')).write_text(done.stderr, encoding='utf-8')
        commands.append({'label': label, 'argv': prefix, 'exit': done.returncode})
        return done

    passed, error = False, None
    try:
        for index, path in enumerate([*[p.resolve() / 'TODO.md' for p in args.hide], project / 'src/main/main.cpp']):
            done = command('visibility-' + str(index), ['/usr/bin/test', '!', '-e', path])
            if done.returncode:
                raise ValueError('original source visibility probe failed')
        done = command('editor-test', [args.test.resolve(), args.cli.resolve(), project])
        if done.returncode or not re.search(r'editor-test: [1-9][0-9]* checks passed', done.stdout):
            raise ValueError('native editor test failed; see editor-test.stderr')
        test = json.loads((evidence / 'result.json').read_text())
        consumer = Path(test['project'])
        directories = []
        for entry in test['compile_commands']:
            argv = [shutil.which('readelf'), '--debug-dump=info', '--dwarf-depth=1', entry['output']]
            inspected = subprocess.run(argv, capture_output=True, text=True, check=True, timeout=60)
            observed = [line for line in inspected.stdout.splitlines() if 'DW_AT_comp_dir' in line]
            if not any(line.rstrip().endswith(entry['directory']) for line in observed):
                raise ValueError('actual compiler working directory differs from the exported entry')
            directories.append({'output': entry['output'], 'directory': entry['directory'], 'argv': argv,
                                'object_sha256': digest(entry['output']), 'observed': observed})
        for label, tail in [('readonly-info', ['project', 'info', '--project', consumer]),
                            ('readonly-export', ['export', 'compile-commands', '--project', consumer,
                                                 '--output', output / 'readonly-compile_commands.json'])]:
            done = command(label, [args.cli.resolve(), '--json', *tail], consumer)
            reply = json.loads(done.stdout)
            if done.returncode or reply.get('ok') is not True:
                raise ValueError(label + ' failed on a read-only project')
        done = command('readonly-output-refusal', [args.cli.resolve(), '--json', 'export', 'compile-commands',
                       '--project', consumer, '--output', work / 'denied.json'], consumer)
        reply = json.loads(done.stdout)
        if done.returncode != 1 or reply.get('ok') is not False or reply['data'].get('code') != 'output_error' or (work / 'denied.json').exists():
            raise ValueError('read-only output refusal did not fail closed')
        allowed = {str(args.cli.resolve()), str(args.test.resolve()), *[str(Path(local[r])) for r in ('cc', 'cxx', 'linker', 'ar')]}
        executed = set()
        for trace in output.glob('*.execve'):
            if trace.stem.startswith('visibility-'):
                continue  # Explicit host visibility controls, separate from native product execution.
            for encoded in re.findall(r'execve\("([^"\n]+)"', trace.read_text()):
                path = bytes.fromhex(encoded.replace('\\x', '')).decode('utf-8')
                executed.add(path)
                if path not in allowed and not Path(path).is_relative_to(consumer / 'out/native'):
                    raise ValueError('unexpected native executed image: ' + path)
        passed = True
    except Exception as exception:
        error = str(exception)
        test, executed = locals().get('test', {}), locals().get('executed', set())
    result = {'passed': passed, 'error': error, 'test_checks': test.get('checks'), 'commands': commands,
              'cli_sha256': digest(args.cli), 'test_sha256': digest(args.test),
              'driver_sha256': digest(__file__), 'executed': sorted(executed),
              'object_working_directories': locals().get('directories', []),
              'isolation': 'network denied, read-only root/SDK/project query, hidden producer/CLI/extension sources, empty PATH'}
    (output / 'result.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'passed': passed, 'error': error, 'test_checks': test.get('checks')}))
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
