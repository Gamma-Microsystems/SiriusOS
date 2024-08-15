.PHONY: all
all:
	@make -C kernel
	@make -C modules all
	@make -C ld
	@make -C lib
	@make -C userland

.PHONY: clean
clean:
	@make -C kernel clean
	@make -C modules clean
	@make -C ld clean
	@make -C lib clean
	@make -C userland clean
