Event Processors
----------------
(Missing information)

simpledaq
`````````
The SimpleDAQ processor simulates a minimal data acquisition system.  The time
of each PMT hit is the time of the first photon hit plus the timing
distribution of the appropriate PMT (i.e. the "frontEndTime" of the first 
photon), and the charge collected at each PMT is just the sum of all charge 
deposited at the anode, regardless of time.  All PMT hits are packed into a 
single event, such that the number of DAQ events will equal the number of MC 
events.

Command
'''''''
::

    /rat/proc simpledaq

Parameters
''''''''''
None.

lesssimpledaq
`````````````
Here, the timing and charge information of each PMT is estimated the same way 
as simpledaq, but a minimal trigger is also simulated based on a trigger 
threshold and a trigger time window. The trigger threshold is the minimum 
number of PMTs that recorded signal within the trigger window. A sliding time 
window is used to identify hits clustered in time.

If the number of hits within a trigger window exceeds the trigger threshold, 
all hits between the pre- and post-trigger boundaries are recorded in a 
"subevent". The hit times within a subevent are all relative to the "cluster 
time", i.e. the time of the hit that tripped the trigger threshold. These 
relative hit times are also what are used to determine if hits occur between 
the pre- and post-trigger boundaries.

The trigger threshold and window are not customisable without altering the 
source code.

| Post Trigger Window: 600 ns
| Pre Trigger Window: -200 ns
| Trigger Window:     200 ns
| Trigger Threshold:  6

Command
'''''''
::
    
        /rat/proc lesssimpledaq

Parameters
''''''''''
None.


splitevdaq
``````````
Behaves the same as lesssimpledaq, but has a more configurable trigger. Also, a 
more realistic output is achieved by splitting hits over multiple trigger 
windows into whole new events, rather than subevents.

Command
'''''''
::

    /rat/proc splitevdaq

Parameters
''''''''''
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

count
`````
The count processor exists mostly as a simple demonstration processor.  It also
displays messages periodically showing both how many physics events and
detector events have been processed. The message looks something like::

    CountProc: Event 5 (8 triggered events)


Command
'''''''
::

    /rat/proc count

Parameters
''''''''''
::

    /rat/procset update [interval]

* interval (optional, integer) - Sets number of physics events between between
  status update messages.  Defaults to 1 (print a message for every event).

prune
`````
The Prune processor is not a kitchen aid, but rather a processor for removing
unwanted parts of the data structure to save space.  The prune processor is
very useful to call before the [wiki:UserGuideOutRoot OutROOT] processor to
avoid writing large amounts of data to disk.

Note that there is minimal benefit to pruning in order to save memory in the
running program.  Only one data structure is present in memory at any given
time, and it is never copied.  Only when lots of events are written to disk
does the overhead become considerable.

Command
'''''''
::

    /rat/proc prune


Parameters
''''''''''
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

If /tracking/storeTrajectory is turned on, mc.track:particle is used, where
particle is the name of the particle track you want to prune
(mc.track:opticalphoton will prune optical photon tracks).

A complex example of pruning can be seen in the
[source:RAT/trunk/mac/prune.mac#latest prune.mac] macro file included in the
RAT source.

fitcentroid
```````````
The ``FitCentroid`` processor reconstructs the position of detector events using
the charge-weighted sum of the hit PMT position vectors.

Command
'''''''
::

    /rat/proc fitcentroid

Parameters
''''''''''
None

Position fit information in data structure
''''''''''''''''''''''''''''''''''''''''''
* name - "centroid"
* figures of merit - None


fitdirectioncenter
``````````````````
The ``fitdirectioncenter`` processor reconstructs the direction of events
as the average of the vectors from the event position to the hit PMT positions.

Command
'''''''
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
''''''''''''''''''''''''''''''''''''''''''
* figure of merit - ``num_PMT`` is the number of PMTs used in a reconstruction
* figure of merit - ``time_resid_low`` is the earliest time residual that passes the lower time residual cut
* figure of merit - ``time_resid_up`` is the latest time residual that passes the upper time residual cut


fitpath
```````
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

outroot
```````
The OutROOT processor writes events to disk in the ROOT format.  The events are
stored in a TTree object called "T" and the branch holding the events (class
[source:RAT/trunk/include/RAT_DS.hh#latest RAT_DS]) is called "ds".

Command
'''''''
::

    /rat/proc outroot

Parameters
''''''''''
::

    /rat/procset file "filename"


* filename (required, string) Sets output filename.  File will be deleted if it already exists.

outnet
``````
The !OutNet processor transmits events over the network to a listening copy of
RAT which is running the [wiki:UserGuideInNet InNet] event producer.  Multiple
listener hostnames may be specified, and events will be distributed across them
with very simplistic load-balancing algorithm.

This allows an event loop to be split over multiple machines.  I'll leave it to
your imagination to think up a use for this...

Command
'''''''
::

    /rat/proc outnet


Parameters
''''''''''
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
