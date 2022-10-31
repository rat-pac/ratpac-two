#ifndef __RAT_Chroma__
#define __RAT_Chroma__

#ifdef ZMQ_Enabled

#include <vector>
#include <string>
#include <zmq.hpp>
#include <G4ThreeVector.hh>

namespace RAT {

class Chroma;

class PhotonData {
friend class Chroma;
public:
    PhotonData();
    virtual ~PhotonData();
    
    virtual void resize(uint32_t _numphotons);
    virtual void* fillmsg(void *dest);
    virtual void* readmsg(void *src);
    virtual size_t size();

protected:
    uint32_t numphotons, event;
    std::vector<double> x,y,z,dx,dy,dz,polx,poly,polz,wavelen,t;
    std::vector<uint32_t> trackid;
};

class HitData : public PhotonData {
friend class Chroma;
public:
    HitData(); 
    virtual ~HitData();
    
    virtual void resize(uint32_t _numphotons);
    virtual void* fillmsg(void *dest);
    virtual void* readmsg(void *src);
    virtual size_t size();
    
protected:
    std::vector<uint32_t> channel;
};

class Chroma {
public:

    Chroma(std::string conn);
    virtual ~Chroma();

    void addPhoton(const G4ThreeVector &pos, const G4ThreeVector &dir, const G4ThreeVector &pol, const double wavelen, const double t, const uint32_t trackid);
    void propagate();

protected:
    zmq::context_t context;
    zmq::socket_t socket;
    
    PhotonData photons;
    HitData hits;
};

}

#endif

#endif
