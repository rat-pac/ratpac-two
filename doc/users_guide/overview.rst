Overview
--------
.. role:: bash(code)
   :language: bash

All source code, documentation, and examples can be found at the following locations:

* `github.com/rat-pac/ratpac-two.git <https://github.com/rat-pac/ratpac-two.git>`_
* `ratpac.readthedocs.io <https://ratpac.readthedocs.io>`_
* `hub.docker.com/repositories/ratpac <https://hub.docker.com/repositories/ratpac>`_
* `github.com/rat-pac/ratpac-setup.git <https://github.com/rat-pac/ratpac-setup.git>`_

Goals
`````
Ratpac is intended to be a framework that combines both Monte Carlo simulation with event-based analysis tasks, like
reconstruction. The primary goals are:

* Make it easy to analyze Monte Carlo-generated events as well as data from disk using the same software with only a few
  command changes. Even in the proposal/R&D phase, where there is no real data, this is still useful for dumping Monte
  Carlo events to disk to be analyzed by another job. When there is real data, being able to do the analysis with the
  same code path as was used on Monte Carlo is very reassuring.
* Allow for a modular, user-controlled analysis of events. This includes allowing the user to selected which analysis
  tasks to perform (different fitters, pruning unneeded data from the event data structure, etc.). It should also be
  relatively straightforward for users to introduce their own code into the analysis process.
* Separate analysis into small tasks which can be developed asynchronously by different people, yet integrated with
  minimal (or perhaps zero) pain.
* Integrate into existing Geant4 efforts with a minimum of code duplication. 

Ratpac-two is a refactor of ratpac which makes the necessary changes to compile with modern versions of GCC and is
compatible with the latest versions of Geant4 and ROOT. This version is not backwards compatible with the previous
version.

Design
``````
The overall design of RAT is much like SNOMAN (the original SNO Monte Carlo package): View analysis as a big loop,
iterated through once for each event. The body of the loop is assembled by the user in their macro file as a list of
"processors." A processor is a self-contained module that takes an event as input and does some work on the event,
possibly altering the contents of the event data structure. A fitting processor would add reconstruction information to
the event structure, and an I/O processor would write the event to disk, but leave the data structure in memory
unchanged. 

Processors can read and modify existing events, but where do the events originally come from? This is the job of event
"producers." A producer can be something like a Monte Carlo simulation. We might decide to simulate the following
reaction:

.. math:: 
  \bar{\nu}_e + p \rightarrow n + e^+

Given the delay between the observation of the positron and the neutron, this single physics event will be detected as
(at least) two separate detector events. The job of the event producer is ultimately to generate physics events and hand
them over to the processors selected by the user, one event at a time. (Other processors may convert the physics event
into detector events.  There is a place in the data structure to put multiple detector events.)

Other event generators are possible. Generators which read events from disk or over the network would function in a
similar manner, creating event data structures in memory and handing them to the event processors one by one.

The event-sequential nature of this computation model is both powerful and simple, but can be awkward for certain kinds
of analyses. Multi-pass analyses, which must go through a list of events more than once, can be implemented in Ratpac
without much difficulty as long as each pass is sequential. Time-correlation processors can also be implemented if the
processor buffers some data internally. A processor that needs full random access to the event stream cannot be
implemented efficiently in Ratpac. The events should be dumped to disk and analyzed in some other program.

Architecture
````````````
Ratpac-two is structured in a way to be fully extensible without requiring recompilation or modification of the base
code. The core structure is designed to work with a CMake build system and compiles to a shared library which can be
loaded via :bash:`find_package(Ratpac)`. For large scale application it is suggested that the user define a complete
experiment package which includes the Ratpac library as a dependency. An example of such implementation can be found at

* `github.com/rat-pac/RatpacExperiment <https://github.com/rat-pac/RatpacExperiment>`_

For smaller scale applications, the user can simply use geometry and configuration files, which are loaded at runtime,
to define and experiment and run the simulation with the stand-alone executable---:bash:`rat`---along with any user defined
macro files.

Relationships with Other Software
`````````````````````````````````
Ratpac makes use of several other software packages to do the heavy-lifting:

* CLHEP --- CLHEP is a library containing classes useful to physics software, such as 3D vectors and random number
  generators. It is also used by Geant4.
* Geant4 --- While Geant4 is intended to simulate particle interactions in detectors, RAT does not use it directly for
  that purpose, delegating that to GLG4sim. Instead, RAT makes direct use of the Geant4 command interpreter to provide a
  language for both interactive use and executing macro files. RAT also uses the Geant4 compilation system and
  makefiles.
* ROOT --- ROOT is used to load and save objects to/from disk and over the network.
* GLG4sim --- This package is a generalized version of the KamLAND Monte Carlo, intended to simulate KamLAND-like neutrino
  experiments with Geant4. The version of GLG4sim used by Ratpac is heavily modified from the original.

Modular Software Extensions
```````````````````````````
Additional software packages can be used to extend the functionality of Ratpac. These packages are not required to
compile Ratpac, but can be included at compile time to enable external physics generators and analysis capabilities.

* Cry --- Cosmic-ray particle shower generator
* Chroma --- Optical photon propagation and detection
* Tensorflow / cppflow --- Enable tensorflow model evaluation event-by-event during simulations.
* PyTorch --- Enable PyTorch model evaluation event-by-event during simulations.
