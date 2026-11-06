"""Seatbelt adapter for native CLI replay. Everything here is test-host code."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


def host_compile(compiler, source, output):
    sdk = subprocess.check_output(['/usr/bin/xcrun', '--sdk', 'macosx', '--show-sdk-path'], text=True).strip()
    subprocess.run([str(compiler), '-isysroot', sdk, str(source), '-o', str(output)], capture_output=True, check=True)


class MacSandbox:
    os_kind = 'macos'
    isolation = ('Seatbelt: hidden sources/inputs/private PKI, cleared environment, private writable scratch/temp, '
                 'declared native executable allowlist; loopback networking enabled only for HTTPS replay')

    def prepare(self, work, scratch, cli, tools, hidden, *, network=False, hostile=None):
        assert sys.platform == 'darwin' and Path('/usr/bin/sandbox-exec').is_file()
        self.work, self.scratch, self.cli = work, scratch, cli
        self.hidden = list(hidden)
        self.network, self.hostile = network, hostile
        self.counter = 0
        self.temp = work / 'tmp'; self.temp.mkdir()
        self.probe = work / 'boundary-probe'
        self.forbidden = [Path(p).resolve() for p in ('/bin/sh', '/bin/bash', '/bin/zsh', '/usr/bin/make', sys.executable)]
        self.forbidden += [Path(p).resolve() for n in ('cmake', 'ninja') if (p := shutil.which(n))]
        source = work / 'boundary-probe.c'
        source.write_text('''#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
int main(int argc, char **argv) {
  if (argc>2 && !strcmp(argv[1],"environment")) {
    int i=2;
    while(i+1<argc && strcmp(argv[i],"--")) {
      if(strncmp(argv[i],"DYLD_",5) || setenv(argv[i],argv[i+1],1)) return 4;
      i+=2;
    }
    if(i+1>=argc) return 5;
    execv(argv[i+1],argv+i+1); return 6;
  }
  for (int i=1; i+1<argc; i+=2) {
    if (!strcmp(argv[i], "read")) {
      int fd=open(argv[i+1], O_RDONLY);
      if (fd>=0) { close(fd); fprintf(stderr,"unexpected read: %s\\n",argv[i+1]); return 1; }
    } else {
      pid_t pid=fork(); int status;
      if (pid<0) return 2;
      if (!pid) { execl(argv[i+1],argv[i+1],"--version",(char *)0); _exit(93); }
      if (waitpid(pid,&status,0)!=pid || !WIFEXITED(status) || WEXITSTATUS(status)!=93) return 3;
    }
  }
  puts("boundary probe: pass"); return 0;
}
''')
        host_compile(tools['cc'], source, self.probe)
        allowed = [cli, self.probe, Path('/usr/bin/xcrun'), Path('/usr/bin/xcode-select'),
                   Path('/Applications/Xcode.app/Contents/Developer/usr/bin/xcodebuild')]
        allowed += [Path(p) for p in tools.values()]
        allowed += [Path(tools['ar']).resolve().parent / 'ranlib']
        self.allowed = sorted({p.absolute() for p in allowed} | {p.resolve() for p in allowed})

    def command(self, tail, user_home, index=None, readonly_paths=()):
        self.counter += 1
        quote = lambda p: json.dumps(str(p), ensure_ascii=False)
        text = '(version 1)\n(allow default)\n'
        if not self.network:
            text += '(deny network*)\n'
        for path in self.hidden:
            text += '(deny file-read* file-write* (subpath ' + quote(path) + '))\n'
        text += '(deny file-write* (require-not (require-any (subpath "/dev") (subpath ' + quote(self.scratch) + ') (subpath ' + quote(self.temp) + '))))\n'
        for path in readonly_paths:
            text += '(deny file-write* (literal ' + quote(path) + '))\n'
        terms = ' '.join('(literal ' + quote(path) + ')' for path in self.allowed)
        text += '(deny process-exec (require-not (require-any ' + terms + ' (subpath ' + quote(self.scratch) + '))))\n'
        policy = self.work / f'policy-{self.counter:03}.sb'
        policy.write_text(text)
        child = [str(self.cli), '--json', *map(str, tail)]
        if self.hostile:
            # sandbox-exec's protected startup removes DYLD_*; inject hostile
            # loader paths afterwards so the consumer's filtering is tested.
            injected = [item for key in ('DYLD_LIBRARY_PATH', 'DYLD_FRAMEWORK_PATH', 'DYLD_FALLBACK_LIBRARY_PATH')
                        for item in (key, str(self.hostile))]
            child = [str(self.probe), 'environment', *injected, '--', *child]
        return ['/usr/bin/sandbox-exec', '-f', str(policy), *child]

    def process_options(self, user_home):
        env = {'HOME': str(user_home), 'PATH': str(self.hostile) if self.hostile else '/nonexistent',
               'TMPDIR': str(self.temp), 'LC_ALL': 'C'}
        if self.hostile:
            for key in ('LD_LIBRARY_PATH', 'DYLD_LIBRARY_PATH', 'DYLD_FRAMEWORK_PATH', 'DYLD_FALLBACK_LIBRARY_PATH', 'LIBRARY_PATH', 'CPATH',
                        'CPLUS_INCLUDE_PATH', 'CRT_SYSROOT', 'CRT_ROOTFS'):
                env[key] = str(self.hostile)
        return {'cwd': self.hostile or self.scratch, 'env': env, 'stdin': subprocess.DEVNULL}

    def verify(self, record):
        argv = self.command([], self.scratch)
        argv = argv[:3] + [str(self.probe)]
        for path in self.hidden:
            argv += ['read', str(path)]
        for path in self.forbidden:
            argv += ['exec', str(path)]
        proc = subprocess.run(argv, capture_output=True, timeout=60, **self.process_options(self.scratch))
        (self.work / 'boundary-result.json').write_text(json.dumps({'argv': argv, 'exit': proc.returncode,
            'stdout': proc.stdout.decode(errors='replace'), 'stderr': proc.stderr.decode(errors='replace')}, indent=2) + '\n')
        record('Seatbelt denies hidden input reads and undeclared interpreter/build-tool execution',
               proc.returncode == 0 and b'boundary probe: pass' in proc.stdout)
