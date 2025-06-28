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

WaveformAnalysisResult
``````````````````````

Describe how the waveform analysis result works.

