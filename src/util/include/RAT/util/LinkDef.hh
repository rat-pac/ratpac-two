// LinkDef fragment for util classes reflected into the RAT ROOT dictionary.
// Pulled in by the global src/LinkDef.hh. The '-' suppresses streamer
// generation: TransitTimeCalculator indirectly owns a live G4Navigator and is
// not meant to be persisted.
#ifdef __CINT__

#pragma link C++ class RAT::TransitTimeCalculator - ;

#endif  // __CINT__
