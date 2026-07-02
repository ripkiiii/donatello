# Modul Belajar DonatelloOS

Dokumen ini nemenin lu ngerti **apa yang udah dibangun di M1–M4**, kenapa
tiap bagian ada, dan konsep di baliknya. Dibuat buat dipelajari pelan-pelan,
bukan dihafal.

---

## Daftar Isi
1. [Apa itu Donatello](#1-apa-itu-donatello)
2. [Peta arsitektur (struktur folder)](#2-peta-arsitektur)
3. [Alur boot: dari nyala sampai nunggu keyboard](#3-alur-boot)
4. [M1 — Boot & layar (Multiboot + VGA)](#m1--boot--layar)
5. [M2 — GDT (segmen memori)](#m2--gdt)
6. [M3 — IDT & interrupts](#m3--idt--interrupts)
7. [M4 — PIC & keyboard](#m4--pic--keyboard)
8. [Polish terminal (backspace + kursor)](#polish-terminal)
9. [Bug & fix yang dialamin](#bug--fix)
10. [Kamus istilah](#kamus-istilah)
11. [Build & jalanin](#build--jalanin)
12. [Roadmap ke depan](#roadmap)

---

## 1. Apa itu Donatello

**Donatello = operating system dari NOL** (bukan wrapper, bukan Linux) —
bootstrap assembly + kernel C sendiri, boot di bare metal (dites di QEMU).
Arsitektur target: **x86 32-bit (i686)**.

**Arah (#1): platform buat jalanin program Rifky sendiri.** Ujungnya:
`donatello> run <program>` — program C buatan sendiri jalan di atas kernel
buatan sendiri. AI/ML atau appliance = kemungkinan di masa depan, bukan
tujuan awal.

**Filosofi "proper" versi Rifky:** *jalan bagus + bug dibenerin pas ketemu*,
BUKAN perfect. Semua software ada bug. "Done" bukan bar; "jalan bagus + terus
tumbuh" itu bar. Ini OS buat kebutuhan sendiri — ga usah nyaingin Linux (itu
tugas Kilian).

---

## 2. Peta arsitektur

```
donatello/
├── boot/
│   └── boot.s          # kode PALING pertama: multiboot header + stack + lompat ke C
├── kernel/
│   └── kernel.c        # kernel_main: orkestrasi urutan boot
├── cpu/                # barang low-level x86
│   ├── gdt.c / gdt_flush.s     # Global Descriptor Table (segmen memori)
│   ├── idt.c / idt_load.s      # Interrupt Descriptor Table
│   ├── isr.s                   # stub exception (0-31) + IRQ (32-47) + common
│   └── irq.c                   # remap PIC + dispatch IRQ + EOI
├── drivers/
│   ├── terminal.c      # nulis teks ke layar (VGA text mode)
│   └── keyboard.c      # handler IRQ1: scancode -> huruf
├── include/            # semua header (.h), dicari lewat -Iinclude
│   ├── gdt.h idt.h irq.h keyboard.h terminal.h io.h
├── linker.ld           # peta memori: kernel di 1 MiB, multiboot paling depan
├── Makefile            # build (build/) + run (qemu) + clean
└── docs/
    └── modul.md        # dokumen ini
```

**Pemisahan tanggung jawab:** `cpu/` = ngomong sama CPU (segmen, interrupt).
`drivers/` = ngomong sama perangkat (layar, keyboard). `kernel/` = ngerakit
semua. Ini pola OS beneran: kernel tipis di tengah, modul di sekelilingnya.

---

## 3. Alur boot

Urutan persis dari mesin nyala sampai nunggu ketikan:

```
1. QEMU cari Multiboot header di kernel  → ketemu → lompat ke _start (boot.s)
2. boot.s: pasang stack (esp)            → call kernel_main (kernel.c)
3. term_init()                           → bersihin layar
4. gdt_init()                            → pasang GDT sendiri (segmen memori)
5. idt_init()                            → pasang tabel handler interrupt
6. pic_remap()                           → geser IRQ hardware ke vektor 32-47
7. keyboard_init()                       → daftarin handler buat IRQ1
8. sti                                    → NYALAIN interrupt hardware
9. for(;;) hlt                            → tidur; bangun tiap ada interrupt
```

Sekarang tiap lu pencet tombol → CPU bangun dari `hlt` → handler keyboard
jalan → huruf muncul → balik tidur. Itu "loop hidup" Donatello sekarang.

---

## M1 — Boot & layar

### Multiboot (kenapa QEMU mau boot kita)
Bootloader (GRUB/QEMU) nyari **"kartu nama"** di 8 KiB pertama kernel: angka
ajaib `0x1BADB002`. Ketemu → "ini kernel, gue boot". Di `boot.s`:
```asm
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)   # MAGIC+FLAGS+CHECKSUM harus = 0 (validasi)
```
Kita adopsi standar Multiboot supaya **ga usah nulis bootloader sendiri**
(yang butuh 16-bit real mode asm — neraka).

### Kenapa butuh assembly sama sekali
Pas mesin nyala, **belum ada stack**. C ga bisa jalan tanpa stack (function
call, variabel lokal). Jadi `boot.s` pesan 16 KiB (`.skip 16384`), arahin
`esp` ke situ, baru `call kernel_main`. Stack numbuk ke bawah → mulai dari
`stack_top`.

### Layar = memori (VGA text mode)
Ada slab memori di `0xB8000` yang **langsung dilukis hardware ke layar**.
Grid 80×25. **Tiap sel = 2 byte: [huruf | warna].** Nulis ke situ = huruf
muncul seketika. Ga ada `printf` — kita bikin sendiri (`drivers/terminal.c`).
```c
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)
```
`volatile` = "compiler, jangan optimize tulisan ini, ini hardware beneran".

### Freestanding
Ga ada OS/libc di bawah kita. Jadi ga ada `strlen`, `printf`, `malloc` —
semua DIY. Flag compiler: `-ffreestanding -nostdlib`.

---

## M2 — GDT

### Masalah yang dipecahin
x86 punya warisan **segmentation**: tiap akses memori diam-diam lewat
"segmen". Ga bisa dimatiin (nyatu di CPU). Bayangin satpam yang maksa ngecek
"lu mau ke zona mana?" tiap akses memori.

### Solusi: flat model (LUMPUHIN segmentation)
Kita bikin GDT yang **semua segmennya nutupin SELURUH 4GB memori** (base 0,
limit 4GB). Efeknya: segmentation jadi no-op, satpamnya ngelambaiin semua
lewat. Kita nyetup GDT justru buat **minggirin** segmentation, bukan makenya.
Manajemen memori beneran nanti pake **paging** (M6).

### Isi minimal (3 descriptor)
- **Null** (entri 0, wajib nol — jebakan keamanan)
- **Code** (base 0, limit 4GB, boleh eksekusi, ring 0) — access byte `0x9A`
- **Data** (base 0, limit 4GB, boleh tulis, ring 0) — access byte `0x92`

### Bagian tricky
- **Struct kececer.** Field `base`/`limit` dipecah aneh (low/mid/high) karena
  format warisan 286. `__attribute__((packed))` biar compiler ga tambah
  padding. Kita korban format-nya, bukan desainer-nya.
- **Angka ajaib** = flag per-bit. `0x9A` vs `0x92` beda cuma di bit
  "executable" (code vs data). `0xCF` = granularity 4KB + mode 32-bit.
- **Ring 0 vs 3** = akar "kernel vs user space" (materi inti GIOS).
- **CS wajib far jump.** DS/ES/SS bisa `mov`, tapi CS (segmen kode yang lagi
  jalan) cuma bisa diganti lewat **lompatan** (`ljmp`) — lihat `gdt_flush.s`.

---

## M3 — IDT & interrupts

### Konsep interrupt
CPU lagi jalanin kode → **ada yang nowel** (butuh perhatian) → CPU pause →
lompat ke handler → urus → **balik persis** ke tempat berhenti. Lawannya
**polling** (terus nanya "ada input?" — boros). Interrupt = efisien.

### 3 sumber
1. **Hardware (IRQ)** — keyboard, timer. Dari luar, kapan aja.
2. **Exception** — CPU teriak pas error: bagi nol, page fault. Dari kode.
3. **Software** (`int N`) — sengaja dipicu, contoh system call.

### IDT
Tabel 256 slot: "kalau interrupt N kejadian, lompat ke handler mana". Di-load
pake `lidt`. **GDT harus duluan** karena tiap entri IDT nunjuk ke code segment
selector (`0x08`) di GDT.

### Cara stub kerja (`cpu/isr.s`)
1. Tiap exception punya **stub** kecil. Yang ga punya error code push dummy
   `0` (biar frame seragam), terus push nomor interrupt.
2. **`isr_common`**: `pusha` (foto semua register) + simpen segmen → `push
   %esp` (alamat foto) → `call isr_handler(registers_t*)` → restore semua →
   **`iret`**.
3. **`iret` ≠ `ret`.** `iret` pop lebih banyak (`eip`+`cs`+`eflags` yang
   di-push CPU) → balik ke tempat + kondisi persis sebelum diinterupsi.
4. **`registers_t`** (di `idt.h`) urutannya HARUS sama persis dengan urutan
   push di stub — ini kontrak antara asm & C.

### Bukti
`kernel_main` sempet picu `int $3` / `int $4` (dulu, sekarang diganti
keyboard). Handler print vektornya (merah), terus `iret` balik → "we
survived". Buktinya mesin interrupt jalan bolak-balik.

---

## M4 — PIC & keyboard

### PIC + remap
Hardware IRQ lewat chip **PIC**. Default-nya PIC naro IRQ di vektor 0-15 —
**tabrakan sama exception CPU (0-31)!** Solusi: **remap** → geser ke 32-47
(`cpu/irq.c` → `pic_remap()`). Abis remap: **IRQ0 (timer)=32, IRQ1
(keyboard)=33.** Urutan ICW (`0x11` → offset → wiring → mode) itu protokol
kaku yang PIC maunya persis.

### Port I/O ≠ memori
PIC & keyboard diakses lewat **port** (`inb`/`outb`, lihat `io.h`), bukan
alamat memori biasa. Baca tombol = `inb(0x60)`.

### EOI (jebakan klasik)
Abis handle IRQ, **WAJIB lapor "selesai"** (kirim `0x20` ke command PIC).
Lupa → PIC ngira masih sibuk → ga kirim IRQ lagi → keyboard mati 1 tombol.
Kalau IRQ dari slave (≥8), lapor ke dua-duanya. Ini beda utama antara
`irq_handler` (hardware, kirim EOI) dan `isr_handler` (exception, nggak).

### Scancode → ASCII
Keyboard ngasih **scancode** (angka posisi tombol + status pencet/lepas),
bukan huruf. Handler baca dari `0x60`, terjemahin lewat tabel `keymap`
(`drivers/keyboard.c`). Bit atas set = tombol dilepas (break code) → diabaikan.

---

## Polish terminal

### Backspace = hapus
`'\b'` (dari tombol Delete) tadinya jatuh ke "cetak apa adanya" → keluar
glyph kotak. Fix di `term_putchar`: cabang khusus `'\b'` → mundurin posisi 1
sel → timpa pake spasi.

### Kursor hardware ngikutin
Ada **dua kursor**: posisi software kita (`term_row/col`, buat naruh huruf)
dan kursor fisik VGA (garis kedip). Tadinya ga nyambung. `term_update_cursor()`
nulis posisi ke port CRTC (`0x3D4`/`0x3D5`, register 14 & 15 — dipecah byte
atas/bawah karena posisi 16-bit lewat port 8-bit). Dipanggil tiap
`term_putchar` → kursor selalu sinkron ke teks.

---

## Bug & fix

| Bug | Gejala | Akar masalah | Fix |
|---|---|---|---|
| Keyboard freeze | Ketik beberapa huruf terus mati | Handler baca 1 byte/IRQ; buffer controller penuh → OBF nyangkut → IRQ berhenti | Loop baca sampai kosong (`while inb(0x64) & 1`) di `keyboard.c` |
| Delete → kotak | Pencet Delete keluar □ | `'\b'` dicetak sebagai glyph, ga di-handle | Cabang `'\b'` di `term_putchar` (hapus beneran) |
| Kursor ga ngikut | `_` nyangkut di pojok | Kursor hardware ga pernah dipindah | `term_update_cursor()` via port CRTC |
| Screenshot rusak | File ga kebuka | QEMU nulis PPM walau ekstensi `.png` | Convert PPM→PNG (bukan bug OS) |

**Pelajaran umum:** tiap "aturan hardware" (format GDT, urutan ICW PIC, EOI,
port CRTC) itu **kontrak yang harus diikutin persis** — bukan pilihan kita.

---

## Kamus istilah

- **Multiboot** — standar/jabat-tangan bootloader↔kernel; kernel yang nurut
  bisa di-boot GRUB/QEMU tanpa nulis bootloader sendiri.
- **Freestanding** — jalan tanpa OS/libc di bawah; semua DIY.
- **VGA text mode** — layar 80×25 di `0xB8000`, tiap sel = huruf+warna.
- **GDT** — Global Descriptor Table; ngejelasin segmen memori (base/limit/izin).
- **Segmentation** — mekanisme legacy x86 yang maksa akses memori lewat segmen.
- **Flat model** — bikin semua segmen nutupin 4GB → segmentation jadi no-op.
- **Ring** — level privilege; 0=kernel (kuasa penuh), 3=program user.
- **Selector** — angka yang nunjuk entri di GDT (`0x08`=code, `0x10`=data).
- **IDT** — Interrupt Descriptor Table; handler buat tiap interrupt (256 slot).
- **ISR** — Interrupt Service Routine; kode yang jalan pas interrupt.
- **Exception** — interrupt yang CPU picu sendiri pas error (bagi nol, dll).
- **IRQ** — Interrupt Request; interrupt dari hardware (keyboard=IRQ1).
- **PIC** — chip yang nyalurin IRQ hardware ke CPU; kita remap ke 32-47.
- **EOI** — End Of Interrupt; lapor ke PIC "selesai" biar dikirim IRQ lagi.
- **Scancode** — angka dari keyboard nandain tombol + pencet/lepas.
- **Port I/O** — akses hardware lewat port (`inb`/`outb`), beda dari memori.
- **`iret`** — instruksi balik dari interrupt (pop eip/cs/eflags).
- **`lgdt`/`lidt`** — load alamat GDT/IDT ke register CPU.

---

## Build & jalanin

Butuh: `i686-elf-gcc` (cross-compiler) + `qemu-system-i386` (dari Homebrew).

```sh
make          # build → build/donatello.bin
make run      # boot di QEMU (jendela kebuka, bisa langsung ngetik)
make clean    # hapus build/
```

---

## Roadmap

| M | Status | Apa |
|---|---|---|
| 1 | ✅ | Boot + print (Multiboot + VGA) |
| 2 | ✅ | GDT (segmen memori, flat model) |
| 3 | ✅ | IDT + interrupts (exception) |
| 4 | ✅ | PIC + keyboard (input beneran) + polish terminal |
| 5 | ⏭️ | **Shell** — baca baris sampai Enter, respons ke command |
| 6 | | Memory: paging + heap (`kmalloc`) |
| 7 | | Program loader (ELF) |
| 8 | | User mode (ring 3) + syscall |
| 9 | | Filesystem — program dipanggil by name |

Ujung jalur: `donatello> run <program>` — program buatan sendiri jalan di
atas kernel buatan sendiri.
