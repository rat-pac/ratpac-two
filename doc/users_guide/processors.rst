.. _processors:

Event Processors
----------------

Event processors, described in :ref:`producers_processors`, are part of the event loop. They do not create new events, but instead receives events one-by-one and may either change the event by adding to or altering its contents. As an example, the data aquisition (DAQ) processors will receive an event and then, based on information such as the number of PMTs that detected light, decide whether the event caused the detector to trigger. All ratpac-two processors inherit the methods from the processor class, which provides the structure for how all processors run. For more details about these methods, how to use them, and how to write new processors, find details in the Programmer's guide: :ref:`programming_a_processor`. Below we describe the existing processors in ratpac-two.

----------------------

.. _using_a_processor_from_the_macro:

Using a Processor From the Macro
````````````````````````````````

The ratpac-two processors run as a block in the macro, instantiated after the '/run/initialize' line and prior to the generators. Below is an example of where several processors are run in a macro. Here we specify the last processor in the chain using the ``proclast`` syntax.

::

        ... set database params up here

        /run/initialize

        /rat/proc splitevdaq

        /rat/proc count
        /rat/procset update 100

        /rat/proc classifychargebalance

        /rat/proc quadfitter

        /rat/proclast outntuple

        /generator/add combo gun:point:poisson
        ... add more generator info down here


The parameters for a processor are often highly configurable. This can be achieved by loading these parameters from the database, ratdb, described more in :ref:`ratdb`. In general, if the processors parameter is loaded from ratdb, we can change it from the macro using (using the PMT noise processor as an example): 

::

        /rat/db/set NOISEPROC noise_flag 1

Additionally, for processors, there are methods provided that allow the user to directly change parameters using the ``procset`` syntax (as shown already in the above example for the count processor):

::

        /rat/proc splitevdaq
        /rat/procset trigger_threshold 4.0

Cases where a parameter is tunable using ``procset`` are documented specifically for each processor below. For more details the Programmer's guide: :ref:`programming_a_processor` shows how this is achieved in the code using the 'Set' methods provided by the Processor class.

----------------------

.. _daq_processors:

DAQ Processors
``````````````

The DAQ processors are located in ratpac-two in the ``src/daq/`` directory. These processors are provided primarily as simple examples and helpful tools for producing triggered events, but will not accurately represent a realistic trigger system for a detector. In general, the DAQ processors can provide the below listed functionality (although the simple versions skip several of these steps):

#. Read information from the database, specified in ``DAQ.ratdb``, for the DAQ settings. Some settings are also provided directly through ``/rat/proc setting value``, as detailed individually for each processor below. 
#. Get the MC information, primarily the true number of PMTs (``DS::MCPMT``), that detected light. 
#. For that group of PMTs, build-up information about the event using the PMT hit-times and/or PMT charges. As an example, we may generate a hypothetical trigger signal pulse that we coudl then check against a threshold.
#. Issue a trigger decision about whether to create a triggered event. This is represented in ratpac-two as a ``DS::EV`` object.
#. Build up information about the event, such as the event count, the trigger time, etc. We also create new PMT objects (``DS::PMT``) that represent PMT hits within the triggered event. Several of the DAQ processors will loop through these PMTs to create information such as the total integrated charge for the event.
#. Based on whether its enabled, we run the waveform digitization for the triggered event. 

In principle, the DAQ code provided in ratpac-two is primarily for testing purposes and any experiment using ratpac-two would write their own custom DAQ code that could build from what is provided.

----------------------

.. _forced_trigger:

Forced Trigger
==============

The forced trigger processor is the simples triggering scheme, which forces the detector to trigger for every MC event. A single EV is created for every MC event and the event structure is filled accordingly. There is no condition for issuing a trigger decision. This may be used for testing a random pulsed trigger, a beam trigger, or something similar.

Command:
::

    /rat/proc forcedtrigger

Parameters:
::

    /rat/procset digitizer_name "digitizer"
    /rat/procset digitize

----------------------

.. _simple_daq:

Simple DAQ
==========
The SimpleDAQ processor simulates a minimal data acquisition system.  The time of each PMT hit is the time of the first photon hit plus the timing distribution of the appropriate PMT (i.e. the "frontEndTime" of the first photon), and the charge collected at each PMT is just the sum of all charge deposited at the anode, regardless of time.  All PMT hits are packed into a single event, such that the number of DAQ events will equal the number of MC events. This acts very similarly to the forced trigger processor, but will only fill the PMT branch if there is a least one hit.

Command:
::

    /rat/proc simpledaq

Parameters: None

----------------------

.. _split_ev_daq:

Split-EV DAQ
============
The SplitEVDaq processor achieves the most realistic of the data acquisition models by summing square trigger pulses together according to the hit-times of the PMTs. The trigger sum is compared against a configurable global trigger threshold, and events above threshold cause a detector trigger. SplitEVDaq also properly handles splitting events seprated in time into separate triggered events, which is critical for simulating coincidence events such as IBDs. The parameters of the triggering are highly configurable and include the width of trigger pulses, the size of the trigger window, the size of the time-steps, etc.

Command:
::

    /rat/proc splitevdaq

Parameters:
::

    /rat/procset pulse_width "value"
    /rat/procset trigger_window "value"
    /rat/procset trigger_threshold "value"
    /rat/procset trigger_lockout "value"
    /rat/procset trigger_resolution "value"
    /rat/procset pmt_lockout "value"
    /rat/procset lookback "value"
    /rat/procset max_hit_time "value"
    /rat/procset trigger_on_noise "0"|"1"
    /rat/procset digitizer_name "digitizer"
    /rat/procset digitize

----------------------

.. _count_processor:

Count Processor
```````````````
The count processor exists mostly as a simple demonstration processor.  It also
displays messages periodically showing both how many physics events and
detector events have been processed. The message looks something like::

    CountProc: Event 5 (8 triggered events)

Command:
::

    /rat/proc count

Parameters:
::

    /rat/procset update [interval]

* interval (optional, integer) - Sets number of physics events between between
  status update messages.  Defaults to 1 (print a message for every event).

----------------------

Prune Processor
```````````````
The Prune processor is a processor for removing unwanted parts of the data structure to save space. The prune processor may be useful to call before the OutROOT processor to avoid writing large amounts of data to disk.

Note that there is minimal benefit to pruning in order to save memory in the running program.  Only one data structure is present in memory at any given time, and it is never copied.  Only when lots of events are written to disk does the overhead become considerable.

Command:
::

    /rat/proc prune


Parameters:
::

    /rat/procset prune "cutlist"

* cutlist - (required) a comma separated (no spaces) list of parts of the data
  structure to remove. [[BR]]The currently allowed entries are:

  * mc.particle
  * mc.pmt
  * mc.pmt.photon
  * mc.track
  * ev
  * ev.pmt

If /tracking/storeTrajectory is turned on, mc.track:particle is used, where particle is the name of the particle track you want to prune (mc.track:opticalphoton will prune optical photon tracks).

----------------------

Reconstruction Processors
`````````````````````````

Here we describe the reconstruction processors.

----------------------

Centroid Fitter
===============
The ``FitCentroid`` processor reconstructs the position of detector events using
the charge-weighted sum of the hit PMT position vectors.

Command:
::

    /rat/proc fitcentroid

Parameters: None

Position fit information in data structure
''''''''''''''''''''''''''''''''''''''''''
* name - "centroid"
* figures of merit - None

----------------------

Quad Fitter
===========

Quad fitter details.

----------------------

Direction Center Fitter
=======================
The ``fitdirectioncenter`` processor reconstructs the direction of events
as the average of the vectors from the event position to the hit PMT positions.

Command:
::

    /rat/proc fitdirectioncenter

Parameters
''''''''''
No parameters are required though a position reconstruction would need to be run before.
Several useful parameters can be set in macro, which allows the processor to be run
multiple times with different settings in a single macro.

Detailed implementations are illustrated in macros/examples/fitdirectioncenter.mac
In particular, there is an example to correct for the drive effect in reconstructed
position.  First, a position reconstruction is run, then a direction reconstruction,
as usual.  However, a second direction reconstruction is run and takes both the
reconstructed position and direction as input to correct for the drive.  The resulting
position is then saved in the fitdirectioncenter FitResult.

=========================   ==========================  ===================
**Field**                   **Type**                    **Description**
=========================   ==========================  ===================
``fitter_name``             ``string``                  Defaults to "fitdirectioncenter"
``position_fitter``         ``string``                  Name of fitter providing position input
``direction_fitter``        ``string``                  Name of fitter providing direction for drive correction

``pmt_type``                ``int``                     PMT "type" to use.  Multiple types can be used.  Defaults to all types.
``verbose``                 ``int``                     FOMs saved in FitResult.  1 saves ``num_PMT``.  2 also saves ``time_resid_low`` and ``time_resid_up``

``time_resid_low``          ``double``                  Lower cut on time residuals in ns
``time_resid_up``           ``double``                  Upper cut on time residuals in ns

``time_resid_frac_low``     ``double``                  Lower cut on time residuals as a fraction in [0.0, 1.0)
``time_resid_frac_up``      ``double``                  Upper cut on time residuals as a fraction in (0.0, 1.0]

``light_speed``             ``double``                  Speed of light in material in mm/ns.  Defaults to water.

``event_position_x``        ``double``                  Fixed position of event in mm
``event_position_y``        ``double``                  Fixed position of event in mm
``event_position_z``        ``double``                  Fixed position of event in mm

``event_time``              ``double``                  Fixed offset of time residuals in ns

``event_drive``             ``double``                  Fixed offset of position input in mm
=========================   ==========================  ===================

Direction fit information in data structure
'''''''''''''''''''''''''''''''''''''''''''
* figure of merit - ``num_PMT`` is the number of PMTs used in a reconstruction
* figure of merit - ``time_resid_low`` is the earliest time residual that passes the lower time residual cut
* figure of merit - ``time_resid_up`` is the latest time residual that passes the upper time residual cut

----------------------


Path Fitter
===========
The ``fitpath`` processor is an implementation (still a work in progress) of
the successful PathFitter algorithm used in SNO. It fits position, time, and
direction for cherenkov events using a maximum likelihood fit of hit time
residuals while taking into account different paths the hit could have taken.
For "direct" light (i.e. neither reflected nor scattered) an angular
distribution of cherenkov light is taken into account to fit the direction. All
other light is considered "other" and does not contribute to the direction fit.

Minimization is done in three stages:
1. Hit time residuals are minimized directly using `simulated-annealing`
from a static seed. 
2. PathFitter likelihood is minimized with `simulated-annealing` from
stage 1's result.
2) PathFitter likelihood is minimized with Minuit2 from stage 1's result.

Command
'''''''
::

    /rat/proc fitpath

Parameters
''''''''''
None required from macro. ``fitpath`` reads parameters from a table ``FTP``
containing the following fields:

=========================   ==========================  ===================
**Field**                   **Type**                    **Description**
=========================   ==========================  ===================
``num_cycles``              ``int``                     Number of annealing iterations (times to lower temp)
``num_evals``               ``int``                     Number of evaluations per iteration (evals per temp)
``alpha``                   ``double``                  Controls the rate of cooling in :ref:`simulated-annealing`

``seed_pos``                ``double[3]``               Static position seed to stage 0
``pos_sigma0``              ``double``                  Size of initial stage 0 simplex in position coordinates
``seed_time``               ``double``                  Static time seed to stage 0
``time_sigma0``             ``double``                  Size of initial stage 0 simplex in time
``temp0``                   ``double``                  Initial temperature of :ref:`simulated-annealing` for stage 0

``seed_theta``              ``double``                  Static theta (detector coordinates) seed to stage 1
``theta_sigma``             ``double``                  Size of initial stage 1 simplex in theta
``seed_phi``                ``double``                  Static phi (detector coordinates) seed to stage 1
``phi_sigma``               ``double``                  Size of initial stage 1 simplex in phi
``pos_sigma1``              ``double``                  Size of initial stage 1 simplex in position coordinates
``time_sigma1``             ``double``                  Size of initial stage 1 simplex in time
``temp1``                   ``double``                  Initial temperature of :ref:`simulated-annealing` for stage 1

``cherenkov_multiplier``    ``double``                  Number of cherenkov photons generated per hits detected
``light_speed``             ``double``                  Speed of light in material in mm/ns 
``direct_prob``             ``double``                  Fraction of direct detected light
``other_prob``              ``double``                  Fraction of late detected light
``photocathode_area``       ``double``                  Area of photocathode mm^2

``direct_time_first``       ``double``                  Time (ns) of first entry in ``direct_time_prob``
``direct_time_step``        ``double``                  Time step (ns) between entries in ``direct_time_prob``
``direct_time_prob``        ``double[]``                Probability (need not be normalized) of being "direct" light with a certain time residual

``other_time_first``        ``double``                  Time (ns) of first entry in ``other_time_prob``
``other_time_step``         ``double``                  Time step (ns) between entries in ``other_time_prob``
``other_time_prob``         ``double[]``                Probability (need not be normalized) of being "other" light with a certain time residual

``cosalpha_first``          ``double``                  Cos(alpha) of first entry in ``cosalpha_prob``
``cosalpha_step``           ``double``                  Cos(alpha) step between entries in ``cosalpha_prob``
``cosalpha_prob``           ``double[]``                Probability (need not be normalized) of Cherenkov light being emitted at a certain cos(alpha) w.r.t. particle direction
=========================   ==========================  ===================

Fit information in DS
'''''''''''''''''''''
In the ``EV`` branch the ``PathFit`` class contains Get/Set methods for the
following data:

======================  ==========================  ===================
**Field**               **Type**                    **Description**
======================  ==========================  ===================
``Time0``               ``double``                  Time seed from simple hit time residual minimization
``Pos0``                ``TVector3``                Position seed from simple hit time residual minimization
``Time``                ``double``                  Time resulting from final stage of minimization
``Position``            ``TVector3``                Position resulting from final stage of minimization
``Direction``           ``TVector3``                Direction resulting from final stage of minimization
======================  ==========================  ===================

``PathFit`` implementes ``PosFit`` under the name ``fitpath``.

MiniSim
=======

What does this do? Do we need this in RAT?

ClassifyChargeBalance
=====================

Document this!

FitTensor
=========

Document this!

----------------------

Output Processors
`````````````````

OutROOT
=======
The OutROOT processor writes events to disk in the ROOT format.  The events are
stored in a TTree object called "T" and the branch holding the events (class
[source:RAT/trunk/include/RAT_DS.hh#latest RAT_DS]) is called "ds".

Command:
::

    /rat/proc outroot

Parameters:
::

    /rat/procset file "filename"


* filename (required, string) Sets output filename.  File will be deleted if it already exists.

OutNtuple
=========

the Outntuple proc details.

----------------------

OutNet
======
Note: This has been untested for like a decade?

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
