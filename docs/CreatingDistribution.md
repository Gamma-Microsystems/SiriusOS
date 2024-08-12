# Creating distribution with kSir (aka Misaka) kernel

# Babysteps

What you will need is [kernel source](../kernel/), [include](https://github.com/Gamma-Microsystems/include), [kernel modules](../modules), and also we recommend you to use [sirius C library](https://github.com/Gamma-Microsystems/libc) or otherwise 90% of features will not be available, if you are an advanced developer you can make custom include and libc.
Your distribution can use own license, not only NCSA, we recommend you to create an git repository and add kernel and include as submodules.
If you making custom patches for the kernel you will need to redistribute it with NCSA license.

# Picking up the userland
You can use SiriusOS userland, but it will make your distribution an SiriusOS fork, which must use NCSA license.
You can port GNU userland through elf toolchain, or your own.
Also you can use busybox.
And the most insane one for distributions which probably going to create own kernel in future, own userland, yes you can just implement all commands UI etc.

# Picking up the bootloader
You can use [SiriusOS bootloader](../boot), grub, limine, easyboot, etc.

But if you came here from linux distrodev we recommend you to use grub

# Termination
Now you decide, how do you want to build your OS, where you will host it, etc.
