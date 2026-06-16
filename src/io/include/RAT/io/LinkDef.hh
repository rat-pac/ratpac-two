// LinkDef fragment for io classes reflected into the RAT ROOT dictionary.
// Pulled in by the generated global LinkDef (see cmake/RATDictionary.cmake).
#ifdef __CINT__

#pragma link C++ class RAT::DSReader;
#pragma link C++ class RAT::DSWriter;

#pragma link C++ class RAT::TrackNav;
#pragma link C++ class RAT::TrackCursor;
#pragma link C++ class RAT::TrackNode;

#pragma link C++ class RAT::ObjInt + ;
#pragma link C++ class RAT::ObjDbl + ;

#endif  // __CINT__

#ifdef __MAKECINT__

#pragma link C++ class vector < RAT::TrackNode *>;

#endif
