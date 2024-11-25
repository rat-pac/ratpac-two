# Ratpac-two

[![Documentation Status](https://readthedocs.org/projects/ratpac/badge/?version=latest)](https://ratpac.readthedocs.io/en/latest/?badge=latest)

[![Build RatpacExperiment](https://github.com/rat-pac/ratpac-two/actions/workflows/build-experiment.yml/badge.svg)](https://github.com/rat-pac/ratpac-two/actions/workflows/build-experiment.yml)

[![clang-format Check](https://github.com/rat-pac/ratpac-two/actions/workflows/check-format.yml/badge.svg)](https://github.com/rat-pac/ratpac-two/actions/workflows/check-format.yml)

[![Build Docker Image](https://github.com/rat-pac/ratpac-two/actions/workflows/latest-container.yml/badge.svg?branch=main&event=deployment)](https://github.com/rat-pac/ratpac-two/actions/workflows/latest-container.yml)

[ratpac.readthedocs.io](ratpac.readthedocs.io)

## Quick Start with containers

The easiest way to get started with ratpac-two is via containers. The latest
builds can be found on
[dockerhub](https://hub.docker.com/r/ratpac/ratpac-two/tags) or on
[ghcr.io](https://github.com/rat-pac/ratpac-two/pkgs/container/ratpac-two).

On HPC platforms, apptainer/singularity containers are a better option than
docker containers. To create these, run

```sh
apptainer pull ratpac2.sif docker://ratpac/ratpac-two:main
```

To enter the container:

```sh
apptainer run [-B /disks/to/mount] ratpac2.sif
```

## Installation

Installation requires [ROOT 6.25+](https://root.cern.ch),
[Geant4 11.0+](https://geant4.web.cern.ch/), and [cmake 3.22+](https://cmake.org/)

For development, the following are also required:

- clang-format: major version 14. _other major versions are known to produce
different outputs that will result in failed checks_

The most well-supported way of installing ratpac-two is via the [ratpac-setup
script](https://github.com/rat-pac/ratpac-setup). Follow the instruction will
produce a standalone directory that includes all dependencies. You can also
check this repo for the best-tested minor versions of each dependency.

A convenience Makefile exists to automate the above process, simply type `make`.

Install using cmake:

``` sh
    cmake . -Bbuild
    cmake --build build -- -j$(nproc)
```

If you want to install the code, just add

``` sh
    cmake --build build . --target install -j$(nproc)
```

## Usage

Ratpac-two compiles as a library which can be extended for specific experiment
use cases. The library can be accessed through CMake using
`find_package(Ratpac)`.

A stand-alone executable is included with the library that can be run on simple
experimental geometries and test suites. The intent is for users to compile
against the ratpac library, and any specific additions to the C++ framework
should exist outside of the main repository.

## About

Ratpac-two is a refactor of ratpac which makes the necessary changes to compile
with modern versions of GCC and is compatible with the latest Geant4 and ROOT
versions. This version of Ratpac is not backwards compatible with the previous
version.

Ratpac is a simulation and analysis package built with GEANT4, ROOT, and C++,
originally developed by S. Seibert for the Braidwood Collaboration. Versions of
Ratpac are now being used and developed by several particle physics
experiments.

Ratpac combines simulation and analysis into a single framework, which allows
analysis code to trivially access the same detector geometry and physics
parameters used in the detailed simulation.

Ratpac follows the "AMARA" principle: As Microphysical as Reasonably
Achievable. Each and every photon is tracked through an arbitrarily detailed
detector geometry, using standard GEANT4 or custom physics processes. PMTs are
fully modeled, and detected photons may be propagated to a simulation of
front-end electronics and DAQ.

This generic version is intended as a starting point for collaborations looking
for an easy-to-learn, extensible detector simulation and analysis package that
works out of the box. Once acquainted with Ratpac, it is easy to customize the
geometry elements, physics details, data structure, analysis tools, etc., to
suit your experiment's needs.
