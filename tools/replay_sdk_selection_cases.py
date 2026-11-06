"""Shared installed-store collision cases; host adapters supply isolated CLI calls.

Synthetic second-SDK metadata exercises resolution without claiming a producer
release. The original installed payload and installation pins are never edited.
"""
import hashlib
import json
from pathlib import Path
import shutil


def compact(value):
    return json.dumps(value, sort_keys=True, ensure_ascii=False, separators=(',', ':')).encode()


def selection_cases(*, call, record, store, sdk_dir, scratch, tools, allow_dirty_sdk=False):
    project = scratch / 'project'
    call('create the selection project', ['new', str(project)])
    base = ['--project', str(project)]
    configure = ['configure', *base]
    for role in ('cc', 'cxx', 'linker', 'ar'):
        configure += ['--' + role, str(tools[role])]
    if allow_dirty_sdk:
        configure += ['--allow-dirty-sdk', 'true']
    explicit = [*configure, '--sdk', sdk_dir]
    call('select the installed SDK explicitly', explicit)
    binding_paths = [project / 'crt.lock', project / '.crt/local.json']

    def binding():
        return [path.read_bytes() for path in binding_paths]

    original_binding = binding()

    def refused(label, args, message=None):
        before = binding()
        call(label, args, expect_ok=False, expect_text=message)
        record(label + ': both binding files preserved', binding() == before)

    receipt_path = store / '.receipts' / (sdk_dir + '.json')
    original_receipt = receipt_path.read_bytes()
    contract_path = store / '.contracts' / (sdk_dir + '.json')
    original_contract = contract_path.read_bytes()
    receipt = json.loads(original_receipt)
    content = receipt['sdk']['content_sha256']
    for length in (8, 12, 13, 32, 64):
        call(f'identity prefix of {length} digits selects the SDK', [*configure, '--sdk', content[:length]])
        record(f'prefix {length} retains the exact binding', binding() == original_binding)
    for length in (13, 32, 64):
        wrong = content[:length - 1] + ('0' if content[length - 1] != '0' else '1')
        refused(f'reject a mismatching {length}-digit identity', [*configure, '--sdk', wrong], 'no installed SDK matches')
    refused('reject a too-short identity', [*configure, '--sdk', content[:7]], 'no installed SDK matches')

    # A genuinely different content identity of the same version fits the
    # manifest too. This is a private resolver fixture, not an installed release.
    provider = json.loads(original_contract)
    variant_bytes = b'private selection collision fixture\n'
    inventory = provider['sdk_inventory'] + [['_selection_variant', 'file', hashlib.sha256(variant_bytes).hexdigest()]]
    # CRT inventories order filesystem path components, not generic strings
    # (a directory named support sorts before its sibling support.h).
    inventory.sort(key=lambda entry: Path(entry[0]).parts)
    variant_hash = hashlib.sha256(compact(inventory)).hexdigest()
    variant_dir = receipt['sdk']['version'] + '-' + variant_hash[:12]
    variant = store / variant_dir
    shutil.copytree(store / sdk_dir, variant, symlinks=True)
    (variant / '_selection_variant').write_bytes(variant_bytes)
    provider['sdk_inventory'] = inventory
    provider['sdk_content_sha256'] = variant_hash
    variant_contract = compact(provider)
    variant_receipt = json.loads(original_receipt)
    variant_receipt['sdk'].update(content_sha256=variant_hash, directory=variant_dir)
    variant_receipt['contract'] = {'sha256': hashlib.sha256(variant_contract).hexdigest()}
    (store / '.contracts' / (variant_dir + '.json')).write_bytes(variant_contract)
    (store / '.receipts' / (variant_dir + '.json')).write_bytes(compact(variant_receipt))
    try:
        call('the competing synthetic SDK verifies', ['sdk', 'verify', variant_dir])
        refused('automatic selection refuses two fitting SDKs', configure, 'several installed SDKs fit')
        call('explicit original selection works with a competing SDK', explicit)
        record('explicit original choice preserves its exact pins', binding() == original_binding)
        call('the existing project builds without re-resolving ambiguity', ['build', *base])
        record('a locked build keeps the original SDK', binding() == original_binding)
        call('explicit second identity selects the competing SDK', [*configure, '--sdk', variant_hash])
        selected = json.loads(binding()[0])['selection']
        record('the second SDK has independent content and contract pins', selected['sdk_content_sha256'] == variant_hash and
               selected['contract_sha256'] == variant_receipt['contract']['sha256'])
        call('restore the explicit original selection', explicit)
    finally:
        shutil.rmtree(variant)
        (store / '.contracts' / (variant_dir + '.json')).unlink()
        (store / '.receipts' / (variant_dir + '.json')).unlink()

    # Neither absent declarations nor non-native scripts may trigger PATH lookup.
    for role in ('cc', 'cxx', 'linker', 'ar'):
        index = configure.index('--' + role)
        refused('missing declared ' + role, [*configure[:index], *configure[index + 2:], '--sdk', sdk_dir], 'missing --' + role)
        bad = list(explicit); bad[index + 1] = str(scratch / 'absent tool')
        refused('nonexistent declared ' + role, bad)
    script = scratch / 'fake native compiler'
    script.write_text('#!/bin/sh\nexit 97\n')
    script.chmod(0o755)
    bad = list(explicit); bad[bad.index('--cxx') + 1] = str(script)
    refused('a script cannot substitute for the compiler', bad, 'non-native tool')

    # A selected compiler whose bytes change is refused before any invocation.
    local_path = project / '.crt/local.json'
    local = json.loads(local_path.read_bytes())
    alias = scratch / 'changed compiler'
    shutil.copy2(tools['cc'], alias)
    with alias.open('ab') as output:
        output.write(b'changed tool bytes')
    local['cc'] = str(alias)
    local_path.write_bytes(compact(local))
    refused('locked tool substitution fails before execution', ['build', *base], 'locked external tool digest mismatch')
    local_path.write_bytes(original_binding[1])
    call('final build with explicit tools', ['build', *base])
    call('final test with explicit tools', ['test', *base])
    call('final run with explicit tools', ['run', *base])
    shared = scratch / 'shared-project'
    call('create a shared-runtime collision consumer', ['new', str(shared), '--runtime-linkage', 'shared'])
    shared_configure = list(explicit); shared_configure[shared_configure.index('--project') + 1] = str(shared)
    call('configure the shared-runtime collision consumer', shared_configure)
    call('build the shared-runtime collision consumer', ['build', '--project', str(shared)])
    call('test the shared-runtime collision consumer', ['test', '--project', str(shared)])
    call('run the shared-runtime collision consumer', ['run', '--project', str(shared)])
    record('all selection failures preserve the final original binding', binding() == original_binding)
    record('original installation pins are unchanged', receipt_path.read_bytes() == original_receipt and contract_path.read_bytes() == original_contract)
