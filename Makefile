CC ?= cc
CFLAGS ?= -O2 -g
WARN = -Wall -Wextra -Wno-misleading-indentation
COMMON = -std=c99 -D_POSIX_C_SOURCE=200809L -Isrc -Ivendor -DMINIZ_NO_TIME -DMINIZ_NO_ZLIB_APIS -DMINIZ_NO_DEFLATE_APIS
SOURCES = src/main.c src/vm.c src/context.c src/ndless_compat.c vendor/miniz.c
NDLESS_CC ?= nspire-gcc
NDLESS_LD ?= nspire-ld

.PHONY: all host ndless test clean
all: host
host: build/nspire-jvm
build/nspire-jvm: $(SOURCES) src/vm.h src/context.h src/threads.inc src/unsafe.inc src/loader.inc src/identifiers.inc src/indy.inc src/format.inc src/reflection.inc vendor/miniz.h Makefile
	mkdir -p build
	$(CC) $(COMMON) $(WARN) $(CFLAGS) $(SOURCES) -lm -o $@

ndless: dist/nspire-jvm.tns
build/ndless/%.o: %.c src/vm.h src/context.h src/threads.inc src/unsafe.inc src/loader.inc src/identifiers.inc src/indy.inc src/format.inc src/reflection.inc Makefile
	mkdir -p $(dir $@)
	$(NDLESS_CC) $(COMMON) $(WARN) -Os -marm -ffunction-sections -fdata-sections -c $< -o $@
build/ndless/%.o: %.S
	mkdir -p $(dir $@)
	$(NDLESS_CC) -marm -c $< -o $@
build/nspire-jvm.elf: $(SOURCES:%.c=build/ndless/%.o) build/ndless/src/context_arm.o
	$(NDLESS_LD) $^ -Wl,--nspireio,--gc-sections -lm -o $@
dist/nspire-jvm.tns: build/nspire-jvm.elf
	mkdir -p dist
	genzehn --input $< --output $@ --name "Nspire JVM"

test: host
	python3 tools/test.py --vm build/nspire-jvm
clean:
	rm -rf build
