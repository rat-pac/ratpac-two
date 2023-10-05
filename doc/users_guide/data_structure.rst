Data Structure
--------------
The event data structure is a tree of information about a particular event.
Event producers (like [wiki:UserGuideGsim Gsim]) create an instance of the the
data structure for each event, and processors can then operate on this
structure, transforming it as desired. A single event is that which is generated
when a macro calls `/generator/beamOn`; however, a processor is free to break
this event into multiple sub-events.

The Ratpac Data Structure (RatDS)
`````````````````````````````````
The Ratpac data structure is defined in the `src/ds` directory and lists
everything under the `RAT::DS` namespace. An instance of the data structure is
defined in the `RAT::DS::Root` object. The data structure is tree-like, with each
instance of `RAT::DS::Root` containing a list of `RAT::DS::MC` objects which contain
the Monte Carl truth infomration, and a list of `RAT::DS::EV` objects which contain
the reconstructed event information (usually after going through a processor that
simulates the detector response).

RAT::DS::MC
'''''''''''

RAT::DS::EV
'''''''''''
