> [!IMPORTANT]
> SiriusOS is an pre-alpha software.
>
> It is not recommended to use it as daily driver.
>
> Especially right now with the source code reorganization and global changes in the architecture.

# SiriusOS

SiriusOS is a fork of [Toaru](https://github.com/klange/toaruos) operating system for x86-64 PCs and experimental support for ARMv8.

Some features like kuroko was removed due to inactivity

Also we created an distribution creation [guide](docs/CreatingDistribution.md) on kSir (misaka) kernel.

## History

> 2 July 2024
>
> I am created an COSMOS based OS
>
> 27 July 2024
> I am tried to recreate OS in C
>
> 2 August 2024
> I have decided that SiriusOS is going to be fork of ToaruOS because it is [no longer maintained](https://en.wikipedia.org/wiki/ToaruOS#History)

## Features

- **Dynamically linked userspace** with shared libraries and `dlopen`.
- **Composited graphical UI** with software acceleration and a late-2000s design inspiration.
- **VM integration** for absolute mouse and automatic display sizing in VirtualBox and VMware Workstation.
- **Unix-like terminal interface** including a feature-rich terminal emulator and several familiar utilities.
- **Optional third-party ports** including GCC 10.3, Binutils, SDL1.2, Quake, and more.

### Notable Components

- **kSir (Misaka)** (kernel), [kernel/](kernel/), a hybrid modular kernel, and the core of the operating system.
- **Yutani (New Generation/NG)** (window compositor), [apps/compositor.c](apps/compositor.c), manages window buffers, layout, and input routing.
- **Terminal**, [apps/terminal.c](apps/terminal.c), xterm-esque terminal emulator with 24-bit color support.
- **ld.so** (dynamic linker/loader), [linker/linker.c](linker/linker.c), loads dynamically-linked ELF binaries.
- **NeoEsh** (shell), [apps/nesh.c](apps/nesh.c), supports pipes, redirections, variables, etc.

## Current Goals

The following projects are currently in progress:

- **Add more ports**
  - **Bring back ports** from ToaruOS "Legacy", like muPDF and Mesa.
- **Binary compatibility with ToaruOS**
- **Partial kernel rewrite**
  - **Hybrid kernel --> Microkernel**
- **Rewrite the network stack** for greater throughput, stability, and server support.
- **Support more hardware** with new device drivers for AHCI, USB, virtio devices, etc.
- **Continue to improve the C library** which remains quite incomplete compared to Newlib and is a major source of issues with bringing back old ports.
- **More architectures**
  - **PowerPC (32 + 64)**
  - **sparc (32 bit only)**
  - **mips**
  - **riscv**
  - **arm32**
  - **LoongArch64**
- **Use MinixFS3 instead of ext2**
  - [x] **Read mode**
  - [ ] **Write mode**

## Future Goals / Longbox
- **Improve SMP performance** with better scheduling and smarter userspace synchronization functions.

### Project Layout

- **apps** - Userspace applications, all first-party.
- **base** - Ramdisk root filesystem staging directory. Includes C headers in `base/usr/include`, as well as graphical resources for the compositor and window decorator.
- **boot** - BIOS and EFI loader with interactive menus.
- **build** - Auxiliary build scripts for platform ports.
- **kernel** - The kSir (Misaka New Generation) kernel.
- **lib** - Userspace libraries.
- **libc** - C standard library implementation.
- **linker** - Userspace dynamic linker/loader, implements shared library support.
- **modules** - Loadable driver modules for the kernel.
- **util** - Utility scripts, staging directory for the toolchain (binutils/gcc).

### Filesystem Layout

The root filesystem is set up as follows:

- `bin`: First-party applications.
- `cdrom`: Mount point for the CD, if available.
- `dev`: Virtual device directory, generated by the kernel.
  - `net`: Network interface devices.
  - `pex`: Packet Exchange hub, lists accessible IPC services.
  - `pts`: PTY secondaries, endpoints for TTYs.
- `etc`: Configuration files, startup scripts.
- `home`: User directories.
- `lib`: First-party libraries
- `mod`: Loadable kernel modules.
- `proc`: Virtual files that present kernel state.
  - `1`, etc.: Virtual files with status information for individual processes.
- `src`: Source files, see "Project Layout" section above.
- `tmp`: Mounted as a read/write tmpfs normally.
- `usr`: Userspace resources
  - `bin`: Third-party applications, normally empty until packages are installed.
  - `include`: Header files, including potentially ones from third-party packages.
  - `lib`: Third-party libraries. Should have `libgcc_s.so` by default.
  - `share`: Various resources.
    - `cursor`: Mouse cursor sprites.
    - `fonts`: TrueType font files. Live CDs ship with Deja Vu Sans.
    - `games`: Dumping ground for game-related resource files, like Doom wads.
    - `help`: Documentation files for the Help Browser application.
    - `icons`: PNG icons, divided into further directories by size.
    - `ttk`: Spritesheet resources for the window decorator and widget library.
    - `wallpapers`: JPEG wallpapers.
- `var`: Runtime files, including package manager manifest cache, PID files, some lock files, etc.

## Running SiriusOS

### VirtualBox and VMware Workstation

The best end-user experience with SiriusOS will be had in either of these virtual machines, as SiriusOS has support for their automatic display sizing and absolute mouse positioning.

Set up a new VM for an "other" 64-bit guest, supply it with at least 1GiB of RAM, attach the CD image, remove or ignore any hard disks, and select an Intel Gigabit NIC. Two or more CPUs are recommended, as well.

By default, the bootloader will pass a flag to the VirtualBox device driver to disable "Seamless" support as the implementation has a performance overhead. To enable Seamless mode, use the bootloader menu to check the "VirtualBox Seamless" option before booting. The menu also has options to disable automatic guest display sizing if you experience issues with this feature.

### QEMU

Most development of SiriusOS happens in QEMU, as it provides the most flexibility in hardware and the best debugging experience. A recommended QEMU command line in an Ubuntu 20.04 host is:

```
qemu-system-x86_64 -enable-kvm -m 1G -device AC97 -cdrom image.iso -smp 2
```

Replace `-enable-kvm` with `-accel hvm` or `-accel haxm` as appropriate on host platforms without KVM, or remove it to try under QEMU's TCG software emulation.

Note that QEMU command line options are not stable and these flags may produce warnings in newer versions.

The option `-M q35` will replace the PIIX chipset emulation with a newer one, which has the side effect of switching the IDE controller for a SATA one. This can result in faster boot times at the expense of ToaruOS not being able to read its own CD at runtime until I get around to finishing my AHCI driver.

### Other

SiriusOS has been successfully tested on real hardware. If the native BIOS or EFI loaders fail to function, try booting with Grub. SiriusOS complies with the "Multiboot" and "Multiboot 2" specs so it may be loaded with either the `multiboot` or `multiboot2` commands as follows:

```
multiboot2 /path/to/misaka-kernel root=/dev/ram0 migrate vid=auto start=live-session
module2 /path/to/ramdisk.igz
set gfxpayload=keep
```

## License

All first-party parts of SiriusOS are made available under the terms of the University of Illinois / NCSA License, which is a BSD-style permissive license.
Unless otherwise specified, this is the original and only license for all files in this repository - just because a file does not have a copyright header does not mean it isn't under this license.
SiriusOS is intended as an educational reference, and I encourage the use of my code, but please be sure you follow the requirements of the license.
You may redistribute code under the NCSA license, as well as make modifications to the code and sublicense it under other terms (such as the GPL, or a proprietary license), but you must always include the copyright notice specified in the license as well as make the full text of the license (it's only a couple paragraphs) available to end-users.

While most of SiriusOS is written entirely by myself* (K. Lange), be sure to include other authors where relevant, such as with [Mike's audio subsystem](kernel/audio/snd.c) or [Dale's string functions](kernel/misc/string.c).
