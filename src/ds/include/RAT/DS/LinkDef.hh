#ifdef __CINT__

#pragma link C++ class RAT::DS::Root + ;
#pragma link C++ class RAT::DS::PMTInfo + ;
#pragma link C++ class RAT::DS::NestedTubeInfo + ;
#pragma link C++ class RAT::DS::ChannelStatus + ;

#pragma link C++ class RAT::DS::MC + ;
#pragma link C++ class RAT::DS::MCParticle + ;
#pragma link C++ class RAT::DS::MCTrack + ;
#pragma link C++ class RAT::DS::MCTrackStep + ;
#pragma link C++ class RAT::DS::MCPMT + ;
#pragma link C++ class RAT::DS::MCNestedTube + ;
#pragma link C++ class RAT::DS::MCPhoton + ;
#pragma link C++ class RAT::DS::MCNestedTubeHit + ;
#pragma link C++ class RAT::DS::MCSummary + ;
#pragma link C++ class RAT::DS::Calib + ;
#pragma link C++ class RAT::DS::FitResult + ;
#pragma link C++ class RAT::DS::Classifier + ;

#pragma link C++ class RAT::DS::EV + ;
#pragma link C++ class RAT::DS::PMT + ;
#pragma link C++ class RAT::DS::DigitPMT + ;
#pragma link C++ class RAT::DS::WaveformAnalysisResult + ;
#pragma link C++ class RAT::DS::LAPPD + ;
#pragma link C++ class RAT::DS::LAPPDHit + ;
#pragma link C++ class RAT::DS::Digit + ;

#pragma link C++ class RAT::DS::RunStore + ;
#pragma link C++ class RAT::DS::Run + ;

#pragma link C++ class RAT::DSReader;
#pragma link C++ class RAT::DSWriter;

#pragma link C++ class RAT::TrackNav;
#pragma link C++ class RAT::TrackCursor;
#pragma link C++ class RAT::TrackNode;

#pragma link C++ class RAT::DB + ;
#pragma link C++ class RAT::DBLink + ;
#pragma link C++ class RAT::DBLinkPtr + ;
#pragma link C++ class RAT::DBTable + ;
#pragma link C++ class RAT::DBTextLoader + ;
#pragma link C++ class RAT::DBJsonLoader + ;
#pragma link C++ class json::Value + ;

#pragma link C++ class RAT::ObjInt + ;
#pragma link C++ class RAT::ObjDbl + ;
#pragma link C++ class RAT::Log + ;
#pragma link C++ class RAT::HTTPDownloader + ;
#pragma link C++ class RAT::DBFieldCallback + ;

#endif  // __CINT__

#ifdef __MAKECINT__

#pragma link C++ class pair < string, string> + ;
#pragma link C++ class map < int, int> + ;
#pragma link C++ class map < int, vector < int>> + ;
#pragma link C++ class vector < TVector3>;
#pragma link C++ class vector < vector < int>> + ;
#pragma link C++ class vector < vector < double>> + ;

#pragma link C++ class vector < RAT::DS::Root>;
#pragma link C++ class vector < RAT::DS::PMTInfo>;
#pragma link C++ class vector < RAT::DS::ChannelStatus>;

#pragma link C++ class vector < RAT::DS::MC>;
#pragma link C++ class vector < RAT::DS::MCParticle>;
#pragma link C++ class vector < RAT::DS::MCTrack>;
#pragma link C++ class vector < RAT::DS::MCTrackStep>;
#pragma link C++ class vector < RAT::DS::MCPMT>;
#pragma link C++ class vector < RAT::DS::MCPhoton>;
#pragma link C++ class vector < RAT::DS::MCNestedTube>;
#pragma link C++ class vector < RAT::DS::MCNestedTubeHit>;
#pragma link C++ class vector < RAT::DS::Calib>;
#pragma link C++ class vector < RAT::DS::EV>;
#pragma link C++ class vector < RAT::DS::PMT>;
#pragma link C++ class vector < RAT::DS::DigitPMT>;
#pragma link C++ class vector < RAT::DS::LAPPD>;
#pragma link C++ class vector < RAT::DS::LAPPDHit>;
#pragma link C++ class vector < RAT::DS::Digit>;
#pragma link C++ class vector < pair < string, int>>;
#pragma link C++ class vector < RAT::DBTable *>;

#pragma link C++ class vector < RAT::TrackNode *>;
#pragma link C++ class vector < RAT::DS::EV *>;
#pragma link C++ class vector < RAT::DS::PMT *>;
#pragma link C++ class map < Int_t, RAT::DS::PMT>;
#pragma link C++ class vector < RAT::DS::DigitPMT *>;
#pragma link C++ class map < Int_t, RAT::DS::DigitPMT>;
#pragma link C++ class vector < RAT::DS::WaveformAnalysisResult>;
#pragma link C++ class map < string, RAT::DS::WaveformAnalysisResult> + ;
#pragma link C++ class vector < RAT::DS::LAPPD *>;
#pragma link C++ class vector < RAT::DS::LAPPDHit *>;
#pragma link C++ class vector < RAT::DS::Digit *>;
#pragma link C++ class vector < RAT::DS::MCParticle *>;
#pragma link C++ class vector < RAT::DS::MCTrack *>;
#pragma link C++ class vector < RAT::DS::MCPMT *>;
#pragma link C++ class vector < RAT::DS::MCPhoton *>;
#pragma link C++ class vector < RAT::DS::MCNestedTube *>;
#pragma link C++ class vector < RAT::DS::MCNestedTubeHit *>;
#pragma link C++ class vector < RAT::DS::MCTrackStep *>;

#endif
