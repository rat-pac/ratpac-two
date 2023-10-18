// This file is part of the GenericLAND software library.
// $Id: GLG4VisManager.cc,v 1.1 2005/08/30 19:55:23 volsung Exp $
//
// GenericLAND visualization manager based on Geant4's "MyVisManager"
//   -- main purpose of defining our own is to reorient "up" vector
//
// See class description of G4VisManager for more details.
//
// Author:  Glenn Horton-Smith, Jan 28, 2000
//
// Changed by Franco Giuliani, July 2009
// Additional Trajectory Model added. Morgan Askins, Dec 2019

#include "RAT/GLG4VisManager.hh"

#include "G4ViewParameters.hh"
#include "RAT/GLG4VisMessenger.hh"

// Supported drivers...

// Not needing external packages or libraries...
#include "G4ASCIITree.hh"
#include "G4DAWNFILE.hh"
#include "G4HepRepFile.hh"
#include "G4HitFilterFactories.hh"
#include "G4OpenGLImmediateQt.hh"
#include "G4OpenGLStoredQt.hh"
#include "G4RayTracer.hh"
#include "G4TrajectoryDrawByParticleID.hh"
#include "G4TrajectoryFilterFactories.hh"
#include "G4TrajectoryModelFactories.hh"
#include "G4VRML2File.hh"
#include "G4Version.hh"

// Needing external packages or libraries...

#ifdef G4VIS_USE_DAWN
#include "G4FukuiRenderer.hh"
#endif

#ifdef G4VIS_USE_OPACS
#include "G4Wo.hh"
#include "G4Xo.hh"
#endif

#ifdef G4VIS_USE_OPENGLX
#include "G4OpenGLImmediateX.hh"
#include "G4OpenGLStoredX.hh"
#endif

#ifdef G4VIS_USE_OPENGLWIN32
#include "G4OpenGLImmediateWin32.hh"
#include "G4OpenGLStoredWin32.hh"
#endif

#ifdef G4VIS_USE_OPENGLXM
#include "G4OpenGLImmediateXm.hh"
#include "G4OpenGLStoredXm.hh"
#endif

#ifdef G4VIS_USE_OIX
#include "G4OpenInventorX.hh"
#endif

#ifdef G4VIS_USE_OIWIN32
#include "G4OpenInventorWin32.hh"
#endif

#ifdef G4VIS_USE_RAYTRACERX
#include "G4RayTracerX.hh"
#endif

#ifdef G4VIS_USE_VRML
#include "G4VRML2.hh"
#endif

GLG4VisManager::GLG4VisManager() { new GLG4VisMessenger(this); }

void GLG4VisManager::RegisterGraphicsSystems() {
  // Graphics Systems not needing external packages or libraries...
  RegisterGraphicsSystem(new G4ASCIITree);
  RegisterGraphicsSystem(new G4DAWNFILE);
  RegisterGraphicsSystem(new G4HepRepFile);
  RegisterGraphicsSystem(new G4RayTracer);
  RegisterGraphicsSystem(new G4VRML2File);
  RegisterGraphicsSystem(new G4OpenGLImmediateQt);
  RegisterGraphicsSystem(new G4OpenGLStoredQt);

  // Graphics systems needing external packages or libraries...

#ifdef G4VIS_USE_DAWN
  RegisterGraphicsSystem(new G4FukuiRenderer);
#endif

#ifdef G4VIS_USE_OPACS
  RegisterGraphicsSystem(new G4Wo);
  RegisterGraphicsSystem(new G4Xo);
#endif

#ifdef G4VIS_USE_OPENGLX
  RegisterGraphicsSystem(new G4OpenGLImmediateX);
  RegisterGraphicsSystem(new G4OpenGLStoredX);
#endif

#ifdef G4VIS_USE_OPENGLWIN32
  RegisterGraphicsSystem(new G4OpenGLImmediateWin32);
  RegisterGraphicsSystem(new G4OpenGLStoredWin32);
#endif

#ifdef G4VIS_USE_OPENGLXM
  RegisterGraphicsSystem(new G4OpenGLImmediateXm);
  RegisterGraphicsSystem(new G4OpenGLStoredXm);
#endif

#ifdef G4VIS_USE_OIX
  RegisterGraphicsSystem(new G4OpenInventorX);
#endif

#ifdef G4VIS_USE_OIWIN32
  RegisterGraphicsSystem(new G4OpenInventorWin32);
#endif

#ifdef G4VIS_USE_RAYTRACERX
  RegisterGraphicsSystem(new G4RayTracerX);
#endif

#ifdef G4VIS_USE_VRML
  RegisterGraphicsSystem(new G4VRML2);
#endif
}

void GLG4VisManager::RegisterModelFactories() {
  // Trajectory draw models
  RegisterModelFactory(new G4TrajectoryGenericDrawerFactory());
  RegisterModelFactory(new G4TrajectoryDrawByChargeFactory());
  RegisterModelFactory(new G4TrajectoryDrawByParticleIDFactory());
  RegisterModelFactory(new G4TrajectoryDrawByOriginVolumeFactory());
  RegisterModelFactory(new G4TrajectoryDrawByAttributeFactory());

  // Trajectory filter models
  RegisterModelFactory(new G4TrajectoryChargeFilterFactory());
  RegisterModelFactory(new G4TrajectoryParticleFilterFactory());
  RegisterModelFactory(new G4TrajectoryOriginVolumeFilterFactory());
  RegisterModelFactory(new G4TrajectoryAttributeFilterFactory());

  // Hit filter models
  RegisterModelFactory(new G4HitAttributeFilterFactory());

  // Custom model
  G4TrajectoryDrawByParticleID *mymodel = new G4TrajectoryDrawByParticleID("watchmodel");

  G4Colour pink(1.0, 0.2, 0.8);
  G4Colour alpha(0.0, 1.0, 0.8);

  mymodel->SetDefault("white");
  mymodel->Set("opticalphoton", G4Colour(0.30, 0.65, 1.0, 0.03));
  mymodel->Set("gamma", "green");
  mymodel->Set("alpha", alpha);
  mymodel->Set("nu_e", pink);
  mymodel->Set("nu_mu", pink);
  mymodel->Set("anti_nu_e", pink);
  mymodel->Set("anti_nu_mu", pink);
  mymodel->Set("e-", "yellow");
  mymodel->Set("mu-", "red");
  mymodel->Set("e+", "red");
  mymodel->Set("mu+", G4Colour(0.78, 0.78,
                               0.78));  // to distinguish mu+ from mu- on black background.
  mymodel->Set("proton", "magenta");
  mymodel->Set("neutron", G4Colour(1.0, 0.6, 0.0, 1.0));
  RegisterModel(mymodel);

  SelectTrajectoryModel(mymodel->Name());
}
