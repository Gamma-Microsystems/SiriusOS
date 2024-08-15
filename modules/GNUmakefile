include ../kernel/config.mk

CFILES_UNIVERSAL := $(shell find drv fs gpu net snd tests -type f -name '*.c')

ARCH_ENABLED_MODS = $(shell ../util/valid-modules.sh $(KARCH))
MODULES = $(patsubst %.c,../base/mod/%.ko,$(foreach mod,$(ARCH_ENABLED_MODS),$(mod).c))

.PHONY: all
all:
	../base/mod/%.ko: %.c | dirs
		@echo -e 'CC' $@
		@$(KCC) -c $(KCFLAGS) -fno-pie -mcmodel=large -o $@ $<
