"""First-build contract cases shared by native host-test isolation adapters."""
import hashlib
import json
from pathlib import Path
import shutil

from replay_sdk_selection_cases import compact


def first_build_cases(*, call, record, store, sdk_dir, scratch, tools, allow_dirty_sdk=False, publication_failure=None):
    declarations = []
    for role in ('cc', 'cxx', 'linker', 'ar'):
        declarations += ['--' + role, str(tools[role])]
    if allow_dirty_sdk:
        declarations += ['--allow-dirty-sdk', 'true']
    receipt_path = store / '.receipts' / (sdk_dir + '.json')
    contract_path = store / '.contracts' / (sdk_dir + '.json')
    receipt_bytes, contract_bytes = receipt_path.read_bytes(), contract_path.read_bytes()
    receipt = json.loads(receipt_bytes)

    def new(name, **kwargs):
        root = scratch / name
        call('new ' + name, ['new', str(root), *kwargs.get('options', [])])
        record(name + ': new is source-only', not (root / '.crt').exists() and
               json.loads((root / 'crt.lock').read_bytes()) == {'lock_format': 1, 'state': 'unresolved'})
        return root

    def binding(root):
        return [(root / name).read_bytes() if (root / name).is_file() else None for name in ('crt.lock', '.crt/local.json')]

    def build(root, extra=()):
        return ['build', '--project', str(root), *declarations, *extra]

    def refused(label, root, argv, message=None):
        before = binding(root)
        call(label, argv, expect_ok=False, expect_text=message)
        record(label + ': both binding files preserved', binding(root) == before)

    missing = new('missing-tools')
    refused('missing LLVM declarations are diagnosed', missing, ['build', '--project', str(missing)], 'first build requires explicit --cc')
    for role in ('cc', 'cxx', 'linker', 'ar'):
        argv = build(missing); index = argv.index('--' + role); argv[index + 1] = str(scratch / 'absent-' / role)
        refused('missing native ' + role, missing, argv)
    relative = build(missing); relative[relative.index('--cc') + 1] = 'clang'
    refused('relative tool names never search PATH', missing, relative, 'absolute path')
    malformed = new('invalid-binding')
    for label, data in (
        ('legacy lock', b'{"lock_format":0,"state":"unresolved"}'),
        ('fractional lock format', b'{"lock_format":1.5,"state":"unresolved"}'),
        ('malformed lock', b'{broken'),
        ('duplicate lock keys', b'{"lock_format":0,"lock_format":1,"state":"unresolved"}'),
        ('unresolved lock with a selection', b'{"lock_format":1,"state":"unresolved","selection":{}}')):
        (malformed / 'crt.lock').write_bytes(data)
        refused(label + ' is never rebound implicitly', malformed, build(malformed))
    (malformed / 'crt.lock').write_bytes(b'{"lock_format":1,"state":"unresolved"}')
    (malformed / '.crt').mkdir()
    (malformed / '.crt/local.json').write_bytes(b'{}')
    refused('partial binding publication requires explicit configure', malformed, ['build', '--project', str(malformed)], 'unsupported/unresolved project lock')

    # An empty default store must not trigger a PATH/toolchain/SDK fallback.
    away = store.parent / 'sdks-away'
    store.rename(away)
    try:
        refused('no compatible installed SDK is diagnosed', missing, build(missing), 'no installed SDK for this host')
    finally:
        shutil.rmtree(store, ignore_errors=True)
        away.rename(store)

    # Give automatic selection two genuinely different same-version contents.
    provider = json.loads(contract_bytes)
    marker = b'private first-build resolver fixture\n'
    items = provider['sdk_inventory'] + [['_first_build_variant', 'file', hashlib.sha256(marker).hexdigest()]]
    items.sort(key=lambda row: Path(row[0]).parts)
    variant_hash = hashlib.sha256(compact(items)).hexdigest()
    variant_dir = receipt['sdk']['version'] + '-' + variant_hash[:12]
    variant = store / variant_dir
    shutil.copytree(store / sdk_dir, variant, symlinks=True)
    (variant / '_first_build_variant').write_bytes(marker)
    provider.update(sdk_inventory=items, sdk_content_sha256=variant_hash)
    data = compact(provider)
    other_receipt = json.loads(receipt_bytes)
    other_receipt['sdk'].update(content_sha256=variant_hash, directory=variant_dir)
    other_receipt['contract'] = {'sha256': hashlib.sha256(data).hexdigest()}
    (store / '.contracts' / (variant_dir + '.json')).write_bytes(data)
    (store / '.receipts' / (variant_dir + '.json')).write_bytes(compact(other_receipt))
    first = new('hello')
    try:
        refused('ambiguous first-build SDK selection is refused', first, build(first), 'several installed SDKs fit')
        call('first build selects an explicit identity despite ambiguity', build(first, ['--sdk', sdk_dir]))
        original = binding(first)
        selected = json.loads(original[0])['selection']
        record('first build pins the SDK, contract and every LLVM role',
               selected['sdk_content_sha256'] == receipt['sdk']['content_sha256'] and selected['sdk_directory'] == sdk_dir and
               selected['contract_sha256'] == receipt['contract']['sha256'] and selected['source'] == selected['contract_source'] == 'store' and
               all(role in selected['tools'] for role in ('cc', 'cxx', 'linker', 'ar', 'resource_content_sha256')))
        local = json.loads(original[1])
        record('the first-build store lock keeps paths local', 'sdk_root' not in local and 'sdk_contract' not in local and
               str(scratch) not in original[0].decode())
        call('a bound build reuses its pin with two fitting SDKs', ['build', '--project', str(first)])
        call('test follows the first-build pin', ['test', '--project', str(first)])
        call('run follows the first-build pin', ['run', '--project', str(first)])
        refused('build refuses explicit replacement of a bound SDK', first, build(first, ['--sdk', variant_dir]), 'use configure explicitly')
        (first / '.sdk.json').write_bytes(compact({'format': 0, 'sdk': variant_dir, 'cc': '/wrong/compiler'}))
        call('bound build ignores changed configure defaults', ['build', '--project', str(first)])
        record('bound operations preserve exact pins', binding(first) == original)
        local_path = first / '.crt/local.json'; local_path.unlink()
        refused('a resolved lock without local state is not re-resolved', first, ['build', '--project', str(first)], 'no local SDK selection')
        refused('tool flags cannot recreate missing resolved local state', first, build(first), 'use configure explicitly')
        local_path.write_bytes(original[1])
    finally:
        shutil.rmtree(variant)
        (store / '.contracts' / (variant_dir + '.json')).unlink()
        (store / '.receipts' / (variant_dir + '.json')).unlink()

    failed = new('failed-source')
    source = failed / 'src/main/main.cpp'; saved = source.read_bytes()
    source.write_bytes(b'#error deliberate first-build failure\n')
    refused('compiler failure does not resolve the project', failed, build(failed))
    source.write_bytes(saved)
    call('first build succeeds after the source is repaired', build(failed))
    call('repaired project runs without configure', ['run', '--project', str(failed)])

    pending = new('pending-policy')
    changed = json.loads(contract_bytes); changed['changed_policy'] = True
    contract_path.write_bytes(compact(changed))
    try:
        refused('first build refuses changed stored policy bytes', pending, build(pending), 'digest mismatch')
    finally:
        contract_path.write_bytes(contract_bytes)
    record('restoring policy has not silently resolved the project', json.loads(binding(pending)[0])['state'] == 'unresolved')
    declared = {'format': 0, **{role: str(path) for role, path in tools.items()}}
    if allow_dirty_sdk:
        declared['allow_dirty_sdk'] = True
    (pending / '.sdk.json').write_bytes(compact(declared))
    call('plain first build reads tools from .sdk.json', ['build', '--project', str(pending)])
    call('configured-by-build project test', ['test', '--project', str(pending)])

    targeted = new('targeted-c', options=['--language', 'c'])
    refused('unknown first-build target leaves the binding unresolved', targeted, build(targeted, ['--target', 'absent']), 'unknown target')
    call('first build supports a declared target and static C runtime', build(targeted, ['--target', 'targeted-c']))
    call('static C first-build test', ['test', '--project', str(targeted)])
    call('static C first-build run', ['run', '--project', str(targeted)])
    shared = new('shared-cxx', options=['--runtime-linkage', 'shared'])
    call('first build with the shared C++ runtime policy', build(shared))
    call('shared C++ first-build test', ['test', '--project', str(shared)])
    call('shared C++ first-build run', ['run', '--project', str(shared)])

    if publication_failure:
        blocked = new('blocked-publication')
        original = binding(blocked)
        publication_failure(blocked, build(blocked))
        record('failed metadata publication preserves the unresolved binding', binding(blocked) == original)
        record('failed metadata publication removes only its own staging', not list((blocked / '.crt').glob('binding.tmp-*')))
        call('first build retries after publication is writable', build(blocked))
    record('all first-build cases preserve installation policy pins', receipt_path.read_bytes() == receipt_bytes and contract_path.read_bytes() == contract_bytes)
