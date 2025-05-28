.. _processors:

Event Processors
----------------

Event processors, described in :ref:`producers_processors`, are part of the event loop. They do not create new events, but instead receive events one-by-one and may either change the event by adding to or altering its contents. As an example, the data acquisition (DAQ) processors will receive an event and then, based on information such as the number of PMTs that detected light, decide whether the event caused the detector to trigger. All ratpac-two processors inherit the methods from the processor class, which provides the structure for how all processors run. For more details about these methods, how to use them, and how to write new processors, find details in the Programmer's guide: :ref:`programming_a_processor`. Below we describe the existing processors in ratpac-two.

----------------------

.. _using_a_processor_from_the_macro:

Using a Processor From the Macro
````````````````````````````````

The ratpac-two processors run as a block in the macro, instantiated after the ``/run/initialize`` line and prior to the generators. Below is an example of where several processors are run in a macro. Here we specify the last processor in the chain using the ``proclast`` syntax.

::

        # Setup database parameters

        /run/initialize

        # DAQ processor
        /rat/proc splitevdaq

        # Count processor
        /rat/proc count
        /rat/procset update 100

        # Quad fitter reconstruction
        /rat/proc quadfitter

        # Choose output file type
        /rat/proclast outntuple

        /generator/add combo gun:point:poisson

        # ... 
        # Generator continued


The parameters for a processor are often highly configurable. This can be achieved by loading these parameters from the database, ratdb, described more in :ref:`ratdb`. In general, if the processors parameter is loaded from ratdb, we can change it from the macro using (using the PMT noise processor as an example): 

::

        /rat/db/set NOISEPROC noise_flag 1

Additionally, for processors, there are methods provided that allow the user to directly change parameters using the ``procset`` syntax (as shown already in the above example for the count processor):

::

        /rat/proc splitevdaq
        /rat/procset trigger_threshold 4.0

Cases where a parameter is tunable using ``procset`` are documented specifically for each processor below. For more details the Programmer's guide: :ref:`programming_a_processor` shows how this is achieved in the code using the 'Set' methods provided by the Processor class.

----------------------

.. _count_processor:

Count Processor
```````````````
The count processor (located in ``src/core``) exists mostly as a simple demonstration processor.  It also displays messages periodically showing both how many physics events and detector events have been processed. The message looks something like::

    CountProc: Event 5 (3 triggered events)

This can be useful for quickly and roughly understanding what fraction of total simulated events (5) are causing detector triggers (3). In certain cases where the simulated events can create multiple triggered events, we may observe that in the count processor::

    CountProc: Event 5 (10 triggered events)

Command:
::

    /rat/proc count

Parameters:
::

    /rat/procset update [interval]

* interval (optional, integer) - Sets number of physics events between between
  status update messages.  Defaults to 1 (print a message for every event).

----------------------

.. _prune_proc:

Prune Processor
```````````````
The Prune processor is a processor for removing unwanted parts of the data structure to save space. The prune processor may be useful to call before the OutROOT processor to avoid writing large amounts of data to disk.

Note that there is minimal benefit to pruning in order to save memory in the running program.  Only one data structure is present in memory at any given time, and it is never copied.  Only when lots of events are written to disk does the overhead become considerable.

Command:
::

    /rat/proc prune


Parameters:
::

    /rat/procset prune "cutlist"

* cutlist - (required) a comma separated (no spaces) list of parts of the data
  structure to remove. The currently allowed entries are:

  * mc
  * mc.particle
  * mc.pmt
  * mc.pmt.photon
  * mc.track
  * ev
  * ev.pmt

If /tracking/storeTrajectory is turned on, mc.track:particle is used, where particle is the name of the particle track you want to prune (mc.track:opticalphoton will prune optical photon tracks).

----------------------

.. _python:

Python Processor
````````````````

The python processor is a relatively unsupported feature of ``ratpac-two``, but works for simple implementations of processors. There are several examples of python processors implementated in ``python/ratproc/`` that can be used to help develop a new processor. The base classes for the processors are provided in ``python/ratproc/base.py``, which can be overloaded in dedicated python processors. The easiest example to follow is the python count processor, which provides the same functionality as the c++ version discussion in :ref:`count_processor`, and is located in ``python/ratproc/count.py``. The python count processor (the class is named ``Count``) can be run from the macro using::

/rat/proc python
/rat/procset class "ratproc.Count(interval=10)"

----------------------

.. _pmt:

PMT Processors
``````````````

The PMT processors are described in :ref:`pmt_simulation`.

----------------------

.. _daq:

DAQ Processors
``````````````

The DAQ processors run the data aquisition model and are described in :ref:`daq_processors`.

----------------------

.. _recon:

Reconstruction Processors
`````````````````````````

The reconstruction processors are described in :ref:`reconstruction_processors`.

----------------------

.. _output:

Output Processors
`````````````````

The output processors are described in :ref:`output_processors`.

