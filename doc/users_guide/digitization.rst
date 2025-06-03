.. _digitization:

Digitization
------------

In ratpac-two, the user has the option to digitize waveforms for each photodetector channel according to a set of user defined settings. The settings are specified in ``DIGITIZER.ratdb`` and are separated for different digitizer types. Settings for two common CAEN digitizers, the V1730 and V1742, are provided as examples.

The digitization of PMT pulses is triggered by certain DAQ code once the specified conditions are met. As an example, the ``forcedtrigger`` processor, described in more detail in :ref:`daq_processors`, will digitize hit channels for every MC event, given that the value in the appropriate ``DAQ.ratdb`` table is specified as: ``digitize: True``. Also specified in the ``DAQ.ratdb`` table is which type of digitizer to select (e.g., V1730 or V1742), with V1730 being the typical default model.

When the digitization is enabled, a PMT pulse will be generated for every ``mcPE``, detailed in :ref:`pmt_pulse`. A fixed window is selected and the PMT pulse, with Gaussian noise added, is sampled across that window. The voltage (or ADC counts) as a function of time (in steps according to the sampling rate) is used to create a digitized waveform for each PMT. This waveform is written to the ``DS::Digit`` object. The waveforms can also be added to the ntuple output by setting ``include_digitizerwaveforms: true`` in ``IO.ratdb``.

These waveforms can be further processed in ratpac-two in order to produce additional information, such as the integrated charge or the time-over-threshold, by applying the waveform analysis. The waveform analysis will open the ``DS::Digit`` to extract the waveforms and will process those waveforms according to user-specified criteria. The output of the waveform analysis will fill the ``DS::DigitPMT`` object. More details on the waveform analysis are provided in :ref:`waveform_analysis`.
