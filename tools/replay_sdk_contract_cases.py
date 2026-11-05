"""Host-independent replay cases for the toolchain contract a release ships (D2 Stage 2) and for project SDK selection.

Shared by replay_sdk_install_linux.py, replay_sdk_install_macos.py and replay_sdk_install_windows.py so that all hosts are
held to the same cases. Each script supplies `call(label, tail, **kw)`, which runs one CLI invocation under that host's
isolation (bubblewrap, Seatbelt, restricted token) and records a pass/fail result. The keywords are `expect_ok`,
`expect_text`, `check`, `binds` (directories the sandbox must be able to read) and `tools=True` for calls that start the
selected compiler/linker/archiver or a program built by the project.

The SDK store must be `$HOME/.crt/sdks` of the sandbox, because `configure` finds an installed SDK there.
"""
import json
import shutil
from pathlib import Path


def contract_cases(*, call, results, store, sdk_dir, contract, archive, manifest_path, inputs, work, scratch, fresh_store,
                   digest, tree, published, tools, os_kind):
    """`contract` is the release manifest's entry for the contract asset (name, sha256); `os_kind` is the OS SDK kind the
    contract is expected to declare on this host (None on Linux, which needs none)."""

    def record(case, passed, detail=''):
        results.append({'case': case, 'passed': bool(passed), 'detail': str(detail)[:400]})
        print(('PASS ' if passed else 'FAIL ') + case, flush=True)

    # 1. The installation kept the contract, byte for byte, beside the SDK.
    stored = store / '.contracts' / (sdk_dir + '.json')
    record('installation stored the release toolchain contract', stored.is_file() and digest(stored) == contract['sha256'], stored)
    call('list reports the stored contract', ['sdk', 'list', '--store', str(store)],
         check=lambda r: [e['has_contract'] for e in r['data']['sdks']] == [True])

    # 2. A project selects the only installed SDK; neither --sdk nor --sdk-contract is given.
    project = scratch / 'project'
    base = ['--project', str(project)]
    call('new project', ['new', str(project)])
    configure = ['configure', *base, '--cc', str(tools['cc']), '--cxx', str(tools['cxx']),
                 '--linker', str(tools['linker']), '--ar', str(tools['ar'])]
    call('configure selects the only installed SDK and its stored contract', configure, tools=True)
    try:
        lock_text = (project / 'crt.lock').read_text(encoding='utf-8')
        lock, local = json.loads(lock_text), json.loads((project / '.crt' / 'local.json').read_text(encoding='utf-8'))
        selection = lock['selection']
        normalized = lock_text.replace('\\\\', '/')
        selected = (lock['lock_format'] == 1 and selection['source'] == 'store' and selection['sdk_directory'] == sdk_dir and
                    selection['contract_source'] == 'store' and selection['contract_sha256'] == contract['sha256'])
        record('the lock records the SDK identity and contract digest', selected, json.dumps(selection)[:300])
        record('no SDK or contract path is kept in the project', 'sdk_root' not in local and 'sdk_contract' not in local, sorted(local))
        record('the lock names no path of this machine', scratch.as_posix() not in normalized, '')
        record('the OS SDK kind is locked, its location is local state',
               selection.get('os_sdk', {}).get('kind') == os_kind and
               (('os_sdk' not in local) if os_kind is None else (bool(local.get('os_sdk')) and local['os_sdk'].replace('\\', '/') not in normalized)),
               f"kind={selection.get('os_sdk')} local={local.get('os_sdk')}")
    except Exception as error:
        record('the project lock could be read', False, error)

    # 3. The selected toolchain really builds and runs.
    call('build with the stored contract', ['build', *base], tools=True)
    call('test with the stored contract', ['test', *base], tools=True)
    call('run with the stored contract', ['run', *base], tools=True)

    # 4. Damage to the stored contract is caught by the lock and never repaired silently.
    original = stored.read_bytes()
    stored.write_bytes(original + b'\n')
    call('a modified stored contract is refused', ['build', *base], expect_ok=False, expect_text='toolchain contract digest mismatch', tools=True)
    stored.write_bytes(original)
    call('build works again once the contract is restored', ['build', *base], tools=True)
    stored.unlink()
    call('a missing stored contract is reported', ['build', *base], expect_ok=False, expect_text='no stored toolchain contract', tools=True)
    stored.write_bytes(original)

    # 5. The OS SDK is this host's: refused when the contract needs none, refused when it is not a directory.
    if os_kind is None:
        call('--os-sdk is refused when the contract needs none', [*configure, '--os-sdk', str(scratch)],
             expect_ok=False, expect_text='needs no OS SDK', tools=True)
    else:
        call('a missing --os-sdk directory is refused', [*configure, '--os-sdk', str(scratch / 'no-such-os-sdk')],
             expect_ok=False, expect_text='absolute directory', tools=True)

    # 6. The contract asset is verified before anything is extracted or published.
    contract_name = contract['name']

    def release_copy(name, mutate=None):
        directory = work / name
        shutil.rmtree(directory, ignore_errors=True)
        directory.mkdir()
        shutil.copy2(manifest_path, directory / manifest_path.name)
        shutil.copy2(archive, directory / archive.name)
        if mutate is not None:
            shutil.copy2(inputs / contract_name, directory / contract_name)
            mutate(directory / contract_name)
        return directory

    def flip(path):
        data = bytearray(path.read_bytes())
        data[len(data) // 2] ^= 1
        path.write_bytes(bytes(data))

    for slug, label, mutate, text in (('missing', 'a release without its contract asset', None, 'toolchain contract not found beside the archive'),
                                      ('flipped', 'a contract asset with a flipped byte', flip, 'toolchain contract SHA-256 mismatch'),
                                      ('grown', 'a contract asset with a trailing byte', lambda p: p.open('ab').write(b'x'), 'size differs')):
        directory = release_copy('contract-' + slug, mutate)
        target = fresh_store('store-contract-' + slug)
        call(label + ' is refused',
             ['sdk', 'install', '--archive', str(directory / archive.name), '--release-manifest', str(directory / manifest_path.name),
              '--release-manifest-sha256', digest(manifest_path), '--store', str(target)],
             expect_ok=False, expect_text=text, binds=(directory,), store=target)
        record(label + ': nothing published', published(target) == [] and not tree(target / '.staging') and not tree(target / '.contracts'), tree(target))
