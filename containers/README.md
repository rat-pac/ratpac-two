Docker and Singularity Images
=============================

The images are split into `Dockerfile.base` which contains the base operating system,
ROOT, and Geant-4, and `Dockerfile` that pulls from base and only compiles ratpac.

The docker images are stored on `hub.docker.com` at ratpac/ratpac-two

## Singularity
Singularity images are built by bootstrapping from docker.
```bash
singularity build ratpac.sif docker://ratpac/ratpac-two:latest
```
This can be done with either ratpac-two:latest or ratpac-two:base.
