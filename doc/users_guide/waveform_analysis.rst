.. _waveform_analysis:

Waveform analysis
-----------------

It is typical for modern detectors to digitize PMT pulses on waveform digitizers and readout entire waveforms for each PMT channel. These waveforms are then often processed offline to produce PMT hit-times and integrated charges, among other variables. ``ratpac-two`` both provides realistic simulation of the waveform digitizers, discussed in :ref:`digitization`, but also a full chain of waveform analysis that can be used to extract information about the PMT pulses. As with other features, this existing waveform analysis code can easily be extended for an experiment's particular use-cases, and the provided waveform processors should be treated as helpful examples rather than finished products.

The waveform analysis processors are run over the digitized waveforms (the ``DS::Digit`` objects). These processors are only run if (a) a DAQ processor is run (b) digitization of the PMT waveforms is enabled (c) the waveform processing is enabled and (d) the waveform processor is run from the macro.

In order to enable the digitization of waveforms, discussed in :ref:`digitization`, we adjust the ``DAQ.ratdb`` table and set ``digitize: True`` for the appropriate DAQ. As with all ratdb parameters, this can be achieved from the macro. This will cause waveforms for each ``MCPMT`` to be created, which can be further processed using the waveform analysis tools.

The waveform analysis is enabled by running the waveform analysis processor (e.g., ``/rat/proc WaveformPrep``). The analysis processors reads from a ratdb table called ``DIGITIZER_ANALYSIS``. In order to change the index of the table that is loaded, the user can select a new index using ``/rat/procset index_name``. More details are provided below. 

Base analysis
`````````````

The primary waveform analysis processor is run from ``WaveformPrep``, which is critical to run prior to any other waveform processing. By default, ``WaveformPrep`` loads the ``DIGITIZER_ANALYSIS`` table with no index, which contains settings related to the pedestal window, the integration window, the voltage threshold to consider a PMT pulse, etc. ``WaveformPrep`` calculates (among other things) the integrated charge around the PMT pulse, the time-over-threshold, the voltage-over-threshold, the pedestal in a prompt-window, the peak voltage, the total charge across the full window, and the constant-fraction discriminator timing. These variables are written to the ``DigitPMT``.

Parameters in ratdb for ``Waveform Prep`` are defined below. These parameters can all be set using the standard ``/rat/procset variable_name value`` syntax.::

    voltage_threshold

* the threshold over which to count the hit as a PMT pulse. Waveforms without samples that cross this threshold can be optionally suppressed.

::

    zero_suppress

* optionally specify whether to keep the ``DigitPMT`` if the waveform never crosses threshold. By default set to 1, meaning that these below-threshold waveforms will be removed and not analyzed.

::

    pedestal_window_low
    pedestal_window_high

* the lower and upper edge of the window to calculate the baseline, in samples

::

    constant_fraction
    lookback

* parameters to calculate the timing using a constant fraction discriminator.

::

    integration_window_low
    integration_window_high

* the lower and upper edge of the window, centered around the peak of the PMT pulse, to integrate the charge, in samples.

::

    sliding_window_width
    sliding_window_thresh

* parameters to calculate a total charge by sliding an integration window across the waveform.

::

    apply_cable_offset

* apply the cable offset from the PMT channel status. More details are provided in :ref:`channel_status`.

For waveforms that cross the specified threshold, a new ``DS`` object called the ``DigitPMT`` is created. These objects represent the PMT properties as measured by the waveform analysis tools. For example, the ``GetDigitizedTime`` method returns the digitized time as measured by ``WaveformPrep``, which applies a constant-fraction-discriminator to extract a single hit-time for each waveform. Because there are several different analysis methods that might calculate a PMT hit-time, the results are separated using the ``WaveformAnalysisResult`` tool that is further described below.

There are several additional waveform analysis proccesors described below, each of which is attempting to provide a precise measurement of the arrival time for single photoelectron hits. To run the full chain of waveform analysis from the macro::

    # This is set true by default, so not
    # strictly necessary
    /rat/db/set DAQ[SplitEVDAQ] digitize true

    /run/initialize

    # The DAQ will automatically digitize hit
    # PMTs for the triggered events
    /rat/proc splitevdaq
    /rat/procset trigger_threshold 5.0

    # Always start with the waveform prep!
    /rat/proc WaveformPrep
    # If we have custom settings, we could
    # create a new DIGITIZER_ANALYSIS table
    # and load the setting here like:
    # /rat/procset analyzer_name "custom_settings"
    # Which can be done similarly for the 
    # below processors as well.

    # Then run the other waveform analysis
    # processors
    /rat/proc WaveformAnalysisLognormal
    # This automatically loads the 
    # DIGITIZER_ANALYSIS table with an
    # index of 'LognormalFit' unless we
    # select something else using
    # /rat/procset analyzer_name "custom_settings"

    /rat/proc WaveformAnalysisGaussian
    
    /rat/proc WaveformAnalysisSinc

For all of these processors, there is a utility located in ``util/src/`` called ``WaveformUtil.cc`` that provides useful analysis tools. For example, there are public methods to convert ADC counts to voltage, identify the peak of the waveform and the corresponding sample, get the total number of threshold crossings, etc.

-------------------------

.. _lognormalfit:

Lognormal fitting
`````````````````

Performs PMT waveform analysis using a lognormal distribution fit to extract timing and charge information from PMT pulses. This method fits a single lognormal function to the entire waveform around the peak region, seeded by digitTime.

The lognormal function used has the following form:

.. math::

   f(t) = \frac{A e^{-\ln\left(\frac{(t - \theta)}{m}\right)^2}}{(t-\theta)\sigma\sqrt{2\pi}}\qquad x > \theta; m, \sigma > 0

where :math:`\theta` is the time offset parameter, :math:`m` is the scale parameter, :math:`\sigma` is the shape parameter (fixed during fitting), and the magnitude parameter :math:`a` determines the pulse amplitude. The fitted time is calculated as :math:`\theta + m`, and the charge is derived from the magnitude parameter.

The method can be configured using the following ratdb parameters:

================================  ===================
**Name**                          **Description**
================================  ===================
``fit_window_low``                Time window before the digitized peak time to include in the fit, in ns.
``fit_window_high``               Time window after the digitized peak time to include in the fit, in ns.
``lognormal_shape``               The "sigma" parameter in the lognormal function controlling the pulse width.
``lognormal_scale``               The "m" parameter in the lognormal function controlling the pulse timing characteristics.
================================  ===================

-------------------------

Gaussian fitting
````````````````

Performs PMT waveform analysis using a Gaussian distribution fit to extract timing and charge information from PMT pulses. This method fits a single Gaussian function to the waveform around the peak region, seeded by digitTime.

The Gaussian function used has the following form:

.. math::

   g(t) = \frac{A e^{-\frac{(t - \mu)^2}{2\sigma^2}}}{\sigma\sqrt{2\pi}}

where :math:`\mu` is the mean (fitted time), :math:`\sigma` is the standard deviation (fitted within specified bounds), and the magnitude parameter :math:`A` determines the pulse amplitude. The fitted time is directly :math:`\mu`, and the charge is derived from the magnitude parameter.

The method can be configured using the following ratdb parameters:

================================  ===================
**Name**                          **Description**
================================  ===================
``fit_window_low``                Time window before the digitized peak time to include in the fit, in ns.
``fit_window_high``               Time window after the digitized peak time to include in the fit, in ns.
``gaussian_width``                Initial value for the Gaussian width (sigma) parameter, in ns.
``gaussian_width_low``            Lower bound for the fitted Gaussian width parameter, in ns.
``gaussian_width_high``           Upper bound for the fitted Gaussian width parameter, in ns.
================================  ===================

-------------------------

Sinc interpolation
``````````````````

Describe sinc interpolation.


-------------------------

Richardson-Lucy Direct De-modulation (LucyDDM)
``````````````````

Performs PMT waveform analysis using Perform PMT waveform analysis using LucyDDM, a time-domain deconvolution algorithm that enhanced resolution compared to FFT-based convolution methods. This method is able to recosntruction multiple photoelectrons in a single PMT. The primary procedure is described in Sec. 3.3.2 of https://arxiv.org/abs/2112.06913. After deconvolution, peaks in the deconvolved waveform are identified, and a Gaussian fit is performed on each peak to extract time and charge information. Optionally, a likelihood-based NPE estimation can be performed on each resolved wave packet to further improve the charge and time resolution.

The method can be configured using the following ratdb parameters.


================================  ===================
**Name**                          **Description**
================================  ===================
``vpe_scale``                     The "m" parameter in the lognormal function that generates single PE waveform template.
``vpe_shape``                     The "sigma" parameter in the lognormal function.
``vpe_charge``                    The nominal charge of a single photoelectron in pC. A resolved wave packet with integral of 1 will have be assigned this amount of charge.
``roi_threshold``                 The threshold (in mV) above which the deconvolution will be performed on. All samples below threshold will be set to effectively zero (small positive value for numerical stability).
``max_iterations``                Maximum number of LucyDDM iterations to run.
``stopping_nll_diff``             Stop LucyDDM iterations if the negative log likelihood improvement is less than this value.
``peak_height_threshold``         All peaks below this peak height in the deconvolved waveform will not be considered.
``charge_threshold``              All wave packets with integrated charge below this threshold will not be considered.
``min_peak_distance``             If two resolved wave packets are closer than this distance (in ns), they will be merged and considered as one wave packet. 
``npe_estimate``                  If true, perform a NPE estimation on all resolved waveform packets using a likelihood on the integral of the packets.
``npe_estimate_charge_width``     Width of the single PE charge distribution (in pC) used in the NPE estimation likelihood.
``npe_estimate_max_pes``          Maximum number of PEs to consider in the NPE estimation likelihood.
================================  ===================

-------------------------

WaveformAnalysisResult
``````````````````````

The ``WaveformAnalysisResult`` class is a data structure that stores the output from waveform analysis processors. Each waveform analysis method creates a ``WaveformAnalysisResult`` object that is attached to the ``DigitPMT`` to store its specific analysis results. This design allows multiple analysis methods to be run on the same waveform, with each method's results stored separately and accessible independently.

The ``WaveformAnalysisResult`` object maintains three parallel arrays that are automatically sorted by time:

* **Times**: The reconstructed pulse times within the digitization window (no cable delay or trigger offset applied)
* **Charges**: The corresponding pulse charges, nominally in units of pC
* **Figures of Merit**: Additional analysis-specific metrics stored in a map structure

Key features of the ``WaveformAnalysisResult`` include:

**Multiple PE Support**: Unlike the basic ``DigitPMT`` analysis which typically identifies a single PE per waveform, ``WaveformAnalysisResult`` can store multiple reconstructed photoelectrons.

**Automatic Time Ordering**: When pulses are added using ``AddPE()``, they are automatically inserted in the correct time-ordered position, ensuring that all arrays remain synchronized and sorted.

**Flexible Figures of Merit**: Each analysis method can store method-specific quality metrics (e.g., chi-squared, fit width, baseline) that are automatically synchronized with the time and charge arrays.

**Time Offset Handling**: The class supports time offset corrections to account for cable delays or trigger timing, which can be applied when retrieving results without affecting the stored raw timing.

The ``WaveformAnalysisResult`` objects are accessed from the ``DigitPMT`` using the method name as a key::

    DS::WaveformAnalysisResult* lognormal_result = digitpmt->GetWaveformAnalysisResult("Lognormal");
    DS::WaveformAnalysisResult* gaussian_result = digitpmt->GetWaveformAnalysisResult("Gaussian");

    int n_pes = lognormal_result->getNPEs();

    for (int i = 0; i < n_pes; i++) {
        double time = lognormal_result->getTime(i);
        double charge = lognormal_result->getCharge(i);
        double chi2 = lognormal_result->getFOM("chi2ndf", i);  // Method-specific FOM
    }

This design enables comprehensive comparison between different waveform analysis methods and supports multi-PE reconstruction techniques.

