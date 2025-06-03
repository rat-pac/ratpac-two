.. _reconstruction_processors:

Reconstruction
``````````````

Here we describe the reconstruction processors.

----------------------

.. _fitter_handler:

Fitter Input Handler
====================

Document the fitter handler.

----------------------

.. _centroid:

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

.. _quad:

Quad Fitter
===========

Quad fitter details.

----------------------

.. _direction_center_fitter:

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
3. PathFitter likelihood is minimized with Minuit2 from stage 1's result.

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

