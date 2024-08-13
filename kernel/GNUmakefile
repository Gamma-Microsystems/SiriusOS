# Nuke built-in rules and variables.
override MAKEFLAGS += -rR

include config.mk

# Internal linker flags that should not be changed by the user.
# TODO: Add RPI4 specific stuff
override KLDFLAGS += \
    -nostdlib \
    -static \
    -z max-page-size=0x1000 \
    -T arch/$(KARCH)/link.ld

CFILES_x86_64 := $(shell find arch/x86_64 -type f -name '*.c')
ASFILES_x86_64 := $(shell find arch/x86_64 -type f -name '*.S')

CFILES_aarch64 := $(shell find arch/aarch64 -type f -name '*.c')
ASFILES_aarch64 := $(shell find arch/aarch64 -type f -name '*.S')
CFILES_UNIVERSAL := $(shell find audio fs misc net sys video -type f -name '*.c')
ASFILES_UNIVERSAL := $(shell find audio fs misc net sys video -type f -name '*.S')

override CFILES := $(if $(findstring x86_64, $(KARCH)),  $(CFILES_UNIVERSAL) $(CFILES_x86_64), $(CFILES_aarch64))
override ASFILES := $(if $(findstring x86_64, $(KARCH)), $(ASFILES_UNIVERSAL) $(ASFILES_x86_64), $(ASFILES_aarch64))
override ARCH_OBJS := $(filter-out obj-%/%.o,$(OBJ))
override ARCH_DEPS := $(filter-out obj-%/%.d,$(HEADER_DEPS))
override OBJ := $(addprefix obj-$(KARCH)/,$(CFILES:.c=.c.o) $(ASFILES:.S=.S.o))
override HEADER_DEPS := $(addprefix obj-$(KARCH)/,$(CFILES:.c=.c.d) $(ASFILES:.S=.S.d))

# Default target.
.PHONY: all
all: bin-$(KARCH)/$(KERNEL)

# Link rules for the final kernel executable.
bin-$(KARCH)/$(KERNEL): GNUmakefile arch/$(KARCH)/link.ld $(OBJ)
	mkdir -p "$$(dirname $@)"
	$(KLD) $(OBJ) $(KLDFLAGS) -o $@

# Include header dependencies.
-include $(HEADER_DEPS)

obj-$(KARCH)/%.c.o: %.c GNUmakefile
				@mkdir -p "$$(dirname $@)"
				@echo -e 'CC' $@
				@$(KCC) $(KCFLAGS) $(KCPPFLAGS) -c $< -o $@

obj-$(KARCH)/%.S.o: %.S GNUmakefile
				@mkdir -p "$$(dirname $@)"
				@echo -e 'AS' $@
				@$(KCC) $(KCFLAGS) $(KCPPFLAGS) -c $< -o $@

# Remove object files and the final executable.
.PHONY: clean
clean:
	rm -rf bin-$(KARCH) obj-$(KARCH)

.PHONY: distclean
distclean:
	rm -rf bin-* obj-*
