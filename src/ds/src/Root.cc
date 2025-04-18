#include <RAT/DS/Calib.hh>
#include <RAT/DS/Classifier.hh>
#include <RAT/DS/EV.hh>
#include <RAT/DS/FitResult.hh>
#include <RAT/DS/MC.hh>
#include <RAT/DS/MCNestedTube.hh>
#include <RAT/DS/MCNestedTubeHit.hh>
#include <RAT/DS/MCPMT.hh>
#include <RAT/DS/MCParticle.hh>
#include <RAT/DS/MCPhoton.hh>
#include <RAT/DS/PMT.hh>
#include <RAT/DS/Root.hh>
#include <RAT/DSReader.hh>
#include <RAT/TrackNav.hh>

ClassImp(RAT::DS::Root);
ClassImp(RAT::DS::MC);
ClassImp(RAT::DS::MCParticle);
ClassImp(RAT::DS::MCPMT);
ClassImp(RAT::DS::MCNestedTube);
ClassImp(RAT::DS::Calib);
ClassImp(RAT::DS::EV);
ClassImp(RAT::DS::PMT);
ClassImp(RAT::DS::FitResult);
ClassImp(RAT::DS::Classifier);
ClassImp(RAT::DSReader);
ClassImp(RAT::TrackNode);
