include ../libc/GNUmakefile

../base/lib/ld.so: linker.c ../base/lib/libc.a | $(LC)
	@echo -e 'CC' $@
	@$(CC) -g -static -Wl,-static $(CFLAGS) -z max-page-size=0x1000 -o $@ -Os -T link.ld $<
