# RAT (is an Analysis Tool)
[![Documentation Status](https://readthedocs.org/projects/ratpac-watchman/badge/?version=latest)](https://ratpac-watchman.readthedocs.io/en/latest/?badge=latest)
[![Docker Cloud Build Status](https://img.shields.io/docker/cloud/build/aitwatchman/ratpac)](https://hub.docker.com/r/aitwatchman/ratpac)


## Installation

Installation requires [ROOT 6.25+](https://root.cern.ch), [Geant4 11.0+](https://geant4.web.cern.ch/), and [cmake 3.22+](https://cmake.org/)

Install using cmake

    $ cmake . -Bbuild
    $ cmake --build build -- -j$(nproc)

If you want to install the code, just add

    $ cmake --build build . --target install -j$(nproc)

## About
RAT is a simulation and analysis package built with GEANT4, ROOT, and C++,
originally developed by S. Seibert for the Braidwood Collaboration. Versions
of RAT are now being used and developed by several particle physics
experiments.

RAT combines simulation and analysis into a single framework, which allows
analysis code to trivially access the same detector geometry and physics
parameters used in the detailed simulation.

RAT follows the "AMARA" principle: As Microphysical as Reasonably Achievable.
Each and every photon is tracked through an arbitrarily detailed detector
geometry, using standard GEANT4 or custom physics processes. PMTs are fully
modeled, and detected photons may be propagated to a simulation of front-end
electronics and DAQ.

This generic version is intended as a starting point for collaborations
looking for an easy-to-learn, extensible detector simulation and analysis
package that works out of the box. Once acquainted with RAT, it is easy to
customize the geometry elements, physics details, data structure, analysis
tools, etc., to suit your experiment's needs.
