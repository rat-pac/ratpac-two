PMT Simulation
--------------
(Need to update this section, it is mostly out of date)

RAT uses a custom PMT simulation extracted from GLG4Sim.

----------------

Q/T Response
````````````
Gsim checks the database for single photoelectron charge and transit time PDFs
automatically for PMT models that are added to the geometry. These PDFs are
stored in tables named ``PMTCHARGE`` and ``PMTTRANSIT`` respectively, where the
index corresponds to a ``pmt_model`` field used in ``GEO`` tables. These PDFs
are sampled whenever a photon is absorbed by the photocathode to create a
realistic Q/T response automatically for PMTs independent of any DAQ processor.
If no tables are defined for a ``pmt_model`` the time defaults to approximately
zero spread from photoelectron absorption time and the charge defaults to a
phenomenological model used by MiniCLEAN.

``PMTCHARGE`` fields:
 * ``charge`` - "x" values of the charge PDF (arbitrary units)
 * ``charge_prob`` - "y" values of the charge PDF (will be normalized)
 
 
``PMTTRANSIT`` fields:
 * ``cable_delay`` - constant offset applied to all PMTs of this model (nanoseconds)
 * ``time`` - "x" values of the time PDF (nanoseconds)
 * ``time_prob`` - "y" values of the time PDF (will be normalized)

----------------

Dark Current
````````````

PMTs have an intrinsic noise rate, or "dark current", which results from
thermal excitation at the single electron level.  These thermal electrons can
exactly mimic a photoelectron from the PMT's photocathode and, thus, noise hits
cannot be distinguished from 'true' hits caused by incident photons.

On the upside, this makes the noise hits fairly simple to simulate: we can draw
from the same charge spectrum as is used for regular PMT hits.  The only
subtleties are in the timing distribution of the hits, and the rates at which
noise is generated.  This document describes the inclusion of noise hits in
RAT.

There old noise processor (source:rat/src/daq/NoiseProc.cc) had several
problems with the implementation, in particular the hits were incorrectly
distributed in time (generated for one sampling window width from the start of
the simulated event, therefore not extending throughout the event window), and
noise was defined in terms of a number of hits per event instead of a rate
(which is what we are more likely to measure).  In addition, under the
principle of "apathy should lead to the most realistic simulation possible" (-
Dr Stanley Seibert), it was decided to incorporate noise hits into the default
event simulation, rather than retaining the optional processor.  This avoids
the possibility of noise not being included either through forgetfulness, or
because the noise processor was run in the wrong order, for example after the
DAQ processors.  As a result, running RAT will now include PMT noise hits by
default, unless they are switched off.  Details on how to do so follow.

Control
'''''''
There are three options for the inclusion of noise, as follows:

0: No noise hits simulated.
1: Average noise rate read from 'noise_rate' in DAQ.ratdb.
2: Tube-by-tube noise rates read from 'PMTnoise' in DAQ.ratdb.

These options are controlled by the use of the 'noise_flag', in the DAQ.ratdb
file.  This flag can be include' in RAT macros as follows::

    /rat/db/set DAQ noise_flag x

where x = 0, 1, or 2, depending on the noise model chosen.

The noise is included in the simulation after the physics event has been
propagated (all particles followed to extinction, and PMT hits recorded) but
before the DAQ, which runs as a separate processor. All noise hits are flagged
with the 'isNoise' bit in the MCPhoton class (set to 1 for noise hits, and 0
for regular PMT hits).

Timing Distribution
'''''''''''''''''''
Noise hits are generated uniformly in time, throughout a pre-defined 'noise
window'.  The DAQ records data beginning from a predefined time before a
trigger.  This time period (pretrigger time) is given by a fixed number
(currently 1350) of sampling windows (4ns each).  We want noise to be simulated
throughout any recorded waveform.  The noise window therefore begins at a time
before the first photon hit given by this pretrigger time.  To allow for a
noise hit in coincidence with the last recorded photon to cause a trigger, the
noise window continues until the width of the event window, plus the width of a
single discriminator pulse, past the last photon hit time.

Speed and file size Comparison
''''''''''''''''''''''''''''''
To determine the effect of including noise in the default simulation, I
generated 1000 isotropic 20keV electrons at the centre of the detector using
each noise model.  Both the average and the individual tube noise rates were
set to 2000Hz, to emphasise any impact of including noise hits (default is
500Hz).  The results, in terms of CPU usage (output file size), were as
follows:

Noise model 0: 2280.91 s (46M)
Noise model 1: 2285.77 s (48M)
Noise model 2: 2341.45 s (48M)

So including noise in the simulation increases the processing time by 0.2%, and
simulating noise tube-by-tube increases it by a further 2.4%.

The file size increased by ~5% when noise was included in the simulation.

Parameters
''''''''''
All are stored in DAQ.ratdb

noise_rate: 500.0d, // The mean noise rate across all PMTs, in Hz
PMTnoise: [], // an array with 92 entries: individual noise rates per PMT, in Hz
noise_flag: 0, // the flag to determine which noise model you use (default is to turn noise off completely)

----------------

PMT Afterpulsing
`````````````````

Details of PMT afterpulsing

----------------

PMT Pulse Generation
````````````````````

Details of the PMT pulse generation here.

----------------

PMT Encapsulation
`````````````````

PMT encapsulation is used for several reasons, such as to ensure compatability with multiple detection media (e.g. air, water, doped water).

The encapsulation code was originally created for the BUTTON experiment, in which each of the 96 PMTs used are enclosed by two hemisphere domes that are sealed together by metal flanges and bolts.

The encapsulation code structure is based off the PMT construction structure, in which a instance is initialised depending on the construction type given.

When enabled, the encapsulation object is created first, followed the pmt object. The PMT is then placed inside the encapsulation before itself is placed in the mother volume given.

Enabling Encapsulation
''''''''''
Encapsulation by default is turned off. 
In a .geo file, it can be enabled by adding the following line inside the ``inner_pmts`` index entry: ::

    encapsulation: 1,

With 0 being off.
It can also be added in a macro with: ::

    /rat/db/set GEO[inner_pmts] encapsulation 1

The other line that must be included inside the ``inner_pmts`` index entry is the model type: ::

    encapsulation_model: "modelname",

Where "modelname" must match an index entry name in ``ENCAPSULATION.ratdb``.

Encapsulation model information
''''''''''
Encapsulation models need to be added to ``ENCAPSULATION.ratdb``, which is loacted in ``ratpac/ratdb``. 
A entry can be called by using the ``encapsulation_model:`` command as mentioned above.
Each entry provides all the important information that is needed to create the encapsulation objects:

* Construction type
* Enable and disable additional objects
* Object dimensions and materials
* Off-centre object placements

The construction type is needed to ensure the correct encapsulation construction is loaded. This represents the general shape of the encapsulation used.
For any materials used, their properties should be defined in ``MATERIALS.ratdb`` and ``OPTICS.ratdb``.
Any values given such as dimensions and positions should be given in mm.
Multiple entries can use the same construction type, but can vary on the objects and object properties used.

Adding a new Encapsulation construction
''''''''''
Initially, the only encapsulation construction is the "hemisphere" type, which encapsulates the PMT inside two hemispheres.
An inner volume is then created in which the PMT can be placed.

When creating a new construction model (e.g. a box), the .cc file should contain three main functions:

* An initial function that is called to create an instance with the information from the given ``ENCAPSULATION.ratdb`` entry.
* A build function that creates and returns the encapsulation.
* A placement function.

A new encapsulation construction should make the build as customisable as possible.
The important object information such as those stated above should be called from an ``ENCAPSULATION.ratdb`` entry.

To use a new construction type, the option must be added to ``PMTEncapsulationConstruction.cc``.
This file uses the construction type that is given in the called ``ENCAPSULATION.ratdb`` entry to initiate the associated encapsulation construction.
For a working example please see ``HemisphereEncapsulation.cc/hh`` which uses the "hemisphere" construction type.

Placing PMT
''''''''''
If encapsulation is used, then is possible that the medium inside the encapsuation is different to the mother volume medium it would be placed in without encapsulation on.
This can be change in ``PMTFactoryBase.cc`` to ensure that the correct mother volume is used for the placement. If using the visualiser, the scene tree is useful to see if the PMT has been placed inside the correct volume.


PMT Offset
''''''''''
The encapsulation is placed using the PMT position(s) and direction(s) given, this means that by default the PMT is placed in the centre of the encapsulation. 
An offset can be given in the ``ENCAPSULATION.ratdb`` entry so that the PMT is placed off-centre inside the encapsulation. This currently works for z-axis offsets (i.e move the PMT forwards/backwards). 

----------------