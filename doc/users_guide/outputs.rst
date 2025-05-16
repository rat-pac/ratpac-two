.. _output_processors:

Output Processors
`````````````````

OutROOT
=======
The OutROOT processor writes events to disk in the ROOT format.  The events are
stored in a TTree object called "T" and the branch holding the events (class
[source:RAT/trunk/include/RAT_DS.hh#latest RAT_DS]) is called "ds".

Command:
::

    /rat/proc outroot

Parameters:
::

    /rat/procset file "filename"


* filename (required, string) Sets output filename.  File will be deleted if it already exists.

OutNtuple
=========

the Outntuple proc details.

----------------------

OutNet
======
Note: This has been untested for like a decade?

The !OutNet processor transmits events over the network to a listening copy of
RAT which is running the [wiki:UserGuideInNet InNet] event producer.  Multiple
listener hostnames may be specified, and events will be distributed across them
with very simplistic load-balancing algorithm.

This allows an event loop to be split over multiple machines.  I'll leave it to
your imagination to think up a use for this...

Command:
::

    /rat/proc outnet


Parameters:
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
