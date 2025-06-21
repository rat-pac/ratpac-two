.. _output_processors:

Output Processors
`````````````````

.. _outroot:

outroot
=======
The ``OutROOT`` processor writes events to disk in the ROOT format.  The events are stored in a TTree object called "T" and the branch holding the events is called "ds". The full RAT data structure is described in :ref:`ratds`.

Command:
::

    /rat/proc outroot

Parameters:
::

    /rat/procset file "filename"

* filename (required, string) Sets output filename.  File will be deleted if it already exists.

.. _outntuple:

outntuple
=========
The ``OutNtuple`` processor writes events to disk as a flat ``ROOT`` Tree, and are thus much smaller and easier to work with than the files output by the outroot processor. There is also significant flexibility in what information is stored to the file, as detailed below. To run this processor and write the ntuple files out:

Command:
::

    /rat/proc outntuple

Parameters:
::

    /rat/procset file "filename"

    /rat/procset include_mcparticles 1
    /rat/procset include_tracking 1
    /rat/procset include_pmthits 1
    /rat/procset include_nestedtubehits 1
    /rat/procset include_untriggered_events 1
    /rat/procset include_mchits 1
    /rat/procset include_digitizerwaveforms 1
    /rat/procset include_digitizerhits 1
    /rat/procset include_digitizerfits 

* filename (required, string) Sets output filename.  File will be deleted if it already exists.
* include_* (optional, int) Sets whether the ntuple structure will be extended to include more variables, as detailed below. By default the following, based on the entries in IO.ratdb, the following are set to 0 by default: ``include_tracking``, ``include_mcparticles``, and ``include_digitizerwaveforms`` and the rest are set to 1 by default (i.e., the associated variables aare included in the ntuple file, as detailed below).

Similarly to the outroot file, one can pass the filename using the "-o" flag by running the macro as::

    rat example_macro.mac -o output.root

The ntuple contains flat ``TTrees`` called ``meta`` and ``output``, and optionally a tree called ``waveform``. The ``output`` branch contains data for each event, and is structured to hold both the "true" simulated quantities as well as the "detector" quantities, that are filled after a data acquisition processor are run. As an example, we see in the below table that the ``output`` branch contains entries for ``mcx``, the true position of the simulated event, as well as ``triggerTime``, the time of the event trigger. This can lead to confusion because not all simulated events will trigger the detector. Importantly, if we do not run any DAQ simulation, none of the events will be saved unless ``include_untriggered_events`` is set to true. If you're entirely interested in studying variables generated prior to the data aquisition (e.g., particle position, number of Cherenkov photons produced, the true number of detected photoelectrons, etc.), be certain to set ``include_untriggered_events`` to true in the macro. 

If we do run the DAQ simulation, only information for the triggered events will be saved to the ``output`` branch, unless ``include_untriggered_events`` is set to true, in which case all events will be saved to the ``output`` branch. Furthermore, for triggered event we should expect to have differences between the simulated and detector quantities. For example, the ``mcPMTID`` (only filled when ``mchits`` is set true), the true set of PMTs that detected light, the ``hitPMTID`` (only filled when ``pmthits`` is set true), the set of PMTs that detected light after applying the data aquisition simulation, and the ``digitPMTID``, the set of PMTs that detected light after applying the data aquisition and waveform analysis simulations, can all be different length vectors (the ``hitPMTID`` and ``digitPMTID`` will always be a subset of ``mcPMTID``).

The ``meta`` branch of the output file should only have a single entry and contains information relevant for all simulated events, such as the run-number, the run-time, the PMT positions, and the total number of simulated events. The data-structure for the "default" ``meta`` tree, that are always written to the file, are:

================================  ===================  ===================
**Name**                          **Type**             **Description**
================================  ===================  ===================
``runID``                         int                  The run number, from the RUN table.
``runType``                       ulong64              The run type, from the RUN table.
``runTime``                       ulong64              The unix start time of the run.
``dsentries``                     int                  The total number of simulated events.
``macro``                         string               The macro name.
``pmtType``                       vector<int>          The list of PMT types.
``pmtID``                         vector<int>          The list of PMT IDs.
``pmtChannel``                    vector<int>          The list of PMT electronic channels.
``pmtIsOnline``                   vector<bool>         The list of whether each PMT is online.
``pmtCableOffset``                vector<double>       The list of calibration cable offsets per PMT.
``pmtChargeScale``                vector<double>       The list of calibration charge scales per PMT.
``pmtPulseWidthScale``            vector<double>       The list of calibration pulse width scales per PMT.
``pmtX``                          vector<double>       The list of PMT x positions.
``pmtY``                          vector<double>       The list of PMT y positions.
``pmtZ``                          vector<double>       The list of PMT z positions.
``pmtU``                          vector<double>       The list of PMT x directions.
``pmtV``                          vector<double>       The list of PMT y directions.
``pmtW``                          vector<double>       The list of PMT z directions.
``digitizerWindowSize``           uint32               The length of the digitizer window.
``digitizerSampleRate_GHz``       double               The sampling rate of the digitizer, in GHz.
``digitizerDynamicRange_mV``      double               The dynamic range of the digitizer, in mV.
``digitizerResolution_mVPerADC``  double               The resolution of the digitizer, in mV/ADC.
================================  ===================  ===================

The data-structure for the ``output`` tree is as follows. First, the "default" variables that are always written to the ntuple are detailed below. Note the distinction between simulated event and detector event, described in :ref:`daq_processors`. The word "true" is used to indicate simulated quantities that are not affected by the detector response.

===========================  ===================  ===================
**Name**                     **Type**             **Description**
===========================  ===================  ===================
``mcpdg``                    int                  Particle data code for highest energy particle.
``mcx``                      double               True x position of the highest energy particle.
``mcy``                      double               True y position of the highest energy particle.
``mcz``                      double               True z position of the highest energy particle.
``mcu``                      double               True x direction of the highest energy particle.
``mcv``                      double               True y direction of the highest energy particle.
``mcw``                      double               True z direction of the highest energy particle.
``mcke``                     double               True kinetic energy of the highest energy particle.
``mct``                      double               True time, relative to the start of the simulation, of the highest energy particle.
``scintEdep``                double               True total energy deposited in the scintillator (0 if no scintillator).
``scintEdepQuenched``        double               True total quenched energy deposited in the scintillator.
``scintPhotons``             int                  True total number of scintillation photons produced.
``remPhotons``               int                  True total number of re-emitted photons produced.
``cherPhotons``              int                  True total number of Cherenkov photons produced.
``mcid``                     int                  The simulated event ID.
``mcparticlecount``          int                  The true total number of simulated particles.
``mcnhits``                  int                  The true total number of PMTs that detected light.
``mcpecount``                int                  The true total number of detector photoelectrons.
``evid``                     int                  The detector event ID.
``subev``                    int                  The ID of the event within a single simulated event.
``nhits``                    int                  The total number of PMTs that detected light in the detector event.
``triggerTime``              double               The trigger time of the detector event, relative to the start of the simulation.
``timestamp``                double               The UTC time of the detector event.
``timeSinceLastTrigger_us``  double               The time since the last triggered event, in microseconds. 
``event_cleaning_word``      ulong64              The list of event cleaning cuts that failed.
===========================  ===================  ===================

If ``include_mcparticles`` is set then we additionally add the following information to the ``output`` branch of the ntuple. These are filled from the ``DS::MCParticle`` branch. This provides a method for looking at all simulated particles, rather than just the first.

===========================  ===================  ===================
**Name**                     **Type**             **Description**
===========================  ===================  ===================
``mcpdgs``                   vector<int>          Particle data code for all particles.
``mcxs``                     vector<double>       True x position of all particles.
``mcys``                     vector<double>       True y position of all particles.
``mczs``                     vector<double>       True z position of all particles.
``mcus``                     vector<double>       True x direction of all particles.
``mcvs``                     vector<double>       True y direction of all particles.
``mcws``                     vector<double>       True z direction of all particles.
``mckes``                    vector<double>       True kinetic energy of all particles.
``mcts``                     vector<double>       True time of each particle, relative to the start of the simulation.
===========================  ===================  ===================

If ``include_pmthits`` is set then we additionally add the following information to the ``output`` branch of the ntuple. Note that a DAQ system must also run in order for these variables to be filled, see :ref:`daq_processors` for more details. The ``hitPMT`` variables are filled from the ``RAT::DS::PMT``, described in :ref:`ratds`.

===========================  ===================  ===================
**Name**                     **Type**             **Description**
===========================  ===================  ===================
``hitPMTID``                 vector<int>          The unique ID of each of the PMTs that detected light.
``hitPMTTime``               vector<double>       The hit-time of the first PE at each PMT.
``hitPMTCharge``             vector<double>       The charge for each PMT that detected light.
===========================  ===================  ===================

If ``include_digitizerhits`` is set then we additionally add the following information to the ``output`` branch of the ntuple. Note that a DAQ system and waveform analysis processor must also run in order for these variables to be filled, see :ref:`daq_processors` and :ref:`waveform_analysis` for more details. The ``digitPMT`` variables are filled from the ``RAT::DS::DigitPMT``, described in :ref:`ratds`.

=============================  ===================  ===================
**Name**                       **Type**             **Description**
=============================  ===================  ===================
``digitPMTID``                 vector<int>          The unique ID of each of the PMT waveform that crossed threshold.
``digitNhits``                 int                  The total number of PMT waveforms that crossed threshold.
``digitTime``                  vector<double>       The hit-time extracted from each PMT waveform.
``digitCharge``                vector<double>       The charge extracted from each PMT waveform.
``digitNCrossings``            vector<int>          The total number of times each PMT waveform crossed threshold.
``digitTimeOverThreshold``     vector<double>       The total time each PMT waveform spent above threshold.
``digitVoltageOverThreshold``  vector<double>       The integrated voltage over threshold for each PMT.
``digitPeak``                  vector<double>       The peak voltage of each PMT waveform.
``digitLocalTriggerTime``      vector<double>       Convenience variable to add per-PMT calibration timing.
``digitReconNPEs``             vector<int>          The total number of PEs per PMT, as estimated by a PE counting method.
=============================  ===================  ===================

If ``include_digitizerfits`` is set then we additionally add the following information to the ``output`` branch of the ntuple. Note that a DAQ system and waveform analysis processor must also run in order for these variables to be filled, see :ref:`daq_processors` and :ref:`waveform_analysis` for more details. In particular, these are filled by a specific waveform analysis algorithm, such as the lognormal fits described in :ref:`lognormalfit`. The fitter will append its name to the variable name (e.g., ``fit_pmtid_lognormal``). These are filled from the ``DS::WaveformAnalysisResult`` branch.

======================================  ===================  ===================
**Name**                                **Type**             **Description**
======================================  ===================  ===================
``fit_pmtid_+{fitter_name}``            vector<int>          The unique ID of each of the PMT waveform that was fit.
``fit_time_+{fitter_name}``             vector<double>       The time extracted from each PMT waveform fit.
``fit_charge_+{fitter_name}``           vector<double>       The charge extracted from each PMT waveform fit.
``fit_FOM_+{fitter_name}_+{fom_name}``  vector<double>       The figure of merit extracted from each PMT waveform fit.
======================================  ===================  ===================

If ``include_nestedtubehits`` is set then we additionally add the following information to the ``output`` branch of the ntuple. These "nested tubes" are intended for use with liquid-O style fiber optics detectors. These are filled from the ``DS::MCNestedTube`` branch.

=============================  ===================  ===================
**Name**                       **Type**             **Description**
=============================  ===================  ===================
``mcnNTs``                     int                  Total number of nested tubes that detected light.
``mcnNThits``                  int                  Total number of PEs detector by nested tubes.
``mcNTid``                     vector<int>          The unique ID of each true nested tube that detected light.
``mcNThittime``                vector<double>       The true time of each PE detected by a nested tube.
``mcNThitx``                   vector<double>       The true x position for each PE detected by a nested tube.
``mcNThity``                   vector<double>       The true y position for each PE detected by a nested tube.
``mcNThitz``                   vector<double>       The true z position for each PE detected by a nested tube.
``ntId``                       vector<int>          The unique ID for each detector nested tube that detected light.
=============================  ===================  ===================

If ``include_nestedtubehits`` is set then we additionally add the following information to the ``meta`` branch of the ntuple. 

=============================  ===================  ===================
**Name**                       **Type**             **Description**
=============================  ===================  ===================
``ntX``                        vector<double>       The x position of the nested tubes.
``ntY``                        vector<double>       The y position of the nested tubes.
``ntZ``                        vector<double>       The z position of the nested tubes.
``ntU``                        vector<double>       The x direction of the nested tubes.
``ntV``                        vector<double>       The y direction of the nested tubes.
``ntW``                        vector<double>       The z direction of the nested tubes.
=============================  ===================  ===================

If ``include_mchits`` is set then we additionally add the following information to the ``output`` branch of the ntuple. The ``mcPMT`` variables are filled from the ``RAT::DS::MCPMT`` branch, whereas the ``mcPE`` variables are filled from the ``RAT::DS::MCPhoton`` branch.

=============================  ===================  ===================
**Name**                       **Type**             **Description**
=============================  ===================  ===================
``mcPMTID``                    vector<int>          The unique IDs of the true hit PMTs.       
``mcPMTNPE``                   vector<int>          The true number of PEs for each hit PMT.
``mcPMTCharge``                vector<double>       The true charge deposited on each PMT.
``mcPEPMTID``                  vector<int>          The unique ID of the PMT that detected each PE.
``mcPEHitTime``                vector<double>       The true detection time of each PE.
``mcPEFrontEndTime``           vector<double>       The true detection time, smeared by the sensor response, of each PE.
``mcPEProcess``                vector<string>       The process that created the photon that created the PE.
``mcPEx``                      vector<double>       The true x position of the PE.
``mcPEy``                      vector<double>       The true y position of the PE.
``mcPEz``                      vector<double>       The true z position of the PE.
``mcPECharge``                 vector<double>       The true charge of each PE.
=============================  ===================  ===================

If ``include_tracking`` is set then we additionally add the following information to the ``output`` branch of the ntuple. These variables are filled from the ``RAT::DS::MCTrack`` and ``RAT::DS::MCTrackStep`` branches. The variables below are mostly 2D vectors. The inner vector is the set of steps along the particle track (``RAT::DS::MCTrackStep``) and the outer vector is the set of tracks along the particle trajectory (``RAT:DS::MCTrack``). In other words, each track can have many steps, each of which as an associated position, momentum, process, and volume. As a reminder, ``/tracking/storeTrajectory 1`` must also be set in the macro in order to save the tracking information.

=============================  ======================  ===================
**Name**                       **Type**                **Description**
=============================  ======================  ===================
``trackPDG``                   vector<int>             The PDG code of the particle associated with this track. 
``trackPosX``                  vector<vector<double>>  The starting x position of each of the steps along the particle track.
``trackPosY``                  vector<vector<double>>  The starting y position of each of the steps along the particle track.
``trackPosZ``                  vector<vector<double>>  The starting z position of each of the steps along the particle track.
``trackMomX``                  vector<vector<double>>  The starting x momentum of each of the steps along the particle track.
``trackMomY``                  vector<vector<double>>  The starting y momentum of each of the steps along the particle track.
``trackMomZ``                  vector<vector<double>>  The starting z momentum of each of the steps along the particle track.
``trackKE``                    vector<vector<double>>  The kinetic energy of each of the steps along the particle track.
``trackTime``                  vector<vector<double>>  The time, relative to the start of the simulation, of the particle steps.
``trackProcess``               vector<vector<int>>     The ID of the process that created the step.
``trackVolume``                vector<vector<int>>     The ID of the detector volume the step started in.
=============================  ======================  ===================

If ``include_tracking`` is set then we additionally add the following information to the ``meta`` branch of the ntuple.

=============================  ===================  ===================
**Name**                       **Type**             **Description**
=============================  ===================  ===================
``processCodeMap``             map<string, int>     A map from process name to process ID.
``volumeCodeMap``              map<string, int>     A map from volume name to volume ID.
=============================  ===================  ===================

If ``include_digitizerwaveforms`` is set then we create a new branch in the ntuple called ``waveform`` that stores the full digitized waveforms. Note this will significantly increase the size of the files. The ``waveform`` branch will contain the following variables:

=============================  ===================  ===================
**Name**                       **Type**             **Description**
=============================  ===================  ===================
``evid``                       int                  The event ID, repeated in the ``output`` tree.
``waveform_pmtid``             vector<int>          The unique PMT ID associated with the waveform.
``inWindowPulseTimes``         vector<double>       The list of MCPE front-end times that fall inside the waveform window.
``inWindowPulseCharges``       vector<double>       The list of MCPE charges that fall inside the waveform window.
``waveform``                   vector<ushort>       The digitized waveform, per PMT.
=============================  ===================  ===================

.. _outnet:

OutNet
======
NOTE: This processor is not currently supported. The below documentation is outdated, but may provide some useful information.

The !OutNet processor transmits events over the network to a listening copy of
RAT which is running the [wiki:UserGuideInNet InNet] event producer.  Multiple
listener hostnames may be specified, and events will be distributed across them
with very simplistic load-balancing algorithm.

This allows an event loop to be split over multiple machines.  I'll leave it to
your imagination to think up a use for this...

Command:
::

    /rat/proc outnet


Parameters:
::

    /rat/procset host "hostname:port"

* hostname:port (required) Network hostname (or IP address) and port number of
  listening RAT process.  

=== Notes ===

The "load balancing" mentioned above distributes events by checking to see
which sockets are available for writing and picking the first one that can be
found.  The assumption is that busy nodes will have a backlog of events, so
their sockets will be full.  In principle, this means that a few slow nodes
won't hold up the rest of the group.

This processor and its [wiki:UserGuideInNet sibling event producer] have no
security whatsoever.  Don't use your credit card number as a seed for the Monte
Carlo.
