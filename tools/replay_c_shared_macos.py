#!/usr/bin/env python3
"""Native SDK-only C provider replay. Python/otool are test-host tools, not product dependencies.

Install an independently pinned release asset set, then use the native CLI's
unchanged SDK policy in Seatbelt. Retain link commands/maps, actual PAL image
enumeration, negative controls and immutable SDK inventories. An explicit
same-install-name provider is a relink control, not a separately built port.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys

from replay_macos_sandbox import MacSandbox
from replay_port_macos import deny_inputs, inventory


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def image_paths(text):
    paths = [line[6:] for line in text.splitlines() if line.startswith('IMAGE:')]
    if any(not path or not Path(path).is_absolute() for path in paths):
        raise ValueError('image enumeration contains a non-absolute path')
    return sorted({str(Path(path).resolve()) for path in paths})


def link_command(receipt, executable):
    found = [list(command) for command in receipt.get('commands', [])
             if '-c' not in command and '-o' in command and command[command.index('-o') + 1] == str(executable)]
    if len(found) != 1:
        raise ValueError('expected exactly one declared executable link command')
    return found[0]


def replace_provider(command, selected, replacement):
    aliases = {str(selected), str(selected.resolve())}
    found = [index for index, value in enumerate(command) if value in aliases]
    if len(found) != 1:
        raise ValueError('explicit-provider control needs exactly one selected SDK zlib input')
    command = list(command)
    command[found[0]] = str(replacement)
    return command


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('cli', 'release-dir', 'c-test-source', 'c-dso-source', 'output', 'work', 'cc', 'cxx', 'linker', 'ar', 'os-sdk'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--manifest-name', required=True)
    parser.add_argument('--manifest-sha256', required=True)
    parser.add_argument('--hide', type=Path, action='append', required=True)
    args = parser.parse_args()
    if sys.platform != 'darwin' or platform.machine().lower() not in ('arm64', 'aarch64'):
        parser.error('requires native macOS/arm64')
    output, work = args.output.resolve(), args.work.resolve()
    if output.exists() or work.exists():
        parser.error('output and work must be fresh; preserve earlier evidence')
    original_manifest = args.release_dir / args.manifest_name
    if digest(original_manifest) != args.manifest_sha256:
        parser.error('release manifest differs from the independent pin')
    manifest = json.loads(original_manifest.read_bytes())
    archive = next(a for a in manifest['assets'] if 'install' in a)
    if manifest.get('target') != {'os': 'macos', 'arch': 'aarch64'}:
        parser.error('manifest does not identify a native macOS/arm64 SDK')
    contract_sha256 = next(a['sha256'] for a in manifest['assets'] if a['name'] == archive['contract'])
    content = archive['install']['content_sha256']
    output.mkdir(parents=True)
    work.mkdir(parents=True)
    scratch = work / 'scratch'; scratch.mkdir()
    home = scratch / 'home'; home.mkdir()
    inputs = work / 'inputs'
    shutil.copytree(args.release_dir, inputs, symlinks=True)
    cli = work / 'crt-cli'; shutil.copy2(args.cli, cli)
    tools = {name: getattr(args, name).absolute() for name in ('cc', 'cxx', 'linker', 'ar')}
    hidden = [p.resolve() for p in [*args.hide, args.release_dir, args.cli]]
    if any(work.is_relative_to(p) or p.is_relative_to(work) for p in hidden):
        parser.error('hidden inputs and replay work must not overlap')
    sandbox = MacSandbox()
    sandbox.prepare(work, scratch, cli, tools, hidden)
    sandbox.temp.rmdir()
    sandbox.temp = scratch / 'tmp'; sandbox.temp.mkdir()
    results, commands, images = [], [], []
    sdk = home / '.crt/sdks' / (archive['install']['version'] + '-' + content[:12])
    before = None

    def save(name, document):
        (output / name).write_text(json.dumps(document, ensure_ascii=False, indent=2) + '\n')

    def record(case, passed, detail=''):
        results.append({'case': case, 'passed': bool(passed), 'detail': str(detail)[:2000]})
        print(('PASS ' if passed else 'FAIL ') + case, flush=True)

    def run(label, argv, *, boxed=True, hidden_extra=()):
        command = list(map(str, argv))
        options = {'cwd': scratch, 'env': {'PATH': '/nonexistent', 'HOME': str(home), 'LC_ALL': 'C'}, 'stdin': subprocess.DEVNULL}
        if boxed:
            base = sandbox.command([], home)
            policy = Path(base[2])
            if hidden_extra:
                policy.write_text(deny_inputs(policy.read_text(), hidden_extra))
            command = base[:3] + command
            options = sandbox.process_options(home)
        index = len(commands)
        done = subprocess.run(command, capture_output=True, text=True, timeout=1800, **options)
        for suffix, text in (('stdout', done.stdout), ('stderr', done.stderr)):
            (output / f'call-{index:03}.{suffix}').write_text(text)
        commands.append({'case': label, 'argv': command, 'exit': done.returncode, 'boxed': boxed})
        save('commands.json', commands)
        return done

    def cli_json(label, tail, *, failure=None):
        done = run(label, [cli, '--json', *tail])
        try:
            reply = json.loads(done.stdout)
        except ValueError:
            reply = {}
        passed = done.returncode == 0 and reply.get('ok') is True
        if failure:
            passed = done.returncode > 0 and reply.get('ok') is False and failure in json.dumps(reply.get('data', {}))
        record(label, passed, done.stderr[-1000:] or reply.get('data', {}))
        if not passed:
            raise ValueError(label + ': unexpected CLI outcome')
        return reply.get('data', {})

    def dependencies(label, binary):
        done = run(label + ': Mach-O dependencies', ['/usr/bin/otool', '-L', binary], boxed=False)
        if done.returncode:
            raise ValueError(done.stderr)
        return [line.strip().split(' (compatibility', 1)[0] for line in done.stdout.splitlines()[1:] if line.strip()]

    def loaded(label, executable, shared, zlib, project_library=None):
        done = run(label + ': execute', [executable])
        paths = image_paths(done.stdout)
        actual_c = [p for p in paths if Path(p).name == 'libc.dylib']
        actual_z = [p for p in paths if Path(p).name.startswith('libz.')]
        expected_c = [str((sdk / 'lib/libc.dylib').resolve())] if shared else []
        expected_z = [str(zlib.resolve())] if zlib else []
        record(label + ': constructors, allocation, errno and zlib round trip',
               done.returncode == 0 and 'sdk_c_provider: ok' in done.stdout, done.stderr)
        record(label + ': actual libc and libz are the selected providers',
               actual_c == expected_c and actual_z == expected_z, {'libc': actual_c, 'libz': actual_z})
        if project_library:
            record(label + ': exactly one project dylib is mapped',
                   [p for p in paths if Path(p).name == project_library.name] == [str(project_library.resolve())])
        if shared:
            absent = run(label + ': SDK libc denied', [executable], hidden_extra=[sdk / 'lib/libc.dylib'])
            record(label + ': no substitute runtime when SDK libc is denied',
                   absent.returncode != 0 and 'sdk_c_provider: ok' not in absent.stdout and 'libc.dylib' in absent.stderr,
                   absent.stderr[-1500:])
        images.append(str(executable))
        return paths

    def project(name, targets, sources):
        root = scratch / ('projects space 한글') / name
        cli_json('new ' + name, ['new', root, '--language', 'c'])
        document = {'manifest_format': 1, 'package': {'name': 'cshared', 'version': '0.1.0'},
                    'sdk': {'profile': 'dev', 'version': '*', 'target': 'native', 'storage': 'shared-immutable'},
                    'include_dirs': ['include'], 'targets': targets}
        (root / 'crt.json').write_text(json.dumps(document, indent=2) + '\n')
        for folder in ('src/main', 'src/test'):
            for stale in (root / folder).iterdir():
                stale.unlink()
        for name, text in sources.items():
            (root / name).write_text(text)
        return root

    tool_flags = [value for key, path in tools.items() for value in ('--' + key, str(path))]
    tool_flags += ['--os-sdk', str(args.os_sdk.resolve())]
    source = args.c_test_source.read_text()
    dso_source = args.c_dso_source.read_text()
    save('inputs.json', {'cli_sha256': digest(cli), 'manifest_sha256': args.manifest_sha256,
                        'sdk_content_sha256': content, 'producer': manifest['source_commit'], 'contract_sha256': contract_sha256,
                        'sources': {str(p): digest(p) for p in (args.c_test_source, args.c_dso_source)},
                        'tools': {key: {'path': str(path), 'sha256': digest(path)} for key, path in tools.items()},
                        'driver_sha256': digest(Path(__file__)), 'os_sdk': str(args.os_sdk.resolve()), 'hidden': list(map(str, hidden))})
    try:
        cli_json('install SDK from pinned manifest', ['sdk', 'install', '--archive', inputs / archive['name'],
                 '--release-manifest', inputs / args.manifest_name, '--release-manifest-sha256', args.manifest_sha256])
        cli_json('verify installed SDK before replay', ['sdk', 'verify', sdk.name])
        before = inventory(sdk)
        profile = json.loads((sdk / 'share/crt/contracts/development.json').read_bytes())
        record('SDK producer and compile provenance are clean', profile['producer_working_tree_dirty'] is False and
               profile['compile_provenance']['working_tree_dirty'] is False)
        shared_receipt = None
        for name, shared, shared_z, dso in (('static-sdk-static', False, False, False),
                ('shared-sdk-static', True, False, False), ('shared-sdk-shared', True, True, False),
                ('shared-project-dso', True, True, True)):
            target = {'name': 'app', 'kind': 'executable', 'runtime_linkage': 'shared' if shared else 'static',
                      'sdk_library_linkage': 'shared' if shared_z else 'static', 'sdk_libraries': ['z'], 'sources': ['src/main/main.c']}
            targets = []
            sources = {'src/main/main.c': ('#define CRT_TEST_PROJECT_DSO 1\n' if dso else '') + source}
            if dso:
                targets.append({'name': 'crt_provider', 'kind': 'shared-library', 'sources': ['src/main/provider.c']})
                target['dependencies'] = ['crt_provider']
                sources['src/main/provider.c'] = dso_source
            root = project(name, [*targets, target], sources)
            receipt = cli_json(name + ': cold build', ['build', '--project', root, '--sdk', content, *tool_flags])
            bound = (root / 'crt.lock').read_bytes()
            lock = json.loads(bound)
            record(name + ': exact clean SDK selection', lock['selection']['sdk_content_sha256'] == content and
                   lock['selection']['allow_dirty_sdk'] is False)
            warm = cli_json(name + ': warm build', ['build', '--project', root])
            record(name + ': cold executes and warm reuses all commands', receipt['cache']['executed'] > 0 and
                   receipt['cache']['reused'] == 0 and warm['cache']['executed'] == 0)
            executable = root / 'out/native/app'
            deps = dependencies(name, executable)
            record(name + ': direct CRT dependency matches linkage', shared == ('@rpath/libc.dylib' in deps), deps)
            if dso:
                library = root / 'out/native/libcrt_provider.dylib'
                record(name + ': project dylib uses the same CRT', '@rpath/libc.dylib' in dependencies(name + ' dylib', library))
                images.append(str(library))
            command = link_command(receipt, executable)
            mapped = executable.with_name('app.mapped')
            command[command.index('-o') + 1] = str(mapped)
            map_path = scratch / (name + '.map')
            mapped_result = run(name + ': unchanged policy relink with map', [*command, '-Wl,-map,' + str(map_path)])
            record(name + ': link map captured', mapped_result.returncode == 0 and map_path.is_file(), mapped_result.stderr)
            map_text = map_path.read_text()
            shutil.copy2(map_path, output / map_path.name)
            wanted = sdk / ('lib/libz.dylib' if shared_z else 'lib/libz.a')
            if not shared_z:
                record(name + ': map selects SDK static zlib', str(wanted) in map_text)
            record(name + ': no static CRT in a shared executable' if shared else name + ': map selects SDK static CRT',
                   all(str(sdk / 'lib' / lib) not in map_text for lib in ('libc.a', 'libm.a', 'libdl.a')) if shared
                   else str(sdk / 'lib/libc.a') in map_text)
            loaded(name, executable, shared, wanted if shared_z else None, library if dso else None)
            cli_json(name + ': CLI run', ['run', '--project', root])
            record(name + ': warm build/run preserve lock', bound == (root / 'crt.lock').read_bytes())
            if name == 'shared-sdk-shared':
                shared_receipt = (receipt, executable)

        receipt, executable = shared_receipt
        provider = scratch / 'explicit provider/lib/libz.1.dylib'
        provider.parent.mkdir(parents=True)
        shutil.copy2((sdk / 'lib/libz.dylib').resolve(), provider)
        command = replace_provider(link_command(receipt, executable), sdk / 'lib/libz.dylib', provider)
        explicit_exe = executable.with_name('app.explicit')
        command[command.index('-o') + 1] = str(explicit_exe)
        command.insert(1, '-Wl,-rpath,' + str(provider.parent))
        explicit_map = scratch / 'explicit-provider.map'
        done = run('explicit same-install-name provider relink control', [*command, '-Wl,-map,' + str(explicit_map)])
        record('explicit same-install-name control links', done.returncode == 0, done.stderr)
        if done.returncode == 0:
            shutil.copy2(explicit_map, output / explicit_map.name)
            record('explicit control map selects the provided dylib', str(provider) in explicit_map.read_text())
            loaded('shared-explicit-provider', explicit_exe, True, provider)

        for name, targets in (
            ('mixed-project', [{'name': 'provider', 'kind': 'shared-library', 'sources': ['src/main/provider.c']},
                {'name': 'app', 'sources': ['src/main/main.c'], 'dependencies': ['provider']}]),
            ('mixed-sdk', [{'name': 'app', 'sources': ['src/main/main.c'], 'sdk_libraries': ['z'], 'sdk_library_linkage': 'shared'}]),
            ('mixed-transitive', [{'name': 'provider', 'kind': 'static-library', 'sources': ['src/main/provider.c'],
                                  'sdk_libraries': ['z'], 'sdk_library_linkage': 'shared'},
                {'name': 'app', 'sources': ['src/main/main.c'], 'dependencies': ['provider']}]),
        ):
            root = project(name, targets, {'src/main/main.c': source, 'src/main/provider.c': dso_source})
            initial = (root / 'crt.lock').read_bytes()
            cli_json(name + ': reject before build', ['build', '--project', root, '--sdk', content, *tool_flags], failure='shared')
            record(name + ': no output and untouched unresolved lock', not (root / 'out').exists() and
                   not (root / '.crt').exists() and (root / 'crt.lock').read_bytes() == initial)
        cli_json('verify installed SDK after replay', ['sdk', 'verify', sdk.name])
        sandbox.verify(record)
    except Exception as error:
        record('replay completed without a harness exception', False, repr(error))
    finally:
        record('installed SDK inventory unchanged', before is not None and inventory(sdk) == before)
        record('original pinned release manifest unchanged', digest(original_manifest) == args.manifest_sha256)
        save('result.json', {'passed': bool(results) and all(r['passed'] for r in results), 'cases': results,
                            'target': manifest['target'], 'contract_sha256': contract_sha256,
                            'manifest_sha256': args.manifest_sha256, 'sdk_content_sha256': content, 'cli_sha256': digest(cli),
                            'sdk_directory': str(sdk), 'images': images,
                            'isolation': sandbox.isolation, 'work': str(work)})
    print(f"{sum(r['passed'] for r in results)}/{len(results)} replay gates passed")
    return 0 if all(r['passed'] for r in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
