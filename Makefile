# Makefile for the lazy

build_directory := build

all:
	cmake . -B$(build_directory)
	cmake --build $(build_directory) -- $(MAKEFLAGS)

clean:
	rm -rf $(build_directory) ratpac.sh ratpac.csh
