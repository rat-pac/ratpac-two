Installation
------------
Prerequisites
`````````````
These software packages should be installed in the order presented before you
attempt to build Ratpac.

 * `Python 3+ with development headers <https://www.python.org/>`_ - 
 * `ROOT 6.25+ <http://root.cern.ch/drupal/content/downloading-root>`_ 
 * `Geant-4 11.0+ <http://geant4.web.cern.ch/geant4/support/download.shtml>`_
 * `CMake 3.22+ <http://www.cmake.org>`_


Build Steps
```````````
A helper package exists at `ratpac-setup
<https://github.com/rat-pac/ratpac-setup>`_ to help with the installation of
Ratpac and its dependencies. This package is recommended for general installation
and will install a local version of ROOT and Geant4.

Containers
``````````
Ratpac can be built in a container using the provided Dockerfile. The container
is based on the `Ubuntu 20.04 image <https://hub.docker.com/_/ubuntu>`_ and
includes all of the dependencies required to build Ratpac. The container can be
built using the following command:

.. code-block:: bash

    docker build -f Dockerfile.base -t ratpac/ratpac-two:base .
    docker build -f Dockerfile -t ratpac/ratpac-two:latest .

The first command will build the base image which only includes the dependencies
but not the Ratpac source code. The second command will build the Ratpac image
which includes the Ratpac source code and uses ratpac/ratpac-two:base.

A Singularity (Apptainer) image can be built from the Docker image using the
following command:

.. code-block:: bash

    singularity build ratpac-two.sif docker-daemon://ratpac/ratpac-two:latest

Or if you are planning to develop Ratpac, you can build the image only from the
base docker image and then compile Ratpac inside the container.

.. code-block:: bash

    singularity build ratpac-two.sif docker-daemon://ratpac/ratpac-two:base
