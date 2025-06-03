.. _daq_processors:

DAQ Models
``````````

The DAQ processors are located in ratpac-two in the ``src/daq/`` directory. These processors are provided primarily as simple examples and helpful tools for producing triggered events, but will not accurately represent a realistic trigger system for a detector. In general, the DAQ processors can provide the below listed functionality (although the simple versions skip several of these steps):

#. Read information from the database, specified in ``DAQ.ratdb``, for the DAQ settings. Some settings are also provided directly through ``/rat/proc setting value``, as detailed individually for each processor below. 
#. Get the MC information, primarily the true number of PMTs (``DS::MCPMT``), that detected light. 
#. For that group of PMTs, build-up information about the event using the PMT hit-times and/or PMT charges. As an example, we may generate a hypothetical trigger signal pulse that we could then check against a threshold.
#. Issue a trigger decision about whether to create a triggered event. This is represented in ratpac-two as a ``DS::EV`` object.
#. Build up information about the event, such as the event count, the trigger time, etc. We also create new PMT objects (``DS::PMT``) that represent PMT hits within the triggered event. Several of the DAQ processors will loop through these PMTs to create information such as the total integrated charge for the event.
#. Based on whether it's enabled, we run the waveform digitization for the triggered event. 

In principle, the DAQ code provided in ratpac-two is primarily for testing purposes and any experiment using ratpac-two would write their own custom DAQ code that could build from what is provided. If no DAQ is specified there will be no triggered events, and only the ``DS::MC`` related branches will be filled. In other words, all of the detector-related aspects of the data-structure (``DS::EV``, ``DS::PMT``, etc.) are only filled if one of the DAQ processors is run. 

----------------------

.. _forced_trigger:

Forced Trigger
==============

The forced trigger processor is the simplest triggering scheme, which forces the detector to trigger for every MC event. A single EV is created for every MC event and the event structure is filled accordingly. There is no condition for issuing a trigger decision. This may be used for testing a random pulsed trigger, a beam trigger, or something similar.

Command:
::

    /rat/proc forcedtrigger

Parameters: None

The digitization settings can be configured through the ``DAQ.ratdb`` table.

----------------------

.. _simple_daq:

Simple DAQ
==========
The SimpleDAQ processor simulates a minimal data acquisition system.  The time of each PMT hit is the time of the first photon hit plus the timing distribution of the appropriate PMT (i.e. the "frontEndTime" of the first photon), and the charge collected at each PMT is just the sum of all charge deposited at the anode, regardless of time.  All PMT hits are packed into a single event, such that the number of DAQ events will equal the number of MC events. This acts very similarly to the forced trigger processor, but will only fill the PMT branch if there is at least one hit.

Command:
::

    /rat/proc simpledaq

Parameters: None

----------------------

.. _split_ev_daq:

Split-EV DAQ
============
The SplitEVDaq processor achieves the most realistic of the data acquisition models by summing square trigger pulses together according to the hit-times of the PMTs. The trigger sum is compared against a configurable global trigger threshold, and events above threshold cause a detector trigger. SplitEVDaq also properly handles splitting events separated in time into separate triggered events, which is critical for simulating coincidence events such as IBDs. The parameters of the triggering are highly configurable and include the width of trigger pulses, the size of the trigger window, the size of the time-steps, etc.

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

The digitization settings can also be configured through the ``DAQ.ratdb`` table.

----------------------

