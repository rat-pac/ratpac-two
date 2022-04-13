/////////////////////
// DARK BOX
/////////////////////

{
  name: "GEO",
  index: "world",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 1, // omitted for visualization
  mother: "",
  type: "box",
  size: [3000.0,3000.0,600.0], //mm, half-lenght
  material: "air",
}

{
  name: "GEO",
  index: "darkbox",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 1, // omitted for visualization
  mother: "world",
  type: "box",
  size: [762.0,762.0,508.0], //mm, half-lenght
  material: "acrylic_black", //acrylic_black
  surface: "acrylic_black",
  color: [0.5, 0.2, 0.1, 0.1],
}

{
  name: "GEO",
  index: "inner",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 1, // omitted for visualization
  mother: "darkbox",
  type: "box",
  size: [711.2,711.2,457.2], //mm, half-lenght
  material: "air",
  color: [0.0, 0.0, 0.0, 0.1],
}

/////////////////////
// ACRYLIC BLOCK
/////////////////////
{
  name: "GEO",
  index: "block",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "inner",
  type: "box",
  size: [160.0,160.0,32.5],
  position: [-398.0, -367.0, -270.0],
  material: "chsrc_uvt_acrylic",
  //surface: "chsrc_uvt_acrylic",
  color: [0.1, 0.3, 0.8, 0.1],
}

/////////////////////
// COVER
/////////////////////
{
  name: "GEO",
  index: "cover",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "block",
  type: "box",
  size: [160.0,160.0,0.5],
  position: [0.0,0.0,32.0],
  material: "chsrc_uvt_acrylic",
  surface: "chsrc_uvt_acrylic",
  color: [0.5, 0.2, 0.1, 0.1],
}

// {
//   name: "GEO",
//   index: "border_cover",
//   valid_begin: [0, 0],
//   valid_end: [0, 0],
//   invisible: 1, // omitted for visualization
//   mother: "block",
//   type: "border",
//   volume1: "block",
//   volume2: "cover",
//   surface: "mirror",
//   reverse: 0,
//   color: [0.0, 0.0, 0.0, 0.1],
// }

{
  name: "GEO",
  index: "cover_window",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "cover",
  type: "tube",
  r_max: 50.0,
  size_z: 0.5,
  position: [0.0,0.0,0.0],
  material: "chsrc_uvt_acrylic", //optical_grease_eljen
  surface: "chsrc_uvt_acrylic",
  color: [0.1, 0.3, 0.8, 0.1],
}

{
  name: "GEO",
  index: "cover_hollow",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "cover_window",
  type: "tube",
  r_max: 5.0,
  size_z: 0.5,
  position: [0.0,0.0,0.0],
  material: "acrylic_white",
  surface: "acrylic_white",
  color: [0.0, 0.0, 0.0, 0.1],
}

{
  name: "GEO",
  index: "hollow",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "block",
  type: "tube",
  position: [0.0, 0.0, -0.5], //0.5
  r_max: 5.0,
  size_z: 32.0,
  material: "acrylic_white",
  surface: "acrylic_white",
  color: [0.0, 0.0, 0.0, 0.1],
}

{
  name: "GEO",
  index: "chip",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "block",
  type: "tube",
  position: [0.0, 0.0, -31.5],
  r_max: 10.0,
  r_min: 5.0,
  size_z: 1.0,
  material: "air",
  color: [0.0, 0.0, 0.0, 0.1],
}

// {
//   name: "GEO",
//   index: "chip_tophalf",
//   valid_begin: [0, 0],
//   valid_end: [0, 0],
//   invisible: 0, // omitted for visualization
//   mother: "block",
//   type: "tube",
//   position: [0.0, 0.0, -31.0],
//   r_max: 10.0,
//   size_z: 0.5, //1.0
//   material: "air",
//   color: [0.0, 0.0, 0.0, 0.1],
// }


////////////////////
//  PMT COUPLING
////////////////////
// {
//   name: "GEO",
//   index: "pmt_coupling",
//   valid_begin: [0, 0],
//   valid_end: [0, 0],
//   invisible: 0, // omitted for visualization
//   mother: "block",
//   type: "box",
//   size: [160.0,160.0,0.5],
//   position: [0.0,0.0,-32.0],
//   material: "chsrc_uvt_acrylic", //optical_grease_eljen
//   color: [0.1, 0.5, 0.5, 0.1],
// }

// {
//   name: "GEO",
//   index: "chip_bottomhalf",
//   valid_begin: [0, 0],
//   valid_end: [0, 0],
//   invisible: 0, // omitted for visualization
//   mother: "pmt_coupling",
//   type: "tube",
//   position: [0.0, 0.0, 0.0],
//   r_max: 10.0,
//   size_z: 0.5, //1.0
//   material: "air",
//   color: [0.0, 0.0, 0.0, 0.1],
// }


/////////////////
//  VESSEL
/////////////////
{
  name: "GEO",
  index: "vessel",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "inner",
  type: "CheSSVessel",
  r_max: 50.0,
  size_z: 15.0,
  rotation:  [0.0, 0.0, 135.0],
  position: [-398.0, -367.0, -220.91],
  material: "chsrc_uvt_acrylic",
  color: [0.1, 0.3, 0.8, 0.1],
  cavity: false,
  pmts: 0
}

// {
//   name: "GEO",
//   index: "border_vessel_1",
//   valid_begin: [0, 0],
//   valid_end: [0, 0],
//   invisible: 1, // omitted for visualization
//   mother: "block",
//   type: "border",
//   volume1: "vessel",
//   volume2: "block",
//   surface: "optical_grease_eljen",
//   reverse: 0,
//   color: [0.0, 0.0, 0.0, 0.1],
// }

// {
//   name: "GEO",
//   index: "border_vessel_2",
//   valid_begin: [0, 0],
//   valid_end: [0, 0],
//   invisible: 1, // omitted for visualization
//   mother: "block",
//   type: "border",
//   volume1: "block",
//   volume2: "vessel",
//   surface: "optical_grease_eljen",
//   reverse: 0,
//   color: [0.0, 0.0, 0.0, 0.1],
// }

{
  name: "GEO",
  index: "cavity",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "vessel",
  type: "tube",
  r_max: 20.0,
  size_z: 1.905,
  position: [0.0, 0.0, 14.685],
  material: "air",
  color: [0.0, 0.0, 0.0, 0.1],
}

{
  name: "GEO",
  index: "content",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "vessel",
  type: "tube",
  r_max: 45.0, //LAB->45mm, WBLS->35mm
  size_z: 12.89,
  position: [0.0, 0.0, -1.7],
  material: "lab",
  color: [0.5, 0.1, 0.5, 0.5],
}

///////////////////////
// COSMIC TAGS
///////////////////////
{
  name: "GEO",
  index: "tag1",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "inner",
  type: "box",
  position: [-398.0, -367.0, -149.0],
  size: [10.0,10.0,55.0],
  material: "acrylic_black",
  color: [0.0, 0.0, 0.0, 0.1],
}

{
  name: "GEO",
  index: "tag1_scint",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 1, // omitted for visualization
  mother: "tag1",
  type: "tube",
  position: [0.0,0.0,-29.9],
  r_max: 5.0,
  size_z: 25.0,
  material: "water", //water, scintillator
  color: [0.0, 0.0, 0.0, 0.1],
}

{
  name: "GEO",
  index: "tag1_coat",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 1, // omitted for visualization
  mother: "tag1",
  type: "border",
  volume1: "tag1_scint",
  volume2: "tag1",
  surface: "mirror",
  reverse: 0,
  color: [0.0, 0.0, 0.0, 0.1],
}

{
  name: "GEO",
  index: "tag2",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "inner",
  type: "box",
  position: [-398.0, -367.0, -357.5],
  size: [15.0,15.0,55.0],
  material: "acrylic_black",
  color: [0.0, 0.0, 0.0, 0.1],
}

{
  name: "GEO",
  index: "tag2_scint",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 1, // omitted for visualization
  mother: "tag2",
  type: "tube",
  position: [0.0,0.0,29.0],
  r_max: 5.0,
  size_z: 25.0,
  material: "water", //water, scintillator
  color: [0.0, 0.0, 0.0, 0.1],
}

{
  name: "GEO",
  index: "tag2_coat",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 1, // omitted for visualization
  mother: "tag2",
  type: "border",
  volume1: "tag2_scint",
  volume2: "tag2",
  surface: "mirror",
  reverse: 0,
  color: [0.0, 0.0, 0.0, 0.1],
}
/////////////////////////////////

//////////////////
// PMT holder
//////////////////
{
  name: "GEO",
  index: "pmtframe",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 1, // omitted for visualization
  mother: "inner",
  type: "box",
  size: [160.0,160.0,16.0],
  position: [-398.0, -367.0, -318.5],
  material: "acrylic_white", //aluminum
  surface: "acrylic_white", //aluminum
  color: [0.5, 0.0, 0.0, 0.5],
}

{
  name: "GEO",
  index: "pmtgrid",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 1, // omitted for visualization
  mother: "pmtframe",
  type: "box",
  size: [130.0,130.0,16.0],
  position: [0.0, 0.0, 0.0],
  material: "acrylic_white",
  surface: "acrylic_white",
  color: [0.0, 0.0, 0.0, 0.1],
}

// {
//   name: "GEO",
//   index: "border_frame_1",
//   valid_begin: [0, 0],
//   valid_end: [0, 0],
//   invisible: 1, // omitted for visualization
//   mother: "pmtframe",
//   type: "border",
//   volume1: "pmtframe",
//   volume2: "block",
//   surface: "mirror",
//   reverse: 0,
//   color: [0.0, 0.0, 0.0, 0.1],
// }

// {
//   name: "GEO",
//   index: "border_frame_2",
//   valid_begin: [0, 0],
//   valid_end: [0, 0],
//   invisible: 1, // omitted for visualization
//   mother: "pmtframe",
//   type: "border",
//   volume1: "block",
//   volume2: "pmtframe",
//   surface: "mirror",
//   reverse: 0,
//   color: [0.0, 0.0, 0.0, 0.1],
// }

//////////////////
// Muon panels
//////////////////
{
  name: "GEO",
  index: "panel_east",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 1, // omitted for visualization
  mother: "inner",
  type: "box",
  size: [25.0,500.0,250.0],
  position: [-686.2, -130.0, -180.0],
  material: "acrylic_black",
  surface: "acrylic_black",
  color: [0.0, 0.5, 0.5, 0.7],
}

{
  name: "GEO",
  index: "panel_north",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 1, // omitted for visualization
  mother: "inner",
  type: "box",
  size: [500.0,25.0,250.0],
  position: [-210.0, -660.0, -180.0],
  material: "acrylic_black",
  surface: "acrylic_black",
  color: [0.0, 0.5, 0.5, 0.7],
}


//////////////////
// PMTS
//////////////////

{
  name: "GEO",
  index: "ring_pmts",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "pmtgrid",
  type: "pmtarray",
  pmt_model: "h11934",
  pmt_detector_type: "idpmt",
  sensitive_detector: "/mydet/pmt/inner",
  pos_table: "PMTINFO_CROSS_SIDE",
  orientation: "manual",
}

{
  name: "GEO",
  index: "light_pmts",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner",
  type: "pmtarray",
  pmt_model: "r7081_rev",
  pmt_detector_type: "idpmt",
  sensitive_detector: "/mydet/pmt/inner",
  pos_table: "PMTINFO_CLOSE",
  orientation: "point",
  orient_point: [-398.0, -367.0, -237.0],
}

{
  name: "GEO",
  index: "topmuon_pmt",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "tag1",
  type: "pmtarray",
  pmt_model: "h3164-10",
  pmt_detector_type: "idpmt",
  sensitive_detector: "/mydet/pmt/tag1",
  pos_table: "PMTINFO_MUON_TOPTAG",
  orientation: "manual",
  orient_point: [-400.0, -400.0, -200.0],
}

{
  name: "GEO",
  index: "bottommuon_pmt",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "tag2",
  type: "pmtarray",
  pmt_model: "h3164-10",
  pmt_detector_type: "idpmt",
  sensitive_detector: "/mydet/pmt/tag2",
  pos_table: "PMTINFO_MUON_BOTTOMTAG",
  orientation: "manual",
  orient_point: [-400.0, -400.0, -200.0],
}

{
  name: "GEO",
  index: "trigger_pmt",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "vessel",
  type: "pmtarray",
  pmt_model: "h11934",
  pmt_detector_type: "idpmt",
  sensitive_detector: "/mydet/pmt/inner",
  pos_table: "PMTINFO_TRIGGER",
  orientation: "point",
  orient_point: [-398.0, -367.0, -222.2],
}

{
   name: "GEO",
   index: "panels_pmts",
   valid_begin: [0, 0],
   valid_end: [0, 0],
   mother: "world",
   type: "pmtarray",
   pmt_model: "h11934",
   pmt_detector_type: "idpmt",
   sensitive_detector: "/mydet/pmt/inner",
   pos_table: "PMTINFO_PANELS",
   orientation: "manual",
   orient_point: [0.0, 0.0, 400.0],
}

{
   name: "GEO",
   index: "control_channel",
   valid_begin: [0, 0],
   valid_end: [0, 0],
   mother: "world",
   type: "pmtarray",
   pmt_model: "h11934",
   pmt_detector_type: "idpmt",
   sensitive_detector: "/mydet/pmt/inner",
   pos_table: "PMTINFO_XCH",
   orientation: "manual",
   orient_point: [0.0, 0.0, 400.0],
}

