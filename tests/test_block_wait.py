"""Run the actual getDataBlock wait block against all uint16_t inputs.

Requires Python 3 and a host C compiler (CC, or cc). No radio/serial access.
delay() records its argument, so the historical multi-day delay never runs.
"""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'source/syncedproto.c').read_text()
function = source.index('static bool getDataBlock(')
start = source.index('    if (ack->pleaseWaitMs) {', function)
end = source.index('\n\n    blockRxLoop', start)
wait_block = source[start:end].replace('printf(', 'ignore_printf(')

harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
_Static_assert(sizeof(int) == 4, "match CC2630 integer promotions");
struct blockRequestAck { uint16_t pleaseWaitMs; };
static uint32_t observed;
static unsigned calls;
static void delay(uint32_t ms) { observed = ms; calls++; }
static int ignore_printf(const char *format, ...) { (void)format; return 0; }
static void actual_wait(uint16_t wait) {
    struct blockRequestAck item = {wait};
    struct blockRequestAck *ack = &item;
    observed = 0;
    calls = 0;
__ACTUAL_WAIT_BLOCK__
}
static void legacy_wait(uint16_t wait) {
    observed = 0;
    if (wait) delay(wait - 20);
}
int main(void) {
    legacy_wait(15);
    assert(observed == UINT32_MAX - 4U);
    for (uint32_t wait = 0; wait <= UINT16_MAX; wait++) {
        actual_wait((uint16_t)wait);
        assert(observed == (wait > 20U ? wait - 20U : 0U));
        assert(calls == (wait > 20U ? 1U : 0U));
    }
    puts("PASS: legacy 15 ms underflow reproduced; actual source wait block passed all 65536 uint16 values.");
    return 0;
}
'''.replace('__ACTUAL_WAIT_BLOCK__', wait_block)

with tempfile.TemporaryDirectory(prefix='cc2630-wait-test-') as temp:
    cfile = Path(temp) / 'block_wait.c'
    binary = Path(temp) / ('block_wait.exe' if os.name == 'nt' else 'block_wait')
    cfile.write_text(harness)
    compiler = shlex.split(os.environ.get('CC', 'cc'))
    subprocess.run([*compiler, '-std=c11', '-Wall', '-Wextra', '-Werror',
                    str(cfile), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
