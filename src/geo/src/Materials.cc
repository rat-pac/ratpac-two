#include <CLHEP/Units/PhysicalConstants.h>
#include <CLHEP/Units/SystemOfUnits.h>

#include <G4NistManager.hh>
#include <G4OpticalSurface.hh>
#include <G4SurfaceProperty.hh>
#include <RAT/Log.hh>
#include <RAT/Materials.hh>
#include <algorithm>
#include <functional>
#include <numeric>
#include <vector>

namespace RAT {

std::map<std::string, G4OpticalSurface *> Materials::optical_surface;

void Materials::LoadOpticalSurfaces() {
  // String interface to enums in G4OpticalSurface and G4SurfaceProperty
  std::map<std::string, G4OpticalSurfaceFinish> opticalSurfaceFinishes;
  opticalSurfaceFinishes["polished"] = polished;
  opticalSurfaceFinishes["polishedfrontpainted"] = polishedfrontpainted;
  opticalSurfaceFinishes["polishedbackpainted"] = polishedbackpainted;
  opticalSurfaceFinishes["ground"] = ground;
  opticalSurfaceFinishes["groundfrontpainted"] = groundfrontpainted;
  opticalSurfaceFinishes["groundbackpainted"] = groundbackpainted;

  std::map<std::string, G4OpticalSurfaceModel> opticalSurfaceModels;
  opticalSurfaceModels["glisur"] = glisur;
  opticalSurfaceModels["unified"] = unified;
  opticalSurfaceModels["dichroic"] = dichroic;

  std::map<std::string, G4SurfaceType> opticalSurfaceTypes;
  opticalSurfaceTypes["dielectric_metal"] = dielectric_metal;
  opticalSurfaceTypes["dielectric_dielectric"] = dielectric_dielectric;
  opticalSurfaceTypes["dielectric_dichroic"] = dielectric_dichroic;
  opticalSurfaceTypes["firsov"] = firsov;
  opticalSurfaceTypes["x_ray"] = x_ray;

  // Loop over OPTICS to find photocathodes and general surfaces
  DBLinkGroup lOptics = DB::Get()->GetLinkGroup("OPTICS");
  for (DBLinkGroup::iterator iv = lOptics.begin(); iv != lOptics.end(); iv++) {
    G4Material *mat = G4Material::GetMaterial(iv->first.c_str());
    Log::Assert(mat, dformat("Materials::LoadOpticalSurfaces: Missing material for entry %s", iv->first.c_str()));

    // Load the photocathodes
    bool isPhotocathode = false;
    try {
      isPhotocathode = (bool)iv->second->GetI("photocathode");
    } catch (DBNotFoundError &e) {
    }

    if (isPhotocathode) {
      G4OpticalSurface *pc = new G4OpticalSurface(iv->first.c_str());
      pc->SetType(dielectric_metal);  // Ignored if RINDEX is defined
      pc->SetMaterialPropertiesTable(mat->GetMaterialPropertiesTable());
      optical_surface[pc->GetName()] = pc;
    }

    // Load other optical surfaces
    bool isOpticalSurface = false;
    try {
      isOpticalSurface = (bool)iv->second->GetI("surface");
    } catch (DBNotFoundError &e) {
    }

    Log::Assert(!(isOpticalSurface && isPhotocathode),
                dformat("Materials::LoadOpticalSurfaces: Entry %s is defined as both "
                        "\"photocathode\" and \"surface\", must be one or the other.",
                        iv->first.c_str()));

    if (isOpticalSurface) {
      // Surface properties
      G4OpticalSurfaceModel model = glisur;
      G4OpticalSurfaceFinish finish = polished;
      G4SurfaceType type = dielectric_dielectric;
      double polish = 0.1;

      try {
        std::string name = iv->second->GetS("model");
        if (opticalSurfaceModels.find(name) != opticalSurfaceModels.end()) {
          model = opticalSurfaceModels[name];
        } else {
          Log::Die(dformat("Materials::LoadOpticalSurfaces: Invalid surface model %s", name.c_str()));
        }
      } catch (DBNotFoundError &e) {
      }

      try {
        std::string name = iv->second->GetS("finish");
        if (opticalSurfaceFinishes.find(name) != opticalSurfaceFinishes.end()) {
          finish = opticalSurfaceFinishes[name];
        } else {
          Log::Die(dformat("Materials::LoadOpticalSurfaces: Invalid surface finish %s", name.c_str()));
        }
      } catch (DBNotFoundError &e) {
      }

      try {
        std::string name = iv->second->GetS("type");
        if (opticalSurfaceTypes.find(name) != opticalSurfaceTypes.end()) {
          type = opticalSurfaceTypes[name];
        } else {
          Log::Die(dformat("Materials::LoadOpticalSurfaces: Invalid surface type %s", name.c_str()));
        }
      } catch (DBNotFoundError &e) {
      }

      try {
        polish = iv->second->GetD("polish");
      } catch (DBNotFoundError &e) {
      }

      if (type == dielectric_dichroic) {
        // Dichroic filters need to have a environment variable
        // set to the transmission property file, rather than
        // declaring a properties table directly.
        std::string name = std::string(iv->first.c_str());
        std::string data_file = iv->second->GetS("dichroic_property_file");
        std::string environment = "RATSHARE";
        try {
          environment = iv->second->GetS("data_env");
        } catch (DBNotFoundError &e) {
        }
        std::string data_base_dir = getenv(environment.c_str()) + std::string("/ratdb/");
        std::string data_path = data_base_dir + data_file;
        info << "Getting dichroic data for Material: " << name << " from file: " << data_path << newline;
        setenv("G4DICHROICDATA", data_path.c_str(), 1);
      }

      G4OpticalSurface *surf = new G4OpticalSurface(iv->first.c_str());
      surf->SetFinish(finish);
      surf->SetModel(model);

      // when dichroic filter data is read, geant4 emits unguarded printouts, spamming the log. Calls are here:
      // [https://gitlab.cern.ch/geant4/geant4/-/blob/84a556a9dca683c2d541394a795642ecb4c07a3d/source/materials/src/G4OpticalSurface.cc#L523-562].
      // This hack temporarily redirects cout to avoid this. Should be reverted if upstream is fixed.
      if (type == dielectric_dichroic) {
        std::ofstream devNull("/dev/null");
        // save cout
        std::streambuf *oldCout = G4cout.rdbuf();
        G4cout.rdbuf(devNull.rdbuf());
        surf->SetType(type);
        G4cout.rdbuf(oldCout);
      } else {
        surf->SetType(type);
      }

      surf->SetPolish(polish);

      surf->SetMaterialPropertiesTable(mat->GetMaterialPropertiesTable());

      optical_surface[surf->GetName()] = surf;
    }
  }
}

// ----------------------------------------------------------------
void Materials::ConstructMaterials() {
  // = Elements =====================

  G4String name;
  G4String symbol;

  DB *db = DB::Get();

  DBLinkGroup lelem = db->GetLinkGroup("ELEMENT");
  DBLinkGroup::iterator i_table;
  for (i_table = lelem.begin(); i_table != lelem.end(); i_table++) {
    std::string namedb = i_table->first;
    DBLinkPtr table = i_table->second;
    double adb;
    int zdb = 0;
    std::string csymbol;

    // Get common fields
    try {
      csymbol = table->GetS("SYMBOL");
      zdb = table->GetI("z");
    } catch (DBNotFoundError &e) {
      warn << "Materials error: Could not construct elements" << newline;
    }

    // Check if this element has special isotope abundances
    try {
      const std::vector<int> &isotopes = table->GetIArray("isotopes");
      const std::vector<double> &isotopes_frac = table->GetDArray("isotopes_frac");

      G4Element *elem = new G4Element(namedb, csymbol, isotopes.size());
      for (unsigned i_isotope = 0; i_isotope < isotopes.size(); i_isotope++) {
        // Leave out last field of constructor so mass/mole of isotope
        // is pulled from NIST database.
        G4Isotope *iso = new G4Isotope(namedb, zdb, isotopes[i_isotope]);
        elem->AddIsotope(iso, isotopes_frac[i_isotope]);
      }
      // All done, skip next try block
      continue;
    } catch (DBNotFoundError &e) {
      // No isotope list, assume it is a normal element, next try
      // block will handle it
    }

    try {
      adb = table->GetD("a");
      new G4Element(namedb, csymbol, zdb, adb * CLHEP::g / CLHEP::mole);
    } catch (DBNotFoundError &e) {
      info << "Materials error: Could not construct elements" << newline;
    }
  }

  // === Material ===============================================

  std::vector<std::string> queue;

  DBLinkGroup mats = db->GetLinkGroup("MATERIAL");
  for (DBLinkGroup::iterator iv = mats.begin(); iv != mats.end(); iv++) {
    std::string namedb = iv->first;
    info << "Loaded material: [" << namedb << "]" << newline;

    DBLinkPtr table = iv->second;
    if (!BuildMaterial(namedb, table)) {
      queue.push_back(namedb);
      info << "[" << namedb << "] thrown on construction queue" << newline;
    }
  }

  // Erasing an iterator to the std::vector implies re-shifting all the subsequent
  // entries which can lead the code to crash.
  // Therefore, the loop goes backwards, which ensures that
  // consistency is maintained
  for (std::vector<std::string>::iterator i = queue.begin(); i != queue.end(); ++i) {
    std::string namedb = *i;
    DBLinkPtr table = DB::Get()->GetLink("MATERIAL", namedb);
    info << "queue (pass1): [" << namedb << "]" << newline;
    if (BuildMaterial(namedb, table)) {
      queue.erase(i);
      // Step backwards to make sure that the iterator will point to the right
      // place
      --i;
      info << "[" << namedb << "] found and removed from construction queue" << newline;
    }
    // BuildMaterial(namedb,table);
  }

  /// if after the second pass it is not yet found, default to the NIST databse
  if (!queue.empty()) {
    info << "Not all materials were found in the database. Trying to load them "
            "from NIST."
         << newline;
    G4NistManager *nist_db = G4NistManager::Instance();
    nist_db->SetVerbose(1);
    for (std::vector<std::string>::iterator i = queue.begin(); i != queue.end(); i++) {
      std::string namedb = *i;
      G4Material *addmatptr = nist_db->FindOrBuildMaterial(namedb);
      if (addmatptr) {
        info << "[" << namedb << "] found in NISTDB and removed from construction queue" << newline;
        queue.erase(i);
        --i;
      }
    }
  }

  // Now that loaded from the database do a final loop on the queue
  for (std::vector<std::string>::iterator i = queue.begin(); i != queue.end(); i++) {
    std::string namedb = *i;
    DBLinkPtr table = DB::Get()->GetLink("MATERIAL", namedb);
    info << "queue (pass2): [" << namedb << "]" << newline;
    if (BuildMaterial(namedb, table)) {
      queue.erase(i);
      --i;
      info << "[" << namedb << "] found and removed from construction queue" << newline;
    }
  }

  for (DBLinkGroup::iterator iv = mats.begin(); iv != mats.end(); iv++) {
    bool isOpticalSurface = false;
    try {
      isOpticalSurface = (bool)iv->second->GetI("surface");
    } catch (DBNotFoundError &e) {
    }

    if (isOpticalSurface) {
      G4Material::GetMaterial(iv->first);
    }
  }

  // == Optics ==================================================
  LoadOptics();
}

bool Materials::BuildMaterial(std::string namedb, DBLinkPtr table) {
  G4MaterialPropertiesTable *MPT = nullptr;

  double densitydb;
  int nelementsdb;
  int nmaterialsdb;
  G4State state = kStateUndefined;
  double temperature;
  double pressure;

  try {
    densitydb = table->GetD("density") * CLHEP::g / CLHEP::cm3;
    nelementsdb = table->GetI("nelements");
    nmaterialsdb = table->GetI("nmaterials");
  } catch (DBNotFoundError &e) {
    info << "Materials: Material read error" << newline;
    return false;
  }

  try {
    std::string stringstate = table->GetS("state");
    if (stringstate == "gas")
      state = kStateGas;
    else if (stringstate == "solid")
      state = kStateSolid;
    else if (stringstate == "liquid")
      state = kStateLiquid;
  } catch (DBNotFoundError &e) {
    state = kStateUndefined;
  }

  try {
    temperature = table->GetD("temperature");
  } catch (DBNotFoundError &e) {
    temperature = CLHEP::STP_Temperature;
  }

  try {
    pressure = table->GetD("pressure") * CLHEP::STP_Pressure;
  } catch (DBNotFoundError &e) {
    pressure = CLHEP::STP_Pressure;
  }

  G4Material *tempptr = new G4Material(namedb, densitydb, nelementsdb + nmaterialsdb, state, temperature, pressure);

  std::string formula = "GOOD";
  try {
    formula = table->GetS("formula");
  } catch (DBNotFoundError &e) {
    formula = "BAD";
  }

  if (formula != "BAD") {
    tempptr->SetChemicalFormula(formula);
  }

  double mol = 0;
  formula = "GOOD";
  try {
    mol = table->GetD("mol");
  } catch (DBNotFoundError &e) {
    formula = "BAD";
  }

  if (formula != "BAD") {
    MPT = new G4MaterialPropertiesTable();
    MPT->AddConstProperty("MOL", mol / CLHEP::g, true);
    tempptr->SetMaterialPropertiesTable(MPT);
  }

  if (nelementsdb > 0) {
    std::vector<std::string> elemname = table->GetSArray("elements");
    std::vector<double> elemprop = table->GetDArray("elemprop");

    if (elemname.size() != elemprop.size()) {
      info << "Death...oh Materials material reader, "
           << " how you have forsaken me" << newline;  // fixme tk
      exit(-1);
    }

    for (std::vector<std::string>::size_type i = 0; i < elemname.size(); i++) {
      tempptr->AddElement(G4Element::GetElement(elemname[i]), elemprop[i]);
    }
  }

  if (nmaterialsdb > 0) {
    std::vector<std::string> elemname = table->GetSArray("materials");
    std::vector<double> elemprop = table->GetDArray("matprop");

    if (elemname.size() != elemprop.size()) {
      info << "Death...oh Materials material reader, "
           << "how you have forsaken me" << newline;  // fixme tk
      exit(-1);
    }

    for (std::vector<std::string>::size_type i = 0; i < elemname.size(); i++) {
      std::string addmatname = elemname[i];
      G4Material *addmatptr = nullptr;
      G4NistManager *man = G4NistManager::Instance();
      addmatptr = man->FindOrBuildMaterial(addmatname);
      if (!addmatptr) {
        addmatptr = G4Material::GetMaterial(elemname[i]);
      }

      // If we encounter a material that hasn't been build,
      // add the material that contains it to the queue
      if (addmatptr != nullptr) {
        tempptr->AddMaterial(addmatptr, elemprop[i]);
      } else {
        G4MaterialTable *tmp_table = (G4MaterialTable *)tempptr->GetMaterialTable();
        std::vector<G4Material *>::iterator it = tmp_table->begin() + tempptr->GetIndex();
        delete tempptr;
        tmp_table->erase(it);
        return false;
      }
    }
  }

  return true;
}

G4MaterialPropertyVector *Materials::LoadProperty(DBLinkPtr table, std::string name) {
  int wavelength_opt = 0;
  try {
    std::string optstring = table->GetS(name + "_option");
    if (optstring == "wavelength")
      wavelength_opt = 1;
    else if (optstring == "dy_dwavelength")
      wavelength_opt = 2;
    else if (optstring == "energy")
      wavelength_opt = 0;
    else
      wavelength_opt = 0;
  } catch (DBNotFoundError &e) {
    wavelength_opt = 0;
  }

  std::vector<double> val1, val2;
  try {
    val1 = table->GetDArray(name + "_value1");
    val2 = table->GetDArray(name + "_value2");
  } catch (DBNotFoundError &e) {
    info << "Could not read property " << name << " of " << table->GetIndex() << newline;
    return nullptr;
  }

  if (val1.size() != val2.size()) {
    info << "Array size error in Materials: "
         << "bad property value sizes for " << name << newline;
    return nullptr;
  }

  // Material properties have to be inserted in energy order, so
  // if this is a wavelength, we have to go backwards
  int direction;
  int start;
  if (wavelength_opt) {
    start = val1.size() - 1;
    direction = -1;
  } else {
    start = 0;
    direction = 1;
  }

  G4MaterialPropertyVector *pv = new G4MaterialPropertyVector();
  for (int i = start; i >= 0 && i < (int)val1.size(); i += direction) {
    double E_value = val1[i];
    double p_value = val2[i];
    if (wavelength_opt) {
      if (E_value != 0.0) {
        double lam = E_value;
        E_value = CLHEP::twopi * CLHEP::hbarc / (lam * CLHEP::nanometer);
        if (wavelength_opt == 2) p_value *= lam / E_value;
      } else {
        warn << "Materials property std::vector zero wavelength!" << newline;
      }
    }
    pv->InsertValues(E_value, p_value);
  }

  return pv;
}

void Materials::BuildMaterialPropertiesTable(G4Material *material, DBLinkPtr table) {
  // Bail if the MPT already exists
  if (material->GetMaterialPropertiesTable() != nullptr) {
    return;
  }

  std::string name = std::string(material->GetName());
  // Determine which fields to load
  std::vector<std::string> props;
  try {
    props = table->GetSArray("PROPERTY_LIST");
  } catch (DBNotFoundError &e) {
    info << "Materials failed to get property list on: " << name << newline;
    return;
  }

  G4MaterialPropertiesTable *mpt = new G4MaterialPropertiesTable();
  material->SetMaterialPropertiesTable(mpt);

  // Loop over DB fields containing material (optical) properties
  for (std::vector<std::string>::iterator i = props.begin(); i != props.end(); i++) {
    // Handle const properties
    if (*i == "LIGHT_YIELD" || *i == "dEdxCOEFF" || *i == "WLSTIMECONSTANT" || *i == "WLSMEANNUMBERPHOTONS" ||
        *i == "FASTTIMECONSTANT" || *i == "SLOWTIMECONSTANT" || *i == "YIELDRATIO" || *i == "RESOLUTIONSCALE" ||
        *i == "WLSFASTTIMECONSTANT" || *i == "WLSSLOWTIMECONSTANT" || *i == "WLSYIELDRATIO" ||
        *i == "WLSRESOLUTIONSCALE" || *i == "WLSSCINTILLATIONYIELD" || *i == "SCINTILLATIONYIELD" ||
        *i == "LAMBERTIAN_REFLECTION" || *i == "LAMBERTIAN_FORWARD" || *i == "LAMBERTIAN_BACKWARD" ||
        *i == "ELECTRICFIELD" || *i == "TOTALNUM_INT_SITES" || *i == "MIEHG_FORWARD" || *i == "MIEHG_BACKWARD" ||
        *i == "MIEHG_FORWARD_RATIO" || *i == "TOTALNUM_INT_SITES" || *i == "NUM_COMP" || *i == "SCINT_RISE_TIME" ||
        *i == "SCINT_RISE_TIMEalpha") {
      mpt->AddConstProperty(i->c_str(), table->GetD(*i), true);
      continue;
    }

    // Ignore properties that don't go into the property std::vector
    if (*i == "RSLENGTH_SCALING" || *i == "ABSLENGTH_SCALING") {
      continue;
    }

    // Skip COMPONENTS specification
    if (*i == "COMPONENTS" || *i == "COMPONENT_FRACTIONS") {
      continue;
    }

    // Otherwise, this is a property std::vector
    G4MaterialPropertyVector *pv = mpt->GetProperty((*i).c_str());
    if (!pv) {
      pv = LoadProperty(table, *i);
    }

    if (pv) {
      mpt->AddProperty((*i).c_str(), pv, true);
    } else {
      continue;
    }
  }
  RescaleProperty(table, mpt, "ABSLENGTH");
  RescaleProperty(table, mpt, "RSLENGTH");
}

void Materials::RescaleProperty(DBLinkPtr dbTable, G4MaterialPropertiesTable *propertiesTable,
                                const std::string &property) {
  std::vector<double> scalings;
  try {
    scalings = dbTable->GetDArray(property + "_SCALING");
  } catch (DBNotFoundError &e) {
    detail << "No scaling for " + property + " for " + dbTable->GetIndex() << newline;
    return;
  }
  std::vector<double> reciprocalSum;
  std::vector<double> energies;
  for (size_t iScale = 0; iScale < scalings.size(); iScale++) {
    std::stringstream propertyName;
    propertyName << property << iScale;
    G4MaterialPropertyVector *current = propertiesTable->GetProperty(propertyName.str().c_str());
    if (current == NULL)  // No guarantee this component has this property
      continue;
    if (reciprocalSum.empty()) {
      reciprocalSum.resize(current->GetVectorLength(), 0.0);
      energies.resize(current->GetVectorLength(), -1.0);
    } else if (reciprocalSum.size() != current->GetVectorLength())
      Log::Die("Optics::RescaleProperty: " + property + " components are different length arrays.");

    current->ScaleVector(1.0, 1.0 / scalings[iScale]);  // Scale the values (not energy)
    for (size_t bin = 0; bin < current->GetVectorLength(); bin++) {
      reciprocalSum[bin] += 1.0 / (*current)[bin];
      energies[bin] = current->Energy(bin);
    }
  }
  // This is empty if no component data exists, in which case don't add the
  // property to the properties table
  if (reciprocalSum.empty()) {
    return;
  }
  G4MaterialPropertyVector *total = new G4MaterialPropertyVector();
  for (size_t bin = 0; bin < energies.size(); bin++) {
    total->InsertValues(energies[bin], 1.0 / reciprocalSum[bin]);
  }
  propertiesTable->AddProperty(property.c_str(), total);
}

void Materials::LoadOptics() {
  DBLinkGroup mats = DB::Get()->GetLinkGroup("OPTICS");

  // Load everything in OPTICS
  for (DBLinkGroup::iterator iv = mats.begin(); iv != mats.end(); iv++) {
    std::string name = iv->first;
    info << "Loading optics: " << name << newline;

    G4Material *material = G4Material::GetMaterial(name);
    if (material == nullptr) {
      info << "While loading optics in Materials, "
           << "there was a bad material name: " << name << newline;
      continue;
    }

    // Build MPT if necessary
    BuildMaterialPropertiesTable(material, iv->second);
  }

  // Handle optics for composite materials
  for (DBLinkGroup::iterator iv = mats.begin(); iv != mats.end(); iv++) {
    std::string name = iv->first;
    DBLinkPtr table = iv->second;

    std::vector<std::string> props;
    try {
      props = table->GetSArray("PROPERTY_LIST");
    } catch (DBNotFoundError &e) {
      info << "Materials failed to get property list on: " << name << newline;
      Log::Die("Materials unable to load property list");
      return;
    }

    if (std::find(props.begin(), props.end(), "COMPONENTS") == props.end()) {
      continue;
    }

    std::vector<std::string> components;
    std::vector<double> fractions;

    // Load and validate the table entries
    try {
      components = table->GetSArray("COMPONENTS");
      fractions = table->GetDArray("COMPONENT_FRACTIONS");
    } catch (DBNotFoundError &e) {
      Log::Die("Materials: Error loading COMPONENTS property.");
    }

    Log::Assert(components.size() == fractions.size(), "Materials: COMPONENTS and COMPONENT_FRACTIONS length mismatch");
    double total_fractions = std::accumulate(fractions.begin(), fractions.end(), 0.0);
    Log::Assert(total_fractions < 1.01 && total_fractions > 0.99, "Materials: Component fractions must sum to 1");

    G4Material *material = G4Material::GetMaterial(name);
    Log::Assert(material, "Materials: Unable to locate primary material");
    G4MaterialPropertiesTable *mpt = material->GetMaterialPropertiesTable();

    // Loop over components, adding their properties to the current material's
    // properties table. They are numbered by their index so that we can
    // e.g. absorb on X and it with the right spectrum, except for
    // scattering, which is combined into a single scattering length
    mpt->AddConstProperty("NCOMPONENTS", components.size(), true);

    const std::vector<G4String> &materialPropertyNames = mpt->GetMaterialPropertyNames();
    const std::vector<G4String> &materialConstPropertyNames = mpt->GetMaterialConstPropertyNames();

    for (size_t i = 0; i < components.size(); i++) {
      std::string compname = components[i];
      double fraction = fractions[i];
      std::stringstream ss;
      ss << "FRACTION" << i + 1;
      mpt->AddConstProperty(ss.str().c_str(), fraction, true);

      info << " - Component " << i << ": " << compname << " (" << 100.0 * fraction << "%)" << newline;

      G4Material *component = G4Material::GetMaterial(compname);
      Log::Assert(material, "Materials: Unable to locate component material");
      //// const std::map<G4String,
      ////                G4MaterialPropertyVector*>* mpm =
      /// mpt->GetPropertiesMap(); / const std::map<G4String, / G4double>* mpmc
      /// = mpt->GetPropertiesCMap();

      G4MaterialPropertiesTable *cpt = component->GetMaterialPropertiesTable();

      // Copy component std::vector properties to material
      G4double *absorption_coeff_x = nullptr;
      G4double *absorption_coeff_y = nullptr;
      G4double *rayleigh_coeff_x = nullptr;
      G4double *rayleigh_coeff_y = nullptr;

      //// const std::map<G4String,
      ////                G4MaterialPropertyVector*>* cpm =
      /// cpt->GetPropertiesMap(); / std::map<G4String, /
      /// G4MaterialPropertyVector*>::const_iterator it;
      auto cptPropertyNames = cpt->GetMaterialPropertyNames();
      auto cptProperties = cpt->GetProperties();
      for (size_t propid = 0; propid < cptPropertyNames.size(); propid++) {
        //// for (it=cpm->begin(); it!=cpm->end(); it++) {
        //// std::string pname = it->first;
        G4String pname = cptPropertyNames.at(propid);
        G4MaterialPropertyVector *propertyVector = cptProperties.at(propid);
        if (pname.find("REEMITWAVEFORM") != std::string::npos || pname.find("SCINTILLATION_WLS") != std::string::npos ||
            pname.find("ABSLENGTH") != std::string::npos) {
          Log::Assert(std::find(materialPropertyNames.begin(), materialPropertyNames.end(), pname) ==
                          materialPropertyNames.end(),
                      "Materials: Composite material cannot contain the same "
                      "properties as components");
          //// Log::Assert(mpm->find(pname) == mpm->end(),
          ////             "Materials: Composite material cannot contain the
          /// same properties as components");
          info << compname << " has " << pname << "!" << newline;
          std::stringstream ss2;
          ss << pname << i + 1;
          //// mpt->AddProperty(ss2.str().c_str(), it->second);
          mpt->AddProperty(ss2.str().c_str(), propertyVector, true);

          // Also compute total absorption length
          if (pname.find("ABSLENGTH") != std::string::npos) {
            if (!absorption_coeff_x) {
              //// G4MaterialPropertyVector* tempv = it->second;
              size_t entries = propertyVector->GetVectorLength();
              absorption_coeff_x = new G4double[entries];
              absorption_coeff_y = new G4double[entries];
              for (size_t ientry = 0; ientry < entries; ientry++) {
                absorption_coeff_x[ientry] = propertyVector->Energy(ientry);
                absorption_coeff_y[ientry] = fraction / propertyVector->Value(ientry);
              }
            }
          }

          size_t entries = propertyVector->GetVectorLength();
          for (size_t ientry = 0; ientry < entries; ientry++) {
            G4double ahere = propertyVector->Value(absorption_coeff_x[ientry]);
            absorption_coeff_y[ientry] += fraction / ahere;
          }
        }

        // The scattering lengths are combined
        else if (pname.find("RSLENGTH") != std::string::npos) {
          //// Log::Assert(mpm->find(pname) == mpm->end(),
          ////             "Materials: Composite material cannot contain the
          /// same properties as components");
          Log::Assert(std::find(materialPropertyNames.begin(), materialPropertyNames.end(), pname) ==
                          materialPropertyNames.end(),
                      "Materials: Composite material cannot contain the same "
                      "properties as components");
          if (!rayleigh_coeff_x) {
            //// G4MaterialPropertyVector* tempv = it->second;
            G4int entries = propertyVector->GetVectorLength();
            rayleigh_coeff_x = new G4double[entries];
            rayleigh_coeff_y = new G4double[entries];
            for (G4int ientry = 0; ientry < entries; ientry++) {
              rayleigh_coeff_x[ientry] = propertyVector->Energy(ientry);
              rayleigh_coeff_y[ientry] = fraction / propertyVector->Value(ientry);
            }
          }

          size_t entries = propertyVector->GetVectorLength();
          for (size_t ientry = 0; ientry < entries; ientry++) {
            G4double ahere = propertyVector->Value(rayleigh_coeff_x[ientry]);
            rayleigh_coeff_y[ientry] += fraction / ahere;
          }
        }
      }

      // Copy component constant properties to material
      auto cptConstPropertyNames = cpt->GetMaterialConstPropertyNames();
      auto cptConstProperties = cpt->GetConstProperties();

      //// const std::map<G4String,
      ////                G4double>* cpmc = cpt->GetPropertiesCMap();
      //// std::map<G4String,
      ////          G4double>::const_iterator itc;
      //// for (itc=cpmc->begin(); itc!=cpmc->end(); itc++) {
      for (size_t propid = 0; propid < cptConstPropertyNames.size(); propid++) {
        G4String cname = cptConstPropertyNames.at(propid);
        //// std::string cname = itc->first;
        if (cname.find("REEMISSION_PROB") != std::string::npos) {
          //// Log::Assert(mpmc->find(name) == mpmc->end(),
          ////             "Materials: Composite material cannot contain the
          /// same properties as components");
          Log::Assert(std::find(materialConstPropertyNames.begin(), materialConstPropertyNames.end(), name) ==
                          materialConstPropertyNames.end(),
                      "Materials: Composite material cannot contain the same "
                      "properties as components");
          info << compname << " has " << cname << newline;
          std::stringstream ss2;
          ss2 << cname << i + 1;
          mpt->AddConstProperty(ss2.str().c_str(), cptConstProperties.at(propid).first);
        }
      }
    }

    info << "----" << newline;
    //// const std::map<G4String,
    ////                G4MaterialPropertyVector*>* mpm =
    /// mpt->GetPropertiesMap(); / std::map<G4String, /
    /// G4MaterialPropertyVector*>::const_iterator it; / for (it=mpm->begin();
    /// it!=mpm->end(); it++) { /   info << it->first << newline; / } / const
    /// std::map<G4String, /                G4double>* mcpm =
    /// mpt->GetPropertiesCMap(); / std::map<G4String, /
    /// G4double>::const_iterator cit; / for (cit=mcpm->begin();
    /// cit!=mcpm->end(); cit++) { /   info << cit->first << newline; / }
    info << "----" << newline;
  }
}

}  // namespace RAT
