CC ?= cc
CFLAGS ?= -O2 -g
WARN = -Wall -Wextra -Wno-misleading-indentation
COMMON = -std=c99 -D_POSIX_C_SOURCE=200809L -Isrc -Ivendor -Ivendor/expat/lib -Ivendor/openjdk8-fdlibm/upstream -fno-strict-aliasing -ffp-contract=off -DXML_UNICODE -DXML_STATIC -DHAVE_EXPAT_CONFIG_H -DMINIZ_NO_TIME -DMINIZ_NO_ZLIB_APIS -DMINIZ_NO_DEFLATE_APIS
SOURCES = src/main.c src/vm.c src/context.c src/ndless_compat.c src/canonical.c src/strictmath_log.c src/strictmath_sqrt.c vendor/miniz.c vendor/expat/lib/xmlparse.c vendor/expat/lib/xmlrole.c vendor/expat/lib/xmltok.c
HOST_SOURCES = $(SOURCES) vendor/expat/lib/random_dev_urandom.c
HOST_HEADERS = $(wildcard src/*.h src/*.inc vendor/expat/lib/*.h vendor/openjdk8-fdlibm/upstream/*.h vendor/openjdk8-fdlibm/upstream/*.c vendor/openjdk8-fdlibm/generated/*.c) vendor/openjdk8-api/methods.inc vendor/openjdk17-casing/data.inc vendor/openjdk17-casing/character.inc vendor/openjdk8-file/canonicalize_md.c vendor/miniz.h
NDLESS_CC ?= nspire-gcc
NDLESS_LD ?= nspire-ld

.PHONY: all host ndless test clean
all: host
host: build/nspire-jvm
build/nspire-jvm build/ndless/src/vm.o: src/filesystem.inc src/descriptors.inc src/linkage.inc src/canonical.h src/time.inc
build/nspire-jvm build/ndless/src/vm.o: src/parse_number.inc
build/nspire-jvm build/ndless/src/vm.o: src/nio_files.inc
build/nspire-jvm build/ndless/src/vm.o: src/strictmath.h
build/nspire-jvm build/ndless/src/strictmath_log.o build/ndless/src/strictmath_sqrt.o: src/fdlibm_config.h src/strictmath.h $(wildcard vendor/openjdk8-fdlibm/upstream/*.h) vendor/openjdk8-fdlibm/upstream/e_log.c vendor/openjdk8-fdlibm/generated/e_sqrt.c
build/nspire-jvm build/ndless/src/canonical.o: src/canonical.h vendor/openjdk8-file/canonicalize_md.c
build/nspire-jvm build/ndless/src/vm.o: src/enums.inc src/builder.inc src/annotations.inc src/character.inc src/regex.inc src/parse_number.inc src/environment.inc src/output.inc src/methods.inc src/boxing.inc src/charset.inc vendor/openjdk8-api/methods.inc vendor/openjdk17-casing/character.inc
build/nspire-jvm: $(HOST_SOURCES) src/vm.h src/context.h src/threads.inc src/unsafe.inc src/loader.inc src/identifiers.inc src/case.inc vendor/openjdk17-casing/data.inc src/indy.inc src/lambda.inc src/format.inc src/split.inc src/search.inc src/reflection.inc src/xml.inc src/expat_config.h $(wildcard vendor/expat/lib/*.h) vendor/miniz.h Makefile
	mkdir -p build
	$(CC) $(COMMON) $(WARN) $(CFLAGS) $(HOST_SOURCES) -lm -o $@

build/nspire-jvm-asan: $(HOST_SOURCES) $(HOST_HEADERS) Makefile
	mkdir -p build
	$(CC) $(COMMON) $(WARN) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer $(HOST_SOURCES) -lm -o $@

build/file-lifetime: file-tests/lifetime.c $(filter-out src/main.c,$(HOST_SOURCES)) $(HOST_HEADERS) Makefile
	mkdir -p build
	$(CC) $(COMMON) $(WARN) $(CFLAGS) file-tests/lifetime.c $(filter-out src/main.c,$(HOST_SOURCES)) -lm -o $@

build/file-lifetime-asan: file-tests/lifetime.c $(filter-out src/main.c,$(HOST_SOURCES)) $(HOST_HEADERS) Makefile
	mkdir -p build
	$(CC) $(COMMON) $(WARN) -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer file-tests/lifetime.c $(filter-out src/main.c,$(HOST_SOURCES)) -lm -o $@

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
