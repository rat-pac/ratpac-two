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
    
    /rat/proc WaveformAnalysisRSNNLS

For all of these processors, there is a utility located in ``util/src/`` called ``WaveformUtil.cc`` that provides useful analysis tools. For example, there are public methods to convert ADC counts to voltage, identify the peak of the waveform and the corresponding sample, get the total number of threshold crossings, etc.

-------------------------

.. _lognormalfit:

Lognormal fitting
`````````````````

Describe lognormal fits.

-------------------------

Gaussian fitting
````````````````

Describe Gaussian fits.

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

Reverse Sparse Non-Negative Least Squares (rsNNLS)
```````````````````````````````````````````````````

Performs PMT waveform analysis using reverse sparse non-negative least squares (rsNNLS) to reconstruct multiple photoelectron times and charges from digitized waveforms. The primary procedure is described in Sec. 3.1.1 of https://www.sciencedirect.com/science/article/pii/S0925231211006370.

The rsNNLS algorithm operates by:

1. **Dictionary Construction**: Building a matrix of time-shifted single photoelectron templates sampled at sub-nanosecond resolution through upsampling
2. **Region Processing**: Optionally identifying threshold-crossing regions for computational efficiency
3. **NNLS Fitting**: Applying non-negative least squares to find optimal template weights
4. **Iterative Thresholding**: Removing low-significance components and redistributing weights to improve sparsity
5. **PE Extraction**: Converting significant weights to photoelectron times and charges

The method supports two template types for single photoelectron waveforms:

* **Lognormal**: Asymmetric pulse shape
* **Gaussian**: Symmetric pulse shape

The method can be configured using the following ratdb parameters:

======================================  ===================
**Name**                                **Description**
======================================  ===================
``process_threshold_crossing``          Enable region-based processing (0=no, 1=yes). When enabled, only processes waveform regions that cross the voltage threshold.
``voltage_threshold``                   Voltage threshold for region detection, in mV. Used when process_threshold_crossing is enabled.
``rsnnls_template_type``                Template type: 0=lognormal, 1=gaussian.
``lognormal_scale``                     The "m" parameter in the lognormal template (used when rsnnls_template_type=0).
``lognormal_shape``                     The "sigma" parameter in the lognormal template (used when rsnnls_template_type=0).
``gaussian_width``                      The "sigma" parameter in the Gaussian template (used when rsnnls_template_type=1).
``vpe_charge``                          Nominal charge of a single photoelectron in pC. Used for template normalization and charge conversion.
``upsampling_factor``                   Dictionary upsampling factor for sub-sample resolution. Higher values provide better timing resolution at increased computational cost.
``max_iterations``                      Maximum number of iterative thresholding iterations.
``nnls_tolerance``                      Convergence tolerance for the NNLS algorithm.
``weight_threshold``                    Minimum weight threshold for component significance. Components below this threshold are removed during iterative thresholding.
======================================  ===================

-------------------------

WaveformAnalysisResult
``````````````````````

Describe how the waveform analysis result works.

