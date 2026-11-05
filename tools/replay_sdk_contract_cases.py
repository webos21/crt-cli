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


def repair_payload_cases(*, call, results, store, sdk_dir, repair):
    """Repair must never turn into SDK payload replacement, even with trusted release inputs."""
    version = store / sdk_dir / 'VERSION'
    receipt = store / '.receipts' / (sdk_dir + '.json')
    contract = store / '.contracts' / (sdk_dir + '.json')
    original = version.read_bytes(); before = (receipt.read_bytes(), contract.read_bytes())
    damaged = original.strip() + b'-damaged\n'
    try:
        version.write_bytes(damaged)
        call('repair refuses a changed installed SDK payload', repair, expect_ok=False, expect_text='immutable')
        passed = version.read_bytes() == damaged and (receipt.read_bytes(), contract.read_bytes()) == before
        results.append({'case': 'refused repair preserves payload and metadata', 'passed': passed, 'detail': ''})
        print(('PASS ' if passed else 'FAIL ') + 'refused repair preserves payload and metadata', flush=True)
    finally:
        version.write_bytes(original)
    call('verify after restoring the refused repair payload', ['sdk', 'verify', sdk_dir])


def contract_cases(*, call, results, store, sdk_dir, contract, archive, manifest_path, inputs, work, scratch, fresh_store,
                   digest, tree, published, tools, os_kind, allow_dirty_sdk=False):
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
    if allow_dirty_sdk:
        configure += ['--allow-dirty-sdk', 'true']
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
    receipt = store / '.receipts' / (sdk_dir + '.json')
    original_receipt = receipt.read_bytes()
    record('receipt retains the release contract pin', json.loads(original_receipt)['contract'] == {'sha256': contract['sha256']})
    unchanged_lock = (project / 'crt.lock').read_bytes()
    unchanged_local = (project / '.crt/local.json').read_bytes()
    valid_change = json.loads(original); valid_change['changed_policy'] = 'still a valid portable contract'
    stored.write_text(json.dumps(valid_change))
    call('verify rejects a valid-looking policy edit', ['sdk', 'verify', sdk_dir], expect_ok=False, expect_text='digest mismatch')
    call('fresh configure cannot re-pin changed bytes', configure, expect_ok=False, expect_text='digest mismatch', tools=True)
    record('failed fresh configure preserves the binding', (project / 'crt.lock').read_bytes() == unchanged_lock and
           (project / '.crt/local.json').read_bytes() == unchanged_local)
    repair = ['sdk', 'repair', sdk_dir, '--archive', str(archive), '--release-manifest', str(manifest_path),
              '--release-manifest-sha256', digest(manifest_path)]
    call('repair refuses raw archive trust alone', ['sdk', 'repair', sdk_dir, '--archive', str(archive), '--archive-sha256', digest(archive)],
         expect_ok=False, expect_text='requires --release-manifest')
    call('repair restores the original policy', repair)
    record('repair preserves the original receipt and contract', receipt.read_bytes() == original_receipt and stored.read_bytes() == original)
    call('verify after explicit policy repair', ['sdk', 'verify', sdk_dir])
    repair_payload_cases(call=call, results=results, store=store, sdk_dir=sdk_dir, repair=repair)
    receipt.unlink()
    call('verify refuses a legacy store without a receipt', ['sdk', 'verify', sdk_dir], expect_ok=False, expect_text='receipt')
    call('configure refuses a legacy store without a receipt', configure, expect_ok=False, expect_text='receipt', tools=True)
    call('ordinary reinstall does not migrate a legacy store', ['sdk', 'install', *repair[3:]], expect_ok=False, expect_text='receipt')
    call('explicit release repair migrates a legacy store', repair)
    stored.unlink()
    call('receipt detects loss of a required contract', ['sdk', 'verify', sdk_dir], expect_ok=False, expect_text='no stored toolchain contract')
    call('explicit repair restores a required contract', repair)
    record('repaired SDK payload identity is unchanged', json.loads(receipt.read_text())['sdk']['content_sha256'] ==
           json.loads(original)['sdk_content_sha256'])
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
