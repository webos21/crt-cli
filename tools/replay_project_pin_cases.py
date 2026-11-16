"""Project-lock negatives shared by native public-Preview adapters.

The adapter supplies already built C/C++ projects and an installed public SDK.
These cases mutate only disposable project inputs, never release/store pins.
"""
import copy
import hashlib
import json


def project_pin_cases(*, call, record, project, language, content_sha256):
    lock_path = project / 'crt.lock'
    local_path = project / '.crt/local.json'
    original = lock_path.read_bytes()
    local = local_path.read_bytes()
    lock = json.loads(original)
    prefix = language + ': '
    outputs = {str(p.relative_to(project)): hashlib.sha256(p.read_bytes()).hexdigest()
               for p in (project / 'out').rglob('*') if p.is_file()}

    def unchanged_outputs():
        return outputs == {str(p.relative_to(project)): hashlib.sha256(p.read_bytes()).hexdigest()
                           for p in (project / 'out').rglob('*') if p.is_file()}

    def refused(label, argv, message):
        before = lock_path.read_bytes()
        call(prefix + label, argv, expect_ok=False, expect_text=message)
        record(prefix + label + ': no binding or output rewrite',
               lock_path.read_bytes() == before and local_path.read_bytes() == local and unchanged_outputs(),
               'lock SHA-256 ' + hashlib.sha256(before).hexdigest())

    base = ['build', '--project', str(project)]
    # Valid JSON with a changed identity must fail independently of transport pins.
    mutations = (
        ('SDK content', ('sdk_content_sha256',), 'locked SDK/toolchain identity mismatch'),
        ('compiler policy', ('contract_sha256',), 'toolchain contract digest mismatch'),
        ('LLVM tool', ('tools', 'cc'), 'locked external tool digest mismatch'),
    )
    for label, fields, message in mutations:
        changed = copy.deepcopy(lock)
        target = changed['selection']
        for field in fields[:-1]:
            target = target[field]
        value = target[fields[-1]]
        target[fields[-1]] = ('0' if value[0] != '0' else '1') + value[1:]
        lock_path.write_text(json.dumps(changed, indent=2) + '\n')
        try:
            refused('tampered project lock ' + label + ' is refused', base, message)
        finally:
            lock_path.write_bytes(original)

    # A different selector is a request to rebind, not authority to rewrite a lock.
    other = ('0' if content_sha256[0] != '0' else '1') + content_sha256[1:]
    config = project / '.sdk.json'
    saved_config = config.read_bytes() if config.exists() else None
    config.write_text(json.dumps({'format': 0, 'sdk': other, 'cc': '/absent/compiler'}) + '\n')
    try:
        call(prefix + 'changed SDK defaults do not rebind a locked build', base,
             check=lambda r: r['data']['cache']['executed'] == 0 and not r['data']['commands'])
        record(prefix + 'changed defaults preserve exact lock, local binding and outputs',
               lock_path.read_bytes() == original and local_path.read_bytes() == local and unchanged_outputs())
        refused('explicit SDK replacement requires configure', [*base, '--sdk', other], 'use configure explicitly')
    finally:
        if saved_config is None:
            config.unlink()
        else:
            config.write_bytes(saved_config)

    call(prefix + 'restored project lock reuses the original SDK', base,
         check=lambda r: r['data']['cache']['executed'] == 0 and not r['data']['commands'])
    record(prefix + 'final binding and outputs equal their original bytes',
           lock_path.read_bytes() == original and local_path.read_bytes() == local and unchanged_outputs(),
           'original lock SHA-256 ' + hashlib.sha256(original).hexdigest())
