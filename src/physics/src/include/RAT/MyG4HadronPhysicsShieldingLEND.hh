//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
//
//---------------------------------------------------------------------------
//
// ClassName:
//
// Author: 7 Nov 2017 Tatsumi Koi
//   created from G4HadronPhysicsShielding
//
// 02.10.2020 V.Ivanchenko use inheritence from Shielding and cleanup
//            utilities; use explicit constructors
//----------------------------------------------------------------------------
//
#ifndef MyG4HadronPhysicsShieldingLEND_h
#define MyG4HadronPhysicsShieldingLEND_h 1

#include "RAT/MyG4HadronPhysicsShielding.hh"

class MyG4HadronPhysicsShieldingLEND : public MyG4HadronPhysicsShielding {
 public:
  explicit MyG4HadronPhysicsShieldingLEND(G4int verbose);
  explicit MyG4HadronPhysicsShieldingLEND(const G4String& name);
  explicit MyG4HadronPhysicsShieldingLEND(const G4String& name = "hInelastic My_ShieldingLEND", G4bool qe = false);
  explicit MyG4HadronPhysicsShieldingLEND(const G4String& name, G4int verbose);
  explicit MyG4HadronPhysicsShieldingLEND(const G4String& name, G4int verbose, G4double minFTFPEnergy,
                                          G4double maxBertiniEnergy);

  virtual ~MyG4HadronPhysicsShieldingLEND();

  // copy constructor and hide assignment operator
  MyG4HadronPhysicsShieldingLEND(MyG4HadronPhysicsShieldingLEND&) = delete;
  MyG4HadronPhysicsShieldingLEND& operator=(const MyG4HadronPhysicsShieldingLEND& right) = delete;
};

#endif
