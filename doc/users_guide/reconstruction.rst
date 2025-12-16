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

.. _classifytimes:

ClassifyTimes
=====================

The ``classifytimes`` processor calculates characteristic parameters of a hit time residual distribution.
One parameter is the ratio of hits in a time window (typically narrow around prompt times) divided by the
hits in another time window (often the full time window; i.e., all hits).  Within another specified time
window, the four central moments are calculated: mean, unbiased standard deviation, standardized unbiased
skewness, and standardized unbiased excess kurtosis.

Command:
::

    /rat/proc classifytimes

Parameters
''''''''''
No parameters are required though a position reconstruction should be run before (or a fixed position specified) and a
light speed is needed to calculate time residuals.  The light speed and a time window for the ratio are specified in
CLASSIFIER.ratdb.  Several useful parameters can be set in macro, which allows the processor to be run multiple times
with different settings in a single macro.  Detailed implementations are illustrated in macros/examples/classifytimes.mac.

=========================   ==========================  ===================
**Field**                   **Type**                    **Description**
=========================   ==========================  ===================
``classifier_name``         ``string``                  Defaults to "classifytimes".
``position_fitter``         ``string``                  Name of fitter providing position input.

``pmt_type``                ``int``                     PMT "type" to use.  Multiple types can be used.  Defaults to all types.
``verbose``                 ``int``                     FOM save option.  1 saves ``num_PMT``'s.  2 also saves ``time_resid_low`` and ``time_resid_up``

``numer_time_resid_low``    ``double``                  Lower cut on time residuals in ns.  Used for ``ratio``.  Defaults to value in CLASSIFIER.ratdb.
``numer_time_resid_up``     ``double``                  Upper cut on time residuals in ns.  Used for ``ratio``.  Defaults to value in CLASSIFIER.ratdb.

``denom_time_resid_low``    ``double``                  Lower cut on time residuals in ns.  Used for ``ratio``.  Defaults to full range.
``denom_time_resid_up``     ``double``                  Upper cut on time residuals in ns.  Used for ``ratio``.  Defaults to full range.

``time_resid_low``          ``double``                  Lower cut on time residuals in ns.  Option for central moments.
``time_resid_up``           ``double``                  Upper cut on time residuals in ns.  Option for central moments.

``time_resid_frac_low``     ``double``                  Lower cut on time residuals as a fraction in [0.0, 1.0).  Option for central moments.
``time_resid_frac_up``      ``double``                  Upper cut on time residuals as a fraction in (0.0, 1.0].  Option for central moments.

``light_speed``             ``double``                  Speed of light in material in mm/ns.  Defaults to water value in CLASSIFIER.ratdb.

``event_position_x``        ``double``                  Fixed position of event in mm.
``event_position_y``        ``double``                  Fixed position of event in mm.
``event_position_z``        ``double``                  Fixed position of event in mm.

``event_time``              ``double``                  Fixed offset of time residuals in ns.
=========================   ==========================  ===================

Classifier information in data structure
''''''''''''''''''''''''''''''''''''''''''
* name - ``classifytimes``
* classifier result - ``ratio`` is the ratio of the numbers of PMTs selected by specified time windows
* classifier result - ``mean`` is the mean time within the time window for central moments
* classifier result - ``stddev`` is the unbiased standard deviation of times within the time window for central moments
* classifier result - ``skewness`` is the standardized unbiased skewness of times within the time window for central moments
* classifier result - ``kurtosis`` is the standardized unbiased excess kurtosis of times within the time window for central moments
* classifier result - ``num_PMT`` is the number of PMTs used in the central moment calculations
* classifier result - ``num_PMT_numer`` is the number of PMTs used in the numerator of the ratio
* classifier result - ``num_PMT_denom`` is the number of PMTs used in the denominator of the ratio
* classifier result - ``time_resid_low`` is the earliest time residual in the time window for central moments
* classifier result - ``time_resid_up`` is the latest time residual in the time window for central moments

----------------------

FitTensor
=========

Document this!

FitMimir
========

``MIMIR`` is a general purpose event reconstruction framework that is meant to
encapsulate a large number of minimization-based reconstruction strategies
using a modular appraoch. It is designed to be flexible and extensible,
allowing a fit to be described in components taht can be added both in RATPAC2
itself and in a downstram prirvate experiment.

Command
'''''''
::

    /rat/proc mimir

Concepts
''''''''

Comoponents
+++++++++++
The MIMIR framework consists of a number of **components** that can be put
together into a full reconstruction recipe. There are three types of
components: 

- **CostFunction**: A function that the fit aims to minimize. This is typically a likelihood function
  or a similar function that produces a numerical value that represents the goodness of the current fit. 
- **Optimizer**: An engine that can minimize a given cost function (e.g. minuit, minuit2, NLOPT). 
- **FitStrategy**: A recipe that controls what is done in a fit. A fit strategy typically instantiates 
  and utilizes the above components during a fit. It is worthy of note that FitStrategies 
  can also consists of other FitStrategies, making them flexible and extensible.


ParamSet
++++++++

All MIMIR components manipulates a parameter set (``ParamSet``). This is a
structure that consists of the 7 possible paramers that could be fitted for a
event: The position of the event (xyz), the time of the event (t), the
direction of the event (theta, phi), and the energy of the event (E). For each
of these fields, ``ParamSet`` keeps track of the current value (either for
seeding or in the middle of a fit) of the field, the left and right bounds of
the value, as well as the status of the field. By default, the fields are set
to have bounds that are effectively infinite: the positional and time
coordinates have bounds at ``std::numerical_limits``, the directional componets
have bounds at ``[0, pi]`` and ``[-pi, pi]``, and energy has bounds at ``[0,
std::numerical_limits]``. 

The fields can take on the following statuses: 

- **INACTIVE**: The field does not participate in the fit nor the cost function.
  It will be ignored completely. 
- **FIXED**: The field does not participate in the fit, but is relevant for 
  evaluating the cost function. It will be passed to the cost function but its 
  value will not be modified by the optimizer.
- **ACTIVE**: The field is currently being fitted, it will be passed to the cost
  function and its value will be modified by the optimizer.


Top-level Configuration
'''''''''''''''''''''''
All components of MIMIR are cinfigured via entries to the RATDB. All
MIMIR-related configuration blocks have the tablename prefix of ``MIMIR_``. At
the very top level, the processor instance needs to be pointed to a
configuration for a ``FitStrategy`` configuration block. This can be done in
the macro via ``procset`` comamands. For example, to instruct the fitter to use
the strategy ``FitStep`` with the configuration type
``PositionTime_PMTTypeTimeResidual``, one would use:

::

    /rat/procset strategy "FitStep[PositionTime_PMTTypeTimeResidual]"

If no such ``procset`` command is given, MIMIR will fall back to the strategy specified in the RATDB. The following RATDB block does the same thing as above:

::

    {
      "name": "FIT_MIMIR",
      "index": "",
      "strategy": "FitStep",
      "strategy_config": "PositionTime_PMTTypeTimeResidual",
    }


Writing a MIMIR component
''''''''''''''''''''''''''
MIMIR components are all templated classes that requires the override of
several functions. The components can be added in either RATPAC2 itsslef or a
downstream experiment.

All components require the following to be done:

- override ``bool configure(ratdblinkptr db_link)``: this function receives a
  ratdb configuration block and correctly instatiates the component.
- register the component with the mimir framework by calling the following
  preprocessor macro: ``mimir_register_type(componettype, classname,
  "humanreadableclassname")``, where ``componenttype`` is either
  ``fitstrategy``, ``cost``, or ``optimizer`` in the ``rat::mimir`` namespace. 

Each type of component has the following additional requirements:

- **Cost**: Override the following functions:

  - ``double operator()(const ParamSet& params)``: takes in a ParamSet and 
    returns the value of the cost function for that set of parameters.

- **Optimizer**: Override the following function:

  - ``void minimizeimpl(std::function<double(const paramset&)> cost, paramset&params)``: 
    given a callable function `cost`, modifies `params` such that
    `cost` is minimized. note that the internal templating of `optimizer`
    allows this minimization routine to be used for both minimization and
    maximization via ``optimizer::minimize`` and ``optimizer::maximize``.

- **FitStrategy**:

  - Override the following functions:

    - ``void Execute(ParamSet &params)``: takes in the ``params`` as a set of
      seeds and bounds, perform the fit, and writes ther eseult back to
      ``params``.
  - During initialization, the FitStrategy also should:
    - correctly identifies the optimizers and costs associates with the
    strategy, look up the correct configuration bloccks from RATDB, and
    instantiate the components.

The Component Factory
+++++++++++++++++++++

To instantiate a MIMIR component, one should utilize the MIMIR component
factory, which provides many convenience functions for creating components
based on their type and configuration. The factory can be used as follows:

::

  Factory<Cost>::GetInstance().make_and_configure(name, index);

Where the complating typename ``Cost`` can be replaced with ``Optimizer`` or
``FitStrategy`` to create the corresponding component. The ``name`` and
``index`` are the type name and configuration index for the component,
respectively. The factory will create a component with typename ``name`` and
use the RATDB entry of ``MIMIR_name[index]`` for configuration.


Available Strategies
''''''''''''''''''''

FitStep
+++++++
``FitStep`` is the simplest fit strategies that can be used with MIMIR. It takes in an optimizer and a cost function, and runs the optimizer to minimize the cost function.

========================   ==========================  ===================
**Field**                  **Type**                    **Description**
========================   ==========================  ===================
``optimizer_name``          ``string``                  Name of the optimizer type to use.
``optimizer_config``        ``string``                  Configuration to use for the optimizer.
``cost_name``               ``string``                  Name of the cost type to use.
``cost_config``             ``string``                  Configuration to use for the cost function.
``position_time_status``    ``int`` or ``int[4]``       Status codes for ``[x, y, z, t]``, ``0 = INIACTIVE, 1 = ACTIVE, 2 = FIXED``.
``direction_status``        ``int`` or ``int[2]``       Status codes for ``[theta, phi]``
``energy_status``           ``int`` or ``int[1]``       Status codes for ``E``
``x_bound``                 ``double[2]``               Bounds for the field ``x``. Also works for all other fields: y, z, t, theta, phi, energy. 
========================   ==========================  ===================

FitSteps
+++++++

========================   ==========================  ===================
**Field**                  **Type**                    **Description**
========================   ==========================  ===================
``steps``                  ``string[]``                A list of ``FitStep`` configurations to run in order.
========================   ==========================  ===================

Available Optimizers
''''''''''''''''''''

RootOptimizer
+++++++++++++
``RootOptimizer`` is a wrapper around the ``ROOT::Math::Minimizer`` class. See the `official documentation <https://root.cern/doc/v606/classROOT_1_1Math_1_1Minimizer.html>`_ for details.

======================  ==========================  ===================
**Field**               **Type**                    **Description**
======================  ==========================  ===================
``minimizer_type``       ``string``                  Passed to ``ROOT::Math::Factory::CreateMinimizer`` as ``minimizerType``.
``algo_type``            ``string``                  Passed to ``Root::Math::Factory::CreateMinimizer`` as ``algoType``.
``max_function_calls``   ``int``                     Specifies the max number of calls to the cost function.
``max_iterations``       ``int``                     Specifies the maximum number of iterations to run the minimizer.
``tolerance``            ``double``                  Absolute tolerance for the minimizer. Detailed use may vary depending on the minimzier and algorithm used.
``print_level``          ``int``                     Verbosity level for the minimizer. 0 is silent, 1 is normal, 2 is verbose.
======================  ==========================  ===================

NLOPTOptimizer
++++++++++++++
``NLOPTOptimizer`` is a wrapper around the ``nlopt::opt`` class. See the `official documentation <https://nlopt.readthedocs.io/en/latest/>`_ for details.

This optimizer only supports **gradient-free** (derivative-free) algorithms. Gradient-based algorithms (those starting with ``LD_`` or ``GD_``) are not supported and will cause the configuration to fail with a clear error message.

======================  ==========================  ===================
**Field**               **Type**                    **Description**
======================  ==========================  ===================
``algo_type``            ``string``                  NLopt algorithm name (e.g., ``LN_COBYLA``, ``LN_NELDERMEAD``, ``LN_SBPLX``). Must be a gradient-free algorithm (``LN_*`` or ``GN_*``). See `NLopt algorithms <https://nlopt.readthedocs.io/en/latest/NLopt_Algorithms/>`_ for a full list.
``max_function_calls``   ``int``                     Maximum number of objective function evaluations allowed.
``tolerance``            ``double``                  Relative tolerance on the optimization parameters (``xtol_rel``). The optimizer stops when the change in all parameters is less than this tolerance.
======================  ==========================  ===================

Available Costs
'''''''''''''''

PMTTypeTimeResidualPDF
++++++++++++++++++++++
Evaluates a 1D time residual PDF as a negative log likelihood.

===========================    ==========================  ===================
**Field**                      **Type**                    **Description**
===========================    ==========================  ===================
``light_speed_in_medium``      ``double``                  Speed of light (in mm/ns) in the material of the detector. Used to calculate time of flight. 
``binning``                    ``double[]``                Bin centers for the time residual PDF. 
``pmt_types``                  ``int[]``                   Types PMTs to use in the fit.
``type_weights``               ``double[]``                Weights for each type of PMT.
``hist_<pmttype>``             ``double[]``                Histogram content for each type of PMT, with ``binning`` as the bin centers.
============================   ==========================  ===================

Note that the received histograms will be noramlized such that the integral in
the range specified by ``binning`` is 1.0. The negative natural logirithms of
the bin heights are then calculated and evaluated via a cubic spline. When the
computed time reisudal is out of range for the current hypothesis, the PDF will
evaluate to the left or right edge of the binning.

PMTTypeCosAlphaPDF
++++++++++++++++++++++
Evaluates a 1D CosAlpha PDF as a negative log likelihood.

===========================    ==========================  ===================
**Field**                      **Type**                    **Description**
===========================    ==========================  ===================
``light_speed_in_medium``      ``double``                  Speed of light (in mm/ns) in the material of the detector. Used to calculate time of flight. 
``binning``                    ``double[]``                Bin centers for the time residual PDF. 
``pmt_types``                  ``int[]``                   Types PMTs to use in the fit.
``type_weights``               ``double[]``                Weights for each type of PMT.
``hist_<pmttype>``             ``double[]``                Histogram content for each type of PMT, with ``binning`` as the bin centers.
``tresid_range``               ``double[2]``               Range of time residuals to use for evaluating the PDF.
============================   ==========================  ===================

