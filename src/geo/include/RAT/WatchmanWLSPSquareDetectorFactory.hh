#ifndef __RAT_WatchmanWLSPSquareDetectorFactory__
#define __RAT_WatchmanWLSPSquareDetectorFactory__

#include <RAT/DetectorFactory.hh>

namespace RAT {

class WatchmanWLSPSquareDetectorFactory : public DetectorFactory {

    public:
        WatchmanWLSPSquareDetectorFactory() { }
        virtual ~WatchmanWLSPSquareDetectorFactory() { }
        
    protected:
        virtual void DefineDetector(DBLinkPtr detector);

};

} //namespace RAT

#endif
