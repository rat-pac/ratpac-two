# Makefile for the lazy

build_directory := build
install_directory := $(PWD)/install
BUILDTYPE ?= RelWithDebInfo # default build type

all:
	ln -sf $(PWD)/cformat.sh $(shell git rev-parse --git-common-dir)/hooks/pre-commit
	cmake . -B$(build_directory) -DCMAKE_BUILD_TYPE=$(BUILDTYPE) -DCMAKE_INSTALL_PREFIX=$(install_directory)
	cmake --build $(build_directory) -- $(MAKEFLAGS)
	cmake --install $(build_directory) | grep -v "Up-to-date"

# Convenient build-type targets (reuse same build dir)
.PHONY: debug release relwithdebinfo minsizerel
debug: BUILDTYPE=Debug
debug: all

release: BUILDTYPE=Release
release: all

relwithdebinfo: BUILDTYPE=RelWithDebInfo
relwithdebinfo: all

minsizerel: BUILDTYPE=MinSizeRel
minsizerel: all

clean:
	rm -rf $(build_directory) $(install_directory) ratpac.sh ratpac.csh RatpacConfig.cmake
