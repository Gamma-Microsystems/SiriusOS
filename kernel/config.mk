# ===================================================================
#                 Cofiguration file for SiriusOS										
# ===================================================================
KBITS = 64
override KERNEL := ksir.$(KBITS)

# Convenience macro to reliably declare user overridable variables.
define DEFAULT_VAR =
    ifeq ($(origin $1),default)
        override $(1) := $(2)
    endif
    ifeq ($(origin $1),undefined)
        override $(1) := $(2)
    endif
endef

# Target architecture to build for. Default to x86_64.
override DEFAULT_KARCH := x86_64
$(eval $(call DEFAULT_VAR,KARCH,$(DEFAULT_KARCH)))

override DEFAULT_KCC := $(KARCH)-elf-gcc
$(eval $(call DEFAULT_VAR,KCC,$(DEFAULT_KCC)))

override KCC_IS_CLANG := no
ifeq ($(shell $(KCC) --version 2>&1 | grep -i 'clang' >/dev/null 2>&1 && echo 1),1)
    override KCC_IS_CLANG := yes
endif

# Same thing for "ld" (the linker).
override DEFAULT_KLD := $(KARCH)-elf-ld
$(eval $(call DEFAULT_VAR,KLD,$(DEFAULT_KLD)))

# User controllable C flags.
override DEFAULT_KCFLAGS := -g -pipe
$(eval $(call DEFAULT_VAR,KCFLAGS,$(DEFAULT_KCFLAGS)))

# User controllable C preprocessor flags. We set none by default.
override DEFAULT_KCPPFLAGS :=
$(eval $(call DEFAULT_VAR,KCPPFLAGS,$(DEFAULT_KCPPFLAGS)))

ifeq ($(KARCH), x86_64)
    # User controllable nasm flags.
    override DEFAULT_KNASMFLAGS := -F dwarf -g
    $(eval $(call DEFAULT_VAR,KNASMFLAGS,$(DEFAULT_KNASMFLAGS)))
endif

# User controllable linker flags. We set none by default.
override DEFAULT_KLDFLAGS :=
$(eval $(call DEFAULT_VAR,KLDFLAGS,$(DEFAULT_KLDFLAGS)))

override KCFLAGS += \
    -ffreestanding \
    -Os \
    -std=gnu11 \
    -g \
    -static \
    -Wall \
    -Wextra \
    -Wno-unused-function \
    -Wno-unused-parameter \
    -Wstrict-prototypes \
    -pedantic \
    -Wwrite-strings \
    -D_KERNEL_ \
    -DKERNEL_ARCH=$(KARCH)

# Internal C preprocessor flags that should not be changed by the user.
override KCPPFLAGS := \
	-I../base/usr/include \
	$(KCPPFLAGS) \
	-MMD \
	-MP


ifeq ($(KARCH),x86_64)
		# Internal nasm flags that should not be changed by the user.
		override KNASMFLAGS += \
			-Wall
endif

# Architecture specific internal flags.
ifeq ($(KARCH),x86_64)
    ifeq ($(KCC_IS_CLANG),yes)
        override KCC += \
            -target x86_64-elf
    endif
    override KCFLAGS += \
        -m64 \
        -march=x86-64 \
        -mno-80387 \
        -mno-mmx \
        -mno-sse \
        -mno-sse2 \
        -mno-red-zone \
        -mcmodel=kernel
    override KLDFLAGS += \
        -m elf_x86_64
    override KNASMFLAGS += \
        -f elf64
else ifeq ($(KARCH),aarch64)
    ifeq ($(KCC_IS_CLANG),yes)
        override KCC += \
            -target aarch64-elf
    endif
    override KCFLAGS += \
        -mgeneral-regs-only
    override KLDFLAGS += \
        -m aarch64elf
#else ifeq ($(KARCH),riscv64)
#    ifeq ($(KCC_IS_CLANG),yes)
#        override KCC += \
#            -target riscv64-elf
#        override KCFLAGS += \
#            -march=rv64imac
#    else
#        override KCFLAGS += \
#            -march=rv64imac_zicsr_zifencei
#    endif
#    override KCFLAGS += \
#        -mabi=lp64 \
#        -mno-relax
#    override KLDFLAGS += \
#        -m elf64lriscv \
#        --no-relax
#else ifeq ($(KARCH),loongarch64)
#    ifeq ($(KCC_IS_CLANG),yes)
#        override KCC += \
#            -target loongarch64-none
#    endif
#    override KCFLAGS += \
#        -march=loongarch64 \
#        -mabi=lp64s
#    override KLDFLAGS += \
#        -m elf64loongarch \
#        --no-relax
else
    $(error Architecture $(KARCH) not supported)
endif
