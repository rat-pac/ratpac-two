// LinkDef fragment for db classes reflected into the RAT ROOT dictionary.
// Pulled in by the generated global LinkDef (see cmake/RATDictionary.cmake).
#ifdef __CINT__

#pragma link C++ class RAT::DB + ;
#pragma link C++ class RAT::DBLink + ;
#pragma link C++ class RAT::DBLinkPtr + ;
#pragma link C++ class RAT::DBTable + ;
#pragma link C++ class RAT::DBTextLoader + ;
#pragma link C++ class RAT::DBJsonLoader + ;
#pragma link C++ class json::Value + ;
#pragma link C++ class RAT::HTTPDownloader + ;
#pragma link C++ class RAT::DBFieldCallback + ;

#endif  // __CINT__

#ifdef __MAKECINT__

#pragma link C++ class vector < RAT::DBTable *>;

#endif
