CC ?= cc
CFLAGS ?= -O2 -g
WARN = -Wall -Wextra -Wno-misleading-indentation
COMMON = -std=c99 -D_POSIX_C_SOURCE=200809L -Isrc -Ivendor -Ivendor/expat/lib -DXML_UNICODE -DXML_STATIC -DHAVE_EXPAT_CONFIG_H -DMINIZ_NO_TIME -DMINIZ_NO_ZLIB_APIS -DMINIZ_NO_DEFLATE_APIS
SOURCES = src/main.c src/vm.c src/context.c src/ndless_compat.c vendor/miniz.c vendor/expat/lib/xmlparse.c vendor/expat/lib/xmlrole.c vendor/expat/lib/xmltok.c
HOST_SOURCES = $(SOURCES) vendor/expat/lib/random_dev_urandom.c
NDLESS_CC ?= nspire-gcc
NDLESS_LD ?= nspire-ld

.PHONY: all host ndless test clean
all: host
host: build/nspire-jvm
build/nspire-jvm build/ndless/src/vm.o: src/enums.inc src/builder.inc src/annotations.inc src/character.inc src/regex.inc src/parse_number.inc src/environment.inc vendor/openjdk17-casing/character.inc
build/nspire-jvm: $(HOST_SOURCES) src/vm.h src/context.h src/threads.inc src/unsafe.inc src/loader.inc src/identifiers.inc src/case.inc vendor/openjdk17-casing/data.inc src/indy.inc src/lambda.inc src/format.inc src/split.inc src/search.inc src/reflection.inc src/xml.inc src/expat_config.h $(wildcard vendor/expat/lib/*.h) vendor/miniz.h Makefile
	mkdir -p build
	$(CC) $(COMMON) $(WARN) $(CFLAGS) $(HOST_SOURCES) -lm -o $@

ndless: dist/nspire-jvm.tns
build/ndless/%.o: %.c src/vm.h src/context.h src/threads.inc src/unsafe.inc src/loader.inc src/identifiers.inc src/case.inc vendor/openjdk17-casing/data.inc src/indy.inc src/lambda.inc src/format.inc src/split.inc src/search.inc src/reflection.inc src/xml.inc src/expat_config.h $(wildcard vendor/expat/lib/*.h) Makefile
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
