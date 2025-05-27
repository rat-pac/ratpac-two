.. _photodetector_simulation:

Photodetector Simulation
------------------------

Several different photodetectors are available in `ratpac-two`.

----------------

.. _pmt_simulation:

PMTs
====

`ratpac-two` uses a custom PMT simulation.

For each photon that enters a PMT volume, there is a probability of creating a photoelectron (PE), which is equal to the product of the quantum efficiency, an efficiency correction factor, and an optical correction factor that accounts for the polarization and incidence angle of photon (relative to the photocathode). The bulk of the code that calculates the optical correction factor and applies the overall efficiency can be found in ``src/physics/src/GLG4PMTOpticalModel.cc``.

The quantum efficiency for each PMT is defined in ``OPTICS_Photocathode.ratdb`` and is specified for each PMT in ``PMT.ratdb`` using the index ``photocathode_surface``. The efficiency correction factor can be specified in two different ways. First, a field in ``PMT.ratdb`` called ``efficiency_correction`` can be set to scale the efficiency of all PMTs of the specified type. The value of this scaling defaults to one. Second, an array of correction factors, called ``efficiency_corr``, can be set in the ``PMTINFO.ratdb`` file for every individual PMT. This will scale the efficiency for each PMT separately.

Once it has been determined that a photoelectron should be created for a PMT, it is added to the ``HitPMTCollection``. For each hit PMT, a ``DS::MCPMT`` object is created. The PMT may have detected more than one PE, in which and the ``DS::MCPhoton`` class (which can be accessed by the ``GetMCPhoton()`` method in the ``MCPMT`` class) keeps track of information for each ``MCPE``.

The ``DS::PMT`` objects are created only after a trigger event has been issued (see DAQ documentation :ref:`daq_processors`) and can include effects from the ``DAQ`` and ``trigger``. Similarly, the ``DS::DigitPMT`` objects are created by the waveform analysis processors, which run over digitized waveforms, as described in :ref:`waveform_analysis`.


.. _pmt_geometries:

Geometries
''''''''''

Describe the different PMT geometries.

Toroidal
########

Describe the toroidal PMTs.

Revolution
##########

Describe the revolution PMTs.

Cubic
#####

Describe the cubic PMTs.

Cylindrical
###########

Describe the cylindrical PMTs.

----------------

.. _pmt_response:

Charge and Time Response
''''''''''''''''''''''''
The PMT charge and time response is set in ``Gsim``, which checks the database for single photoelectron charge and transit time PDFs automatically for PMT models that are added to the geometry. These PDFs are stored in tables named ``PMTCHARGE`` and ``PMTTRANSIT`` respectively, where the index corresponds to a ``pmt_model`` field used in ``GEO`` tables. These PDFs are sampled whenever a photon is absorbed by the photocathode to create realistic charge and time response automatically for PMTs independent of any DAQ processor. If no tables are defined for a ``pmt_model`` the time defaults to approximately zero spread from photoelectron absorption time and the charge defaults to the model for the large-area Hamamatsu r11780 12-inch PMT (arbitrary choice).

``PMTCHARGE`` fields:
 * ``charge`` - "x" values of the charge PDF (arbitrary units)
 * ``charge_prob`` - "y" values of the charge PDF (will be normalized)
 
 
``PMTTRANSIT`` fields:
 * ``cable_delay`` - constant offset applied to all PMTs of this model (nanoseconds)
 * ``time`` - "x" values of the time PDF (nanoseconds)
 * ``time_prob`` - "y" values of the time PDF (will be normalized)

Note that this ``cable_delay`` is applied to every single PE and can be used to shift the timing for all hits by a single value. This is separate from the per-PMT cable delays that can be applied, as described in :ref:`channel_status`.

----------------

.. _dark_noise:

Dark Noise Processor
''''''''''''''''''''

PMTs have an intrinsic noise rate, or "dark current", which results from thermal excitation at the single electron level.  These thermal electrons can exactly mimic a photoelectron from the PMT's photocathode and, thus, noise hits cannot be distinguished from 'true' hits caused by incident photons. The dark-noise simulation is performed by the ``NoiseProc`` processor, with controllable parameters set in the ``NoiseProc.ratdb`` table.

The noise is included in the simulation after the physics event has been propagated (all particles followed to extinction, and PMT hits recorded). In order to include the PMT noise hits into the trigger simulation, the noise processor should be called prior to the DAQ processor. All noise hits are flagged and can be selected using the ``isDarkHit`` method in the ``MCPhoton`` class.

.. _noise_control:

Control
#######
The ``noise_flag`` in the ``NoiseProc.ratdb`` table sets the way in which the dark noise is simulated.

0: Global rate -- all PMTs in the detector have the same dark-rate. The value of the global rate is set by the ``default_noise_rate`` parameter. The default behavior of the noise processor is to use this global setting with a default rate of 1 kHz.

1: Per PMT model rate -- all PMTs of the same type have the same rate. This may be useful if the detector has different types of PMTs. In this case the noise rate is read from the ``PMT.ratdb`` table for the appropriate model.

2: Per PMT rate -- every PMT in the detector have a different noise rate. This may be useful if the noise rates have been measured for every PMT and one wants to simulate these specific per-PMT rates. In this case the noise rates are read from the relevant ``PMTINFO.ratdb`` file, where an array called ``noise_rate`` can be defined. As an example, from the macro we can change the noise simulation to per PMT-model rates and change the rate for a specified model::

        /rat/db/set NOISEPROC noise_flag 1
        /rat/db/set PMT[r14688] noise_rate 5000.0

These parameters can also be set using ``procset``. For example, to set the ``default_noise_rate`` we would do::

/rat/proc noise
/rat/procset rate 5000.0

Command::

/rat/proc noise

Parameters::

/rat/procset flag [value]

* [value] int - sets the value of the ``noise_flag``.

::

/rat/procset rate [value]

* [value] double - sets the value of the ``default_noise_rate``.

::

/rat/procset lookback [value]
/rat/procset lookforward [value]
/rat/procset maxtime [value]

* [values] doubles - sets the relevant parameters for the noise window, described further below. 

.. _noise_timing_and_charge:

Timing and charge distributions
###############################

Noise hits are generated uniformly in time, throughout a window defined by the ``noise_lookback`` and ``noise_lookforward`` parameters in the ``NoiseProc.ratdb`` table. The parameters are set by default to 1000 ns each, and are typically centered around the first true PMT hit-time in the event (in the case that there are no hits, the window is centered around zero). The value of ``noise_maxtime`` sets the timing cut-off for generating noise-hits in the case of long-lived particles in the MC.

The PMT charge distribution is sampled assuming the normal SPE charge distribution, as described in :ref:`pmt_response`.

----------------

.. _pmt_afterpulsing:

PMT Afterpulsing Processor
''''''''''''''''''''''''''

Details of PMT afterpulsing

----------------

.. _pmt_pulse:

PMT Pulse Generation
''''''''''''''''''''

Details of the PMT pulse generation here.

----------------

PMT Encapsulation
'''''''''''''''''

PMT encapsulation is used for several reasons, such as to ensure compatibility with multiple detection media (e.g. air, water, doped water).

The encapsulation code was originally created for the BUTTON experiment, in which each of the 96 PMTs used are enclosed by two hemisphere domes that are sealed together by metal flanges and bolts.

The encapsulation code structure is based off the PMT construction structure, in which a instance is initialized depending on the construction type given.

When enabled, the encapsulation object is created first, followed the pmt object. The PMT is then placed inside the encapsulation before itself is placed in the mother volume given.

Enabling Encapsulation
######################
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
###############################
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
#######################################
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
###########
If encapsulation is used, then is possible that the medium inside the encapsulation is different to the mother volume medium it would be placed in without encapsulation on.
This can be change in ``PMTFactoryBase.cc`` to ensure that the correct mother volume is used for the placement. If using the visualizer, the scene tree is useful to see if the PMT has been placed inside the correct volume.


PMT Offset
##########
The encapsulation is placed using the PMT position(s) and direction(s) given, this means that by default the PMT is placed in the center of the encapsulation.
An offset can be given in the ``ENCAPSULATION.ratdb`` entry so that the PMT is placed off-centre inside the encapsulation. This currently works for z-axis offsets (i.e move the PMT forwards/backwards).

----------------

PMT Concentrators
'''''''''''''''''

Document the PMT concentrators.

----------------

Magnetic Compensation
'''''''''''''''''''''

Technically there is code in ``geo/src/pmt/PMTFactoryBase.cc`` that can be enabled to attempt to change the PMT efficiency based on a specified external magnetic field; however, this is not supported code and is by default turned off.

LAPPDs
======

Describe LAPPDs here.

----------------

Optical Fibers
==============

Descibe Liquid-O style fiber simulations here.

----------------

.. _channel_status:

Channel Status
==============

Details of the channel status here.

