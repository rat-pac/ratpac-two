.. _waveform_analysis:

Waveform analysis
-----------------

It is typical for modern detectors to digitize PMT pulses on waveform digitizers and readout entire waveforms for each PMT channel. These waveforms are then often processed offline to produce PMT hit-times and integrated charges, among other variables. ``ratpac-two`` both provides realistic simulation of the waveform digitizers, discussed in :ref:`digitization`, but also a full chain of waveform analysis that can be used to extract information about the PMT pulses. As with other features, this existing waveform analysis code can easily be extended for an experiment's particular use-cases, and the provided waveform processors should be treated as helpful examples rather than finished products.

The waveform analysis processors are run over the digitized waveforms (the ``DS::Digit`` objects). These processors are only run if (a) a DAQ processor is run (b) digitization of the PMT waveforms is enabled (c) the waveform processing is enabled and (d) the waveform processor is run from the macro.

In order to enable the digitization of waveforms, discussed in :ref:`digitization`, we adjust the ``DAQ.ratdb`` table and set ``digitize: True`` for the appropriate DAQ. As with all ratdb parameters, this can be achieved from the macro. This will cause waveforms for each ``MCPMT`` to be created, which can be further processed using the waveform analysis tools.

The waveform analysis is enabled by running the waveform analysis processor (e.g., ``/rat/proc WaveformPrep``). The analysis processors reads from a ratdb table called ``DIGITIZER_ANALYSIS``. In order to change the index of the table that is loaded, the user can select a new index using ``/rat/procset index_name``. More details are provided below. 

.. _base_analysis:

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
    
    /rat/proc WaveformAnalysisRAVEN

    /rat/proc WaveformAnalysisGreedyMP

    /rat/proc WaveformAnalysisFSMP

For all of these processors, there is a utility located in ``util/src/`` called ``WaveformUtil.cc`` that provides useful analysis tools. For example, there are public methods to convert ADC counts to voltage, identify the peak of the waveform and the corresponding sample, get the total number of threshold crossings, etc.

.. _common_parameters:

Common parameters
`````````````````

All waveform analysis processors inherit from ``WaveformAnalyzerBase`` and share the following parameters, which can be set using ``/rat/procset``::

    min_total_charge
    max_total_charge

* Lower and upper bounds (in pC) on the digitized total charge of the waveform (``DigitPMT::GetDigitizedTotalCharge()``). If the total charge falls outside ``[min_total_charge, max_total_charge]``, the analysis for that channel is skipped entirely. By default these are set to the most negative and most positive finite ``double`` values respectively, so all waveforms are analyzed unless a cut is explicitly specified. A skipped channel still carries an empty ``WaveformAnalysisResult`` for the analyzer (``getNPEs() == 0``); see :ref:`fitter_handler` for how the reconstruction processors treat such channels.

For example, to restrict analysis to waveforms with total charge between -5 pC and 50 pC::

    /rat/proc WaveformAnalysisLognormal
    /rat/procset min_total_charge -5.0
    /rat/procset max_total_charge 50.0

These cuts are applied per-channel, per-event, before ``DoAnalysis()`` is called, so they are a lightweight way to skip waveforms that are clearly noise or saturated without running the full analysis.

An event-level gate skips the whole event, for every channel, before any of them is analyzed::

    max_event_charge
    max_event_mean_crossings
    max_event_negative_fraction

* The event is skipped if the digitized charge summed over the PMT channels exceeds ``max_event_charge`` (pC), if the mean number of threshold crossings per channel that crosses exceeds ``max_event_mean_crossings``, or if the fraction of PMT channels with negative digitized charge exceeds ``max_event_negative_fraction``. All three are off by default. They read the charge and crossings ``WaveformPrep`` stores, so ``WaveformPrep`` must run first. A skipped event still carries an empty result on every PMT, as a channel outside the total-charge cut does.

The gate is meant for the sampling analyzers on data: muons and detector-wide baseline oscillations each cost FSMP seconds per event, and an oscillation comes back as thousands of fake PEs. Both cross threshold many times per channel, and an oscillation leaves most channels with negative charge, while light from a low-energy event does neither. Set it after ``analyzer_name``, which re-reads the table::

    /rat/proc WaveformAnalysisFSMP
    /rat/procset analyzer_name "MyFSMP"
    /rat/procset max_event_charge 2000.0
    /rat/procset max_event_mean_crossings 3.0
    /rat/procset max_event_negative_fraction 0.5

-------------------------

.. _lognormalfit:

Lognormal fitting
`````````````````

Performs PMT waveform analysis using a lognormal distribution fit to extract timing and charge information from PMT pulses. This method fits a single lognormal function to the entire waveform around the peak region, seeded by digitTime.

The lognormal function used has the following form:

.. math::

   f(t) = \frac{A e^{-\frac{\left(\ln\left((t - \theta)/m\right)\right)^2}{2\sigma^2}}}{(t-\theta)\sigma\sqrt{2\pi}}\qquad x > \theta; m, \sigma > 0

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

.. _gaussianfit:

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

.. _sincinterpolation:

Sinc interpolation
``````````````````

Describe sinc interpolation.


-------------------------

.. _lucyddm:

Richardson-Lucy Direct De-modulation (LucyDDM)
``````````````````

Perform PMT waveform analysis using LucyDDM, a time-domain deconvolution algorithm that enhanced resolution compared to FFT-based convolution methods. This method is able to recosntruction multiple photoelectrons in a single PMT. The primary procedure is described in Sec. 3.3.2 of https://arxiv.org/abs/2112.06913. After deconvolution, peaks in the deconvolved waveform are identified, and a Gaussian fit is performed on each peak to extract time and charge information. Optionally, a likelihood-based NPE estimation can be performed on each resolved wave packet to further improve the charge and time resolution.

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

.. _raven:

RAVEN
`````

Performs PMT waveform analysis using RAVEN (Reverse Analysis of Voltage Events with Nonnegativity), a sparse non-negative least squares fitting algorithm that reconstructs multiple photoelectron times and charges from digitized waveforms. The underlying algorithm (rsNNLS) is described in Sec. 3.1.1 of https://www.sciencedirect.com/science/article/pii/S0925231211006370. The method builds a dictionary matrix of time-shifted single PE templates at sub-sample resolution, applies non-negative least squares to find optimal template weights, and iteratively removes low-significance components to improve sparsity. Because that pass only removes components, a mistimed component is optionally refined afterwards by moving it to a neighbouring dictionary column when the residual improves. After weight merging, NPE estimation can be performed on resolved peaks. Two template types are supported: lognormal (asymmetric) and Gaussian (symmetric). A detector mixing PMT models whose single-electron responses have different widths needs one template per model, so the template shape is taken per PMT from the same ``PMTPULSE`` table the waveform generator samples, scaled by the channel's ``pulse_width_scale`` calibration. Where ``PMTPULSE`` gives the Gaussian width as a distribution, RAVEN uses its median.

The method can be configured using the following ratdb parameters:

================================  ===================
**Name**                          **Description**
================================  ===================
``process_threshold_crossing``    Enable region-based processing (0=no, 1=yes).
``voltage_threshold``             Voltage threshold for region detection, in mV.
``region_padding``                Number of samples to pad around threshold crossing regions.
``raven_template_type``           Template type: 0=lognormal, 1=gaussian.
``lognormal_scale``               Lognormal "m" parameter (used when raven_template_type=0).
``lognormal_shape``               Lognormal "sigma" parameter (used when raven_template_type=0).
``gaussian_width``                Gaussian sigma parameter (used when raven_template_type=1).
``vpe_charge``                    Nominal charge of a single PE in pC.
``upsampling_factor``             Dictionary upsampling factor for sub-sample timing resolution.
``max_iterations``                Maximum iterative thresholding iterations.
``nnls_tolerance``                NNLS convergence tolerance.
``width_from_pmtpulse``           If true, take the template shape per PMT from ``PMTPULSE``, scaled by the channel's ``pulse_width_scale``. Must be a boolean.
``refine_times``                  If true, move surviving components to nearby times when that lowers the residual. Must be a boolean.
``weight_threshold``              Minimum weight for component significance.
``weight_merge_window``           Time window (ns) for merging nearby weights. Set to 0 to disable.
``npe_estimate``                  If true, perform NPE estimation on resolved wave packets.
``npe_estimate_charge_width``     Width of single PE charge distribution (in pC) for NPE estimation.
``npe_estimate_max_pes``          Maximum number of PEs to consider in NPE estimation.
================================  ===================

-------------------------

GreedyMP
````````

Performs PMT waveform analysis by greedy Bayesian matching pursuit, the search used by fast Bayesian matching pursuit (Schniter, Potter and Ziniel, Information Theory and Applications Workshop 2008). The waveform is modelled as a sparse spike train of PEs convolved with a single PE template plus Gaussian white noise. For a configuration :math:`z` of occupied points on a sub-sample time grid, the per-PE charges are integrated out analytically to give a multivariate Gaussian evidence :math:`p(w|z)`; the analyzer then builds a configuration one atom at a time, at each step correlating every unoccupied dictionary column with the current fit residual, scoring the best-correlated handful with the full evidence, and keeping whichever most improves it net of a per-PE sparsity penalty. Only that shortlist is scored with the expensive evidence solve, which is what makes the search affordable on a grid ``upsampling_factor`` times denser than the digitizer samples. Two template types are supported: lognormal (asymmetric) and Gaussian (symmetric).

GreedyMP and FSMP below use the same dictionary, regions of interest, evidence and PE emission, and differ only in how they search over configurations. Running both on the same waveforms therefore compares search strategies at identical settings, and FSMP is designed to take GreedyMP's output as its starting point.

The method can be configured using the following ratdb parameters:

================================  ===================
**Name**                          **Description**
================================  ===================
``process_threshold_crossing``    Enable region-based processing (0=no, 1=yes).
``voltage_threshold``             Voltage threshold for region detection, in mV.
``region_padding``                Number of samples to pad around threshold crossing regions.
``template_type``                 Template type: 0=lognormal, 1=gaussian, 2=from PMTPULSE. See below.
``lognormal_scale``               Lognormal "m" parameter (used when template_type=0).
``lognormal_shape``               Lognormal "sigma" parameter (used when template_type=0).
``gaussian_width``                Gaussian sigma parameter (used when template_type=1).
``gaussian_width_pmt_types``      Optional PMT types with their own template width. Paired with the next entry.
``gaussian_width_pmt_widths``     Gaussian sigma for each listed PMT type. Unlisted types use ``gaussian_width``.
``apply_pulse_width_scale``       Stretch the template by ChannelStatus' per-channel ``pulse_width_scale``. Off by default.
``vpe_charge``                    Nominal charge of a single PE in pC.
``vpe_charge_pmt_types``          Optional PMT types with their own single PE charge. Paired with the next entry.
``vpe_charge_pmt_charges``        Single PE charge (pC) for each listed PMT type. Unlisted types use ``vpe_charge``.
``upsampling_factor``             Dictionary upsampling factor for sub-sample timing resolution.
``noise_sigma``                   Gaussian white noise sigma of the waveform, in mV. Must be positive.
``apply_charge_scale``            Fit each waveform against the template scaled by ChannelStatus' per-channel ``charge_scale``. Off by default.
``gamma_k``                       Shape of the per-PE charge prior. Paper value is 1/0.4^2.
``gamma_theta``                   Scale of the per-PE charge prior. Paper value is 0.4^2.
``gamma_pmt_types``               Optional PMT types with their own charge prior. Paired with the next entry.
``gamma_pmt_rel_sigma``           Relative charge prior width for each listed PMT type. Unlisted types use ``gamma_k``/``gamma_theta``.
``seed_analyzer``                 Name of another analyzer's result to take the initial configuration from. Empty to search every region.
``max_iterations``                Maximum number of PEs per region.
``greedy_shortlist``              Number of best-correlated columns scored with the full evidence per step.
``weight_merge_window``           Time window (ns) for merging nearby atoms. Set to 0 to disable.
``npe_estimate``                  If true, perform NPE estimation on resolved atoms.
``npe_estimate_charge_width``     Width of single PE charge distribution (in pC) for NPE estimation.
``npe_estimate_max_pes``          Maximum number of PEs to consider in NPE estimation.
================================  ===================

Single PE response templates
````````````````````````````

The dictionary both analyzers fit with is the single PE response delayed onto an upsampled time grid, and how faithfully it describes the detector sets how many atoms the fit needs to explain a pulse. A template narrower than the true SER is paid for with spurious atoms; a wider one merges PEs that could have been resolved.

``template_type`` chooses where that shape comes from:

* ``0`` --- a lognormal of ``lognormal_scale`` and ``lognormal_shape``, the same shape for every channel.
* ``1`` --- a gaussian of ``gaussian_width``, optionally per PMT type through ``gaussian_width_pmt_types`` / ``gaussian_width_pmt_widths``.
* ``2`` --- whatever ``PMTPULSE`` says for each PMT's model, i.e. the table the simulation generated the pulse from. A ``datadriven`` table is interpolated directly. An analytic gaussian is **marginalised over its measured width distribution**, since the simulation draws a fresh width per PE and no single gaussian reproduces that mixture. An analytic lognormal uses its ``lognormal_mean`` and ``lognormal_width``.

Prefer ``2`` wherever ``PMTPULSE`` describes the detector: no width has to be transcribed into the analysis table, the fit follows the pulse table if it is ever refit, and it is the only option that keeps the per-PE width spread. That spread matters when it is broad --- for Eos's ``r7081_hqe`` it is 23% of the mean width.

``apply_pulse_width_scale`` additionally stretches the template by the per-channel ``pulse_width_scale`` calibration, matching what ``PMTWaveformGenerator`` applies when it builds the pulse. Scales are bucketed to 5% before they reach the dictionary cache, since one dictionary per channel would not fit in memory.

``vpe_charge`` and the charge prior take the same per-PMT-type treatment, which matters in a detector whose PMT models have different gains. Note that ``vpe_charge`` only moves the prior: the dictionary scales by it and the fitted weight by its inverse, so the reported charge is invariant. The per-type prior is spelled as a relative width, ``gamma_pmt_rel_sigma``, which fixes the prior mean at exactly one PE --- the natural form once ``vpe_charge`` is itself per type.

All of this lives in ``RAT::SERDictionary``, shared by both analyzers so that a seed and the sampler that starts from it always describe the same detector.

-------------------------

FSMP
````

Performs PMT waveform analysis using FSMP (Fast Stochastic Matching Pursuit), the Bayesian method of Xu et al. 2022, JINST 17 P06040 (https://arxiv.org/abs/2112.06913). It shares the sparse PE model, dictionary and evidence described under GreedyMP above, and differs in how it searches: rather than adding atoms greedily, it explores configurations with a Metropolis-Hastings-within-Gibbs sampler that makes birth, death and shift moves on :math:`z` jointly with the light curve time :math:`t_0`. The paper describes this as replacing the greedy search of FBMP with stochastic sampling; here that greedy search is the separate GreedyMP processor.

Because the sampler carries :math:`t_0` and the intensity :math:`\mu`, FSMP reports estimates of the incident light itself rather than only the individual PEs. The sampler runs once per waveform across all regions, so these are per-PMT quantities repeated on every PE of that waveform:

* ``fsmp_t0`` --- posterior mean light curve time, in the same time base as the reconstructed PE times.
* ``fsmp_mu`` --- importance-reweighted intensity maximum likelihood estimate (paper eqs. 3.25-3.26).
* ``fsmp_npe`` --- posterior mean number of PEs. More robust than ``fsmp_mu`` when the chain visits a wide range of PE counts.
* ``chi2ndf`` --- goodness of fit of the reconstructed waveform, normalized by ``noise_sigma``, over all regions of the waveform.

The Markov chain needs somewhere to start, which ``seed_analyzer`` supplies from another analyzer's result --- the role LucyDDM plays in the paper, and what GreedyMP is there for::

    /rat/proc WaveformAnalysisGreedyMP

    /rat/proc WaveformAnalysisFSMP
    /rat/procset seed_analyzer "GreedyMP"

The seed analyzer must appear **before** FSMP in the processor chain, since FSMP reads the ``WaveformAnalysisResult`` it left on the ``DigitPMT``. Any analyzer that reports PE times will do --- RAVEN and LucyDDM work equally well. The seed only sets where the chain starts, so a seed analyzer that misses or invents a PE is recoverable: the sampler can still add or remove PEs anywhere in the regions. Repeated PEs at the same time (as produced by an analyzer running ``npe_estimate``) collapse to a single occupied grid point, since the model allows at most one PE per grid point. If the named result is missing FSMP warns once and starts from an empty configuration, which mixes poorly --- the chain then has to discover every PE unaided.

FSMP takes the same parameters as GreedyMP, minus ``greedy_shortlist``, plus the following:

================================  ===================
**Name**                          **Description**
================================  ===================
``n_mcmc_samples``                Number of post-burn-in samples recorded.
``burn_in``                       Number of warm-up samples discarded.
``lightcurve_tau``                Light curve exponential time constant, in ns. 0 gives a pure Gaussian.
``lightcurve_sigma``              Light curve timing spread, in ns. Mainly PMT transit time spread.
``lightcurve_floor``              Fraction of the arrival time prior spread flat across the readout window. 0 by default.
``lightcurve_taus``               Optional scintillator decay constants, in ns. When set, replaces ``lightcurve_tau``.
``lightcurve_weights``            Relative weights of ``lightcurve_taus``, normalised on read. Same length.
``lightcurve_rise``               Rise time shared by every ``lightcurve_taus`` component, in ns. 0 by default.
``lightcurve_prompt``             Fraction of the light with no delay after t0 (Cherenkov). 0 by default.
``t0_step``                       Random walk proposal step for t0, in ns.
``store_expected_waveform``       Whether to keep the expected waveform of the MAP configuration. Off by default.
``pileup_split``                  Recover PEs hidden inside one atom from the region's total charge. Off by default.
``pileup_split_threshold``        Log likelihood gain the charge must show before ``pileup_split`` adds a PE. 2 by default.
``mu_prior``                      Occupancy prior intensity: 0 the seeded PE count (default), 1 the charge, 2 the larger.
================================  ===================

Setting ``store_expected_waveform`` attaches the waveform FSMP's MAP configuration predicts --- its PEs folded back through the single PE response, in mV, pedestal-subtracted, one entry per digitizer sample --- to the ``WaveformAnalysisResult``. This is the model behind ``chi2ndf``, taken before ``weight_merge_window`` and ``npe_estimate`` reshape the atoms for output, so it shows what the fit actually settled on rather than what was reported. ``outntuple`` writes it to the ``waveforms`` tree as ``expected_waveform_FSMP`` alongside the observed waveform (see :doc:`outputs`), which requires ``include_digitizerwaveforms``::

    /rat/proc WaveformAnalysisFSMP
    /rat/procset store_expected_waveform 1

    /rat/proclast outntuple
    /rat/procset include_digitizerwaveforms 1

It costs an array of ``nsamples`` doubles per analyzed PMT per event, on top of the digitized waveforms themselves, so it is meant for inspecting fits rather than for production running.

The sampler draws from its own random number generator, seeded once per run from the global CLHEP engine, so ``rat -s`` reproduces an FSMP run exactly. The generator is private to the analyzer, so enabling FSMP does not consume randomness from the simulation and does not change the events it produces.

``lightcurve_tau`` and ``lightcurve_sigma`` describe the expected arrival time profile of the light and should match the detector medium: set ``lightcurve_tau`` to the effective scintillation decay constant of the fill, or to 0 for a Cherenkov-dominated (e.g. water) fill, and ``lightcurve_sigma`` to the PMT transit time spread. These only affect the sampler, but a badly mismatched ``lightcurve_tau`` degrades ``fsmp_t0`` substantially.

Transit time spread is a lower bound on ``lightcurve_sigma``, not the whole of it: the curve also has to absorb photon time of flight across the photocathode and the finite size of the source. Narrowing it to a measured PMTTRANSIT prompt peak has been seen to cost PE finding efficiency outright.

``lightcurve_floor`` mixes a flat component into that profile. The light curve describes the prompt light only, and it falls off fast enough that a PE in the PMTTRANSIT late tail, or a dark noise PE anywhere in the window, is given an occupancy prior of order 1e-300 --- the sampler cannot accept one however well it fits the waveform. A small floor makes those reachable. Treat it as a nuisance parameter to fit rather than a measured fraction: the flat part is proportional to :math:`\mu` in this model, which dark noise is not.

``lightcurve_taus`` describes a scintillator more closely than the single ex-Gaussian. The curve is then the emission profile GLG4Scint samples --- for each component, :math:`(e^{-t/d} - e^{-t/r})/(d - r)` with decay :math:`d` from ``lightcurve_taus`` and rise :math:`r` = ``lightcurve_rise`` --- weighted by ``lightcurve_weights``, convolved with a Gaussian of ``lightcurve_sigma``, and mixed with a pure Gaussian of fraction ``lightcurve_prompt`` for Cherenkov light. Arrays cannot be set with ``/rat/procset``; set them in the table with ``/rat/db/set`` before ``/run/initialize``. A rise of 0 and one decay is the ex-Gaussian of ``lightcurve_tau``. The shape is best fit to MC truth arrival times of the fill, since the PMT transit time spread, time of flight and reflections all enter it.

``pileup_split`` and ``mu_prior`` address PEs that arrive too close together to separate in time, which on a bright channel is most of them: Cherenkov PEs on one channel arrive within about 1 ns. The sampler then resolves fewer atoms than there are PEs. ``pileup_split`` counts them from the charge instead: one PE's charge follows the Gamma charge prior, so N PEs sum to :math:`\Gamma(N k, \theta)`, and when the region's summed weight prefers more PEs than atoms by more than ``pileup_split_threshold`` in log likelihood, the extra PEs go one at a time to the atom carrying the most charge per PE. It has no effect when ``npe_estimate`` is on. ``mu_prior`` sets the intensity the occupancy prior scales with. The seeded PE count, the default, is low on exactly those channels, and the prior then charges the sampler for every PE it adds; 2 takes the larger of the seed count and the charge.

-------------------------

Shared notes
````````````

``noise_sigma`` should match the digitizer noise model (the ``noise_amplitude`` entry of the ``DIGITIZER`` table, or the pedestal RMS for real data), since it sets the scale of the evidence and therefore how readily either analyzer adds a PE.

On data, set ``apply_charge_scale``: a PE on a channel carries ``vpe_charge`` times that channel's ``charge_scale``, while the template is built per PMT type, so without it a single PE fits with a weight equal to the gain and the charge prior is centred off one PE. The waveform is fit divided by the gain at ``noise_sigma`` divided by the gain, which is equivalent to scaling the template; regions are still found on the recorded waveform and the stored expected waveform is in recorded mV. In simulation ``charge_scale`` is 1 and the setting changes nothing.

``weight_merge_window`` and ``npe_estimate`` default off for both GreedyMP and FSMP, unlike RAVEN. Splitting a resolved atom's charge into integer PEs reintroduces the single PE charge variance that resolving PEs individually is meant to remove.

GreedyMP and FSMP each read their own ``DIGITIZER_ANALYSIS`` index, selected with ``/rat/procset analyzer_name``. Give them separate indices rather than sharing one with RAVEN: they read several keys under the same names but want different values, ``npe_estimate`` and ``weight_merge_window`` above among them.

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

    DS::WaveformAnalysisResult* lognormal_result = digitpmt->GetOrCreateWaveformAnalysisResult("Lognormal");
    DS::WaveformAnalysisResult* gaussian_result = digitpmt->GetOrCreateWaveformAnalysisResult("Gaussian");

    int n_pes = lognormal_result->getNPEs();

    for (int i = 0; i < n_pes; i++) {
        double time = lognormal_result->getTime(i);
        double charge = lognormal_result->getCharge(i);
        double chi2 = lognormal_result->getFOM("chi2ndf", i);  // Method-specific FOM
    }

This design enables comprehensive comparison between different waveform analysis methods and supports multi-PE reconstruction techniques.

