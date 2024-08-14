#GCC_SHARED = $(BASE)/usr/lib/libgcc_s.so.1 $(BASE)/usr/lib/libgcc_s.so

#CRTS  = $(BASE)/lib/crt0.o $(BASE)/lib/crti.o $(BASE)/lib/crtn.o

#LC = $(BASE)/lib/libc.so $(GCC_SHARED)

.PHONY: all
all:
	@make -C kernel
	@make -C modules
	@make -C libc
	@make -C lib
	@make -C ld
	@make -C userland

.PHONY: clean
clean:
	@make -C kernel clean
	@make -C modules clean
	@make -C libc clean
	@make -C lib clean
	@make -C ld clean
	@make -C userland clean
