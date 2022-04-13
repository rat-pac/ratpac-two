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
  color: [0.1, 0.3, 0.8, 0.1],
}

/////////////////////
// COVER
/////////////////////
//{
//  name: "GEO",
//  index: "cover_air",
//  valid_begin: [0, 0],
//  valid_end: [0, 0],
//  invisible: 0, // omitted for visualization
//  mother: "block",
//  type: "box",
//  size: [160.0,160.0,0.05],
//  position: [0.0,0.0,32.45],
//  material: "felt", //air
//  surface: "felt", //none
//  color: [0.5, 0.2, 0.1, 0.1],
//}

//{
//  name: "GEO",
//  index: "cover",
//  valid_begin: [0, 0],
//  valid_end: [0, 0],
//  invisible: 0, // omitted for visualization
//  mother: "block",
//  type: "box",
//  size: [160.0,160.0,0.45],
//  position: [0.0,0.0,31.95],
//  material: "felt",
//  surface: "felt",
//  color: [0.5, 0.2, 0.1, 0.1],
//}

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
  pmts: 0,
  cavity: true,
  rotation:  [0.0, 0.0, 45.0],
  position: [-398.0, -367.0, -220.91], //center
  //position: [-288.0, -257.0, -220.91], //corner
  material: "chsrc_uvt_acrylic",
  color: [0.1, 0.3, 0.8, 0.1],
}

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

//////////////////
// PMT holder
//////////////////
{
  name: "GEO",
  index: "pmtframe",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
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
  invisible: 0, // omitted for visualization
  mother: "pmtframe",
  type: "box",
  size: [130.0,130.0,16.0],
  position: [0.0, 0.0, 0.0],
  material: "acrylic_white",
  surface: "acrylic_white",
  color: [0.0, 0.0, 0.0, 0.1],
}

//////////////////
// PMTS
//////////////////
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
   //pos_table: "PMTINFO_TRIGGER_CORNER",
   orientation: "point",
   orient_point: [-398.0, -367.0, -222.5],
}

//{
//   name: "GEO",
//   index: "cotrigger_pmt",
//   valid_begin: [0, 0],
//   valid_end: [0, 0],
//   mother: "vessel",
//   type: "pmtarray",
//   pmt_model: "h11934",
//   pmt_detector_type: "idpmt",
//   sensitive_detector: "/mydet/pmt/inner",
//   pos_table: "PMTINFO_COTRIGGER",
//   orientation: "point",
//   orient_point: [-398.0, -367.0, -222.5],
//}

{
  name: "GEO",
  index: "ring_pmts",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "pmtgrid",
  type: "pmtarray",
  pmt_model: "h11934", //h11934
  pmt_detector_type: "idpmt",
  sensitive_detector: "/mydet/pmt/inner",
  pos_table: "PMTINFO_CROSS_SIDE_UPGRADED",
  orientation: "manual",
  color: [0.1, 1.0, 0.3, 0.8],
}

{
  name: "GEO",
  index: "lappd_left",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  //drawstyle: "solid",
  mother: "inner",
  type: "pmtarray",
  pmt_model: "lappd",
  pmt_detector_type: "idpmt",
  sensitive_detector: "/mydet/pmt/inner",
  pos_table: "PMTINFO_LAPPD_LEFT",
  orientation: "manual",
}

{
  name: "GEO",
  index: "lappd_right",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  //drawstyle: "solid",
  mother: "inner",
  type: "pmtarray",
  pmt_model: "lappd",
  pmt_detector_type: "idpmt",
  sensitive_detector: "/mydet/pmt/inner",
  pos_table: "PMTINFO_LAPPD_RIGHT",
  orientation: "manual",
}


/////////////////////////////
// RADIACTIVE SOURCES
/////////////////////////////

//TOP
{
  name: "GEO",
  index: "button_source",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "cavity",			        
  type: "tube",			        
  position: [0.0,0.0,-0.318], //-0.318       
  rotation:  [180.0, 0.0, 0.0],	        
  r_max: 12.808,
  size_z: 1.587, //half height
  material: "acrylic_white",
  surface: "acrylic_white",
  color: [0.1, 1.0, 0.3, 0.8],
}

{
  name: "GEO",
  index: "source",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization 
  mother: "button_source",		        
  type: "tube",
  position: [0.0, 0.0, 1.157],
  r_max: 3.175,
  size_z: 0.05, //half height
  material: "acrylic_white", //strontium, acrylic_white
  surface: "acrylic_white",
  color: [0.1, 1.0, 1.0, 0.8],
}

