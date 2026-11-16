#!/usr/bin/env python3
"""Focused regression audit for Gemini's GASH claims. Requires a locally built build/gash.
Run from repository root: python3 audit_gemini.py
Does not modify repository files; GASH sources and emitted binaries use a temp directory.
"""
from pathlib import Path
import subprocess, tempfile, re, sys

ROOT = Path.cwd()
GASH = (ROOT / 'build' / 'gash').resolve()
if not GASH.is_file():
    sys.exit('ERROR: build/gash not found; run cmake -S . -B build && cmake --build build')

# expect_stdout is the exact printed standard output (after stripping only trailing newlines).
# Expected-error cases should report nonzero exit and a useful runtime diagnostic.
CASES = [
    ('global_in_function', '''var g = 10;\nfunc addG(int x) { return x + g; }\noutput(addG(5));\n''', 'success', '15'),
    ('output_bool', '''var flag = true;\noutput(flag);\n''', 'success', 'true'),
    ('output_list', '''var nums = list<int>(2);\nnums[0] = 7;\nnums[1] = 8;\noutput(nums);\n''', 'success_nonempty', None),
    ('divide_zero', '''var x = 10;\noutput(x / 0);\n''', 'runtime_error', None),
    ('modulo_zero', '''var x = 10;\noutput(x % 0);\n''', 'runtime_error', None),
    ('power_zero_negative', '''output(0 ^ -1);\n''', 'runtime_error', None),
    ('repeat_negative', '''repeat(-3) { output(111); }\noutput(222);\n''', 'runtime_error', None),
    ('char_quote', "output('\\'');\n", 'success', "'"),
    ('char_nul', "var x = '\\0';\noutput(int(x));\n", 'success', '0'),
    ('nested_function', '''var base = 2;\n{\n  var offset = 3;\n  func helper(int x) { return x + offset; }\n  output(helper(base));\n}\n''', 'success', '5'),
    ('list_parameter', '''func first(list<int> xs) { return xs[0]; }\nvar xs = list<int>(1);\nxs[0] = 9;\noutput(first(xs));\n''', 'success', '9'),
    ('missing_return', '''func maybe(bool condition) {\n  if (condition) { return 7; }\n}\noutput(maybe(false));\n''', 'runtime_error', None),
    ('caught_runtime_error', '''var balance = 10;\nguard balance >= 0;\noutput(123);\nbalance = -1;\n''', 'runtime_error_with_prefix', '123'),
    ('syntax_location', '''var x = 1;\nvar y = 2\noutput(x);\n''', 'syntax_location', None),
]

def run(cmd, cwd, timeout=25):
    try:
        p = subprocess.run(cmd, cwd=cwd, capture_output=True, timeout=timeout, text=True, errors='replace', stdin=subprocess.DEVNULL)
        return p.returncode, p.stdout, p.stderr, ''
    except subprocess.TimeoutExpired:
        return None, '', '', f'timed out after {timeout}s'
    except OSError as e:
        return None, '', '', str(e)

print('GASH Gemini regression audit')
print('Compiler:', GASH)
print('NOTE: FAIL means the proposed fix is not demonstrated; inspect the diagnostic to distinguish unsupported syntax from a genuine bug.\n')

results=[]
with tempfile.TemporaryDirectory(prefix='gash_gemini_audit_') as folder:
    tmp=Path(folder)
    for name, program, mode, expected in CASES:
        src=tmp/(name+'.gash')
        src.write_text(program, encoding='utf-8')
        checked, outcheck, errcheck, echeck = run([str(GASH),'--check',str(src)], ROOT)
        code=out=err=''
        if mode=='syntax_location':
            diagnostic=(outcheck+'\n'+errcheck)
            # Require a line+column indicator; deliberately tolerant of formatting.
            loc = bool(re.search(r'(?i)(line\s*\d+.{0,60}col(?:umn)?\s*\d+|\b\d+\s*:\s*\d+\b)', diagnostic))
            ok = checked is not None and checked!=0 and loc
            reason=f'--check status={checked}; location={loc}'
        elif checked!=0:
            ok=False
            reason=f'--check rejected source (status={checked}); cannot assess emitted C++/runtime'
        else:
            code,out,err,erun=run([str(GASH),'--run',str(src)], ROOT)
            norm=out.replace('\r\n','\n').rstrip('\n')
            alltext=(out+'\n'+err)
            if mode=='success':
                ok=code==0 and norm==expected
            elif mode=='success_nonempty':
                ok=code==0 and bool(norm.strip())
            elif mode=='runtime_error':
                ok=code is not None and code!=0 and (bool(err.strip()) or bool(out.strip()))
            elif mode=='runtime_error_with_prefix':
                ok=code is not None and code!=0 and '123' in out and bool(err.strip())
            else:
                ok=False
            reason=f'--check=0; --run status={code}; stdout={out.strip()[:130]!r}; stderr={err.strip()[:160]!r}'
            if erun: reason+='; '+erun
        results.append(ok)
        print(f'{"PASS" if ok else "FAIL"}: {name}: {reason}')
        if not ok:
            diagnostic=(outcheck+'\n'+errcheck+('\n'+err if checked==0 else '')).strip()
            if diagnostic: print('    diagnostic:', diagnostic[:360].replace('\n',' | '))

    # Check --run from outside repo; pass an absolute path to the source.
    outside=tmp/'outside_cwd.gash'; outside.write_text('output(42);\n', encoding='utf-8')
    checked,_,_,_=run([str(GASH),'--check',str(outside)],ROOT)
    status,out,err,e=run([str(GASH),'--run',str(outside)],tmp)
    ok=checked==0 and status==0 and out.replace('\r\n','\n').rstrip('\n')=='42'
    results.append(ok)
    print(f'{"PASS" if ok else "FAIL"}: run_outside_repo: --check={checked}; --run status={status}; stdout={out.strip()!r}; stderr={err.strip()[:180]!r}')

print(f'\nAudit result: {sum(results)}/{len(results)} checks passed')
print('Separate manual review needed: top-level/main function return coverage; nested recursion; list formatting semantics; output escaping on all platforms; exact diagnostic type; CMake test count and documentation.')
sys.exit(0 if all(results) else 1)
