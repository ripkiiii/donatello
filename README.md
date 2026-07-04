# Donatello

A 32-bit (i686) operating system written from scratch — no Linux, no libc,
no wrapper around an existing kernel. Bootstrap assembly and a C kernel
that boot on bare metal (tested in QEMU), built one verifiable milestone
at a time.

```
donatello> run hello
elf: entry=0x00200000, 1 program header(s)
elf: jumping to entry point.
Hello from a LOADED program! (not the kernel talking)
```

That line — a program written separately, stored on disk, found by name,
and executed on top of a kernel built from nothing — is the whole point.

## What's built

| # | Milestone | What it proves |
|---|---|---|
| 1 | Boot + VGA text output | Multiboot handshake, freestanding C, no libc |
| 2 | GDT | Flat memory segmentation |
| 3 | IDT + interrupts | CPU exceptions caught and handled |
| 4 | PIC + keyboard | Real hardware input, IRQ remapping |
| 5 | Shell | A REPL living inside the kernel |
| 6 | Memory | Physical frame allocator, paging (identity-mapped MMU), page fault handling, a `kmalloc`/`kfree` heap |
| 7 | ELF loader | Parses and runs a genuinely separate compiled program |
| 8 | User mode + syscall | Ring 3 isolation via GDT/TSS, `int $0x80` syscalls, page-level access control — verified with a deliberate isolation-violation test |
| 9 | Filesystem | An ATA PIO disk driver and a minimal flat-file filesystem — programs are found **by name** on disk, not baked into the kernel image |

Every milestone above is verified, not assumed — each one includes a
shell command or test program that deliberately tries to break the thing
being built (touch unmapped memory, violate ring-3 isolation, etc.) and
confirms the kernel catches it correctly instead of just "not crashing."

## Build & run

Needs `i686-elf-gcc` (a cross-compiler — the Mac's own compiler can't
target this), `qemu-system-i386`, and `python3` (packs the disk image).

```sh
make        # builds build/donatello.bin
make run    # builds disk.img, boots it in QEMU with the disk attached
make clean
```

Type `help` once it boots for the full command list (`ls`, `run <name>`,
`usermode`, `pagefault`, `heaptest`, and more — each one a live
demonstration of a specific milestone above).

## Why

Not trying to replace Linux — that itch is scratched by
[Kilian](https://github.com/ripkiiii/kilian). This is about understanding
what an operating system actually *is* by building the smallest real
version of one: a kernel that manages its own memory, isolates the
programs it runs, and finds them by name on its own disk.

## What's next

A staged plan toward running a small TCP key-value store natively on
Donatello — PCI enumeration, a NIC driver, ARP/IP/TCP from scratch, then
socket syscalls. Slow and deliberate, the same way everything above got
built.
