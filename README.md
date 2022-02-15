# RAT (is an Analysis Tool), Watchman Edition
[![Documentation Status](https://readthedocs.org/projects/ratpac-watchman/badge/?version=latest)](https://ratpac-watchman.readthedocs.io/en/latest/?badge=latest)
[![Docker Cloud Build Status](https://img.shields.io/docker/cloud/build/aitwatchman/ratpac)](https://hub.docker.com/r/aitwatchman/ratpac)

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


## Installation

Installation requires [ROOT](https://root.cern.ch), [Geant4](https://geant4.web.cern.ch/), and [cmake 3.11+](https://cmake.org/)

Install using cmake

    $ git clone https://github.com/AIT-WATCHMAN/rat-pac
    $ cd rat-pac 
    $ cmake . -Bbuild
    $ cmake --build build -- -j$(nproc)

If you want to install the code, just add

    $ cmake --build build . --target install -j$(nproc)

## Docker

We have a docker image with rat pre-installed at
[aitwatchman/ratpac](https://hub.docker.com/r/aitwatchman/ratpac)

In this current implementation, any branched merged into the master version of
the github will automatically be uploaded to the docker image.

The correct work procedure is to fork the repository to your personal
directory. Make required changes. Make pull request by assigning an approved
custodian.

Running docker on linux
```bash
docker run --rm -it \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -e DISPLAY
  aitwatchman/ratpac:latest
```
