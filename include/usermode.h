/* usermode.h — the one-way door from ring 0 into ring 3.
 *
 * There's no CPU instruction that just says "lower my privilege". The only
 * mechanism that changes CPL is returning from an interrupt (`iret`), which
 * restores CS/SS from whatever values are sitting on the stack. So we build
 * a FAKE interrupt-return frame by hand and `iret` into it — the CPU can't
 * tell the difference between "genuinely returning from an interrupt" and
 * "someone constructed a frame that looks like one". */
#ifndef USERMODE_H
#define USERMODE_H

#include <stdint.h>

/* Never returns (in the success case) — control passes to `entry` running
 * at CPL 3, using `user_stack` as its ESP. */
void enter_usermode(uint32_t entry, uint32_t user_stack);

#endif
