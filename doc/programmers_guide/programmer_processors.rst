.. _programming_a_processor:

Creating a Processor
''''''''''''''''''''

Creating a new processor and adding it to RAT requires only a few steps.

Historically processors are named "RAT::XXXXProc" where XXXX is some short descriptive name for what your processor does.  If the processor is a fitter, it should be named "RAT::FitXXXXProc". Currently, not all processors follow this naming scheme. All processors are subclasses of the RAT::Processor class, which defines the common interface for processors. The easiest way to create a processor class which follows this interface is to copy the CountProc files in ``src/core`` and edit them. Processors live in several of the ratpac-two subdirectories and are primarily organized by topic. For example, the reconstruction processors all live in ``src/fit``. 

In the header file, you find the new processor class (in this example, we call it ``NewProc``) intialized as part of the `RAT` namespace as follows::

  // Header guards 
  #ifndef __RAT_NewProc__
  #define __RAT_NewProc__
  
  #include <RAT/Processor.hh>
  
  namespace RAT {
  
  class NewProc : public Processor {
   public:
    // Constructor
    NewProc();
  
    // Destructor
    virtual ~NewProc();

Then from the constructor we set the processor name that gets called from the macro::

    NewProc::NewProc() : Processor("new_processor") {}

From the ``BeginOfRun`` method we can load any database paramters, setup variables, and prepare the processor. This runs once at the beginning of the run, prior to any events being processed. As an example, for the noise processors loads parameters from the database in ``BeginOfRun``::

  void NoiseProc::BeginOfRun(DS::Run *run) {
    DBLinkPtr lnoise = DB::Get()->GetLink("NOISEPROC");
  
    fNoiseFlag = lnoise->GetI("noise_flag");
    fDefaultNoiseRate = lnoise->GetD("default_noise_rate");
    ... 
    
    DS::PMTInfo *pmtinfo = run->GetPMTInfo();
    UpdatePMTModels(pmtinfo);
    ...
  }

In ``BeginOfRun`` the run object is passed, which allows users to grab the ``DS::PMTInfo`` or ``DS::ChannelStatus``, as per the example above. In this example, the noise processor uses the ``PMTInfo`` to generate necessary information prior to the processor running.

Next, you need to decide whether you want your processor to be invoked once per physics event or once per detector event.  If you are interested in Monte Carlo (MC) information primarily, or need to consider all the detector events as a group, you should overload the ``DSEvent`` method::

    Processor::Result DSEvent(DS::ROOT &ds);

As an example, the DAQ processors use the MC information to generate information about the event and decide whether to issue a trigger and create a detector event. Therefore the DAQ processors all overload the ``DSEvent`` method::

    Processor::Result SplitEVDAQProc::DSEvent(DS::Root *ds);

If you are writing a processor that is primarily interested in detector (triggered)  events, it may be easier to instead overload the ``Event`` method instead::

    Processor::Result Event(DS::Root *ds, DS::EV *ev);

``Event`` will be called once for every "detector" event, even if there are multiple detector events in a particular physics event.  The ``Event`` method is only provided as a convenience, since you could implement the same behavior by writing your own loop in ``DSEvent`` instead::

  for (int i = 0; i < ds->GetEVCount(); i++) {
     DS::EV *ev = ds->GetEV(i);
     // Process events here
  }

You should only overload ``DSEvent`` or ``Event``, but NOT BOTH.

Next you need to decide what parameters your processor will accept at runtime.  Many processors will not need this feature at all. If your processor needs constants or other external data to function, you should probably put them into a RATDB table.  Users can override RATDB values using the `/rat/db/set` command in their macro files. If you do want parameters that can be set using `/rat/procset`, you will need to select names for them and decide what type of data you want. Currently, processors can accept parameters in int, float, and double, and string format by overloading the appropriate methods::

    virtual void SetI(std::string param, int value);
    virtual void SetF(std::string param, float value);
    virtual void SetD(std::string param, double value);
    virtual void SetS(std::string param, std::string value);

Only overload the methods you need.

Finally, the ``EndOfRun`` method is invoked once after all of the events have been processed, and can be used to clean-up variables or print summary statistics.

---------------------------

Write the Class Implementation
``````````````````````````````
When you implement your class, you should take a look at CountProc.cc for an example of how to implement the ``DSEvent`` and ``SetI`` methods.

The return value of ``DSEvent`` and ``Event`` both have the same meaning:

* ``Processor::OK`` - This event was successfully processed

* ``Processor::FAIL`` - A non-fatal error has occurred.  This event will continue to be processed through the event loop, but a later processor may use this information to change its behavior.

* ``RATProcessor::ABORT`` - A non-recoverable error with this event has occurred. If a processor returns this value, then the processing of this event immediate stops, and the event loop starts over with the next event, if any.

If you are implementing one of the parameter methods, you should use this general pattern::

    void CountProc::SetI(std::string param, int value) {
      if (param == "update") {
        if (value > 0) {
          updateInterval = value;
        }
        else {
          throw ParamInvalid(param, "update interval must be > 0");
        }
      }
      else {
        throw ParamUnknown(param);
      }
    }

The exceptions will be caught by the RAT command interpreter and appropriate error messages will be shown to the user before aborting the application.

---------------------------

Register the Class with ProcBlockManager
````````````````````````````````````````
Finally, once you have your processor implemented, you need to edit ``src/cmd/ProcBlockManager.cc`` to register your processor so that users can add it to their macros.  Include the header for your processor at the top, then find the relevant code in the constructor that looks like::

    // Create processor allocator table
    AppendProcessor<OutROOTProc>();
    AppendProcessor<OutNtupleProc>();
    AppendProcessor<NoiseProc>();
    AppendProcessor<CountProc>();

and append your new processor.

