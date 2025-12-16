# ABOUT HEARTIX (heavy-modified Ghost)

Heartix is a heavily modified fork of the Ghost microkernel project. It keeps the
original Ghost architecture and copyright notices by Max Schlüssel, but rebrands
as **Heartix** and tracks its own versioning starting at **0.1.0**.

## Status
* Kernel: Heartix 0.1.0 (heavy-modified Ghost)
* License: GPLv3 (original Ghost licensing retained)
* Credits: Max Schlüssel (upstream Ghost); Heartix modifications by Efe Ilhan Yüce and contributors.

## Documentation
See `documentation/` for design notes and build instructions inherited from Ghost.

Outputs are placed under `target/` (bootable ISO, kernel, sysroot artifacts).

## Running
Test in a VM (VirtualBox/VMware/QEMU) with at least 512 MB RAM. Prefer VMSVGA/VMware SVGA for graphics.

## Features (inherited, evolving)
* x86_64 microkernel with SMP
* ELF userland, shared libs
* libapi + libc (Heartix-branded Ghost libs)
* IPC: messages, pipes, shared memory
* Drivers: VESA/VBE/VMSVGA/Bochs-VGA, PS/2, PCI, AC97, E1000 (where available)
* Limine boot protocol compliance

## Ported/used software
libpng, pixman, zlib, cairo, freetype, musl (libm), duktape, etc. (see `patches/ports`).

## Attribution
This codebase originates from the Ghost project by Max Schlüssel. Heartix retains the GPL and upstream notices while applying its own modifications and branding.

## Contact
Heartix mods: eilhanzy@protonmail.com
Ghost upstream: lokoxe@gmail.com
