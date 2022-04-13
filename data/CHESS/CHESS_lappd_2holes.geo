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


/////////////////////////////
// CUVETTE
/////////////////////////////

{
  name: "GEO",
  index: "cuvette",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "inner",
  type: "box",
  size: [6.25,6.25,22.5], //mm, half-lenght
  position: [0.0, 0.0, 22.5],
  rotation:  [0.0, 0.0, 0.0],
  material: "quartz",
  color: [0.0, 0.0, 0.0, 0.1],
}

{
  name: "GEO",
  index: "cap",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "inner",
  type: "box",
  size: [6.25,6.25,1.0], //mm, half-lenght
  position: [0.0, 0.0, 46.0], //10cm from PMT
  rotation:  [0.0, 0.0, 0.0],
  material: "acrylic_white",
  surface: "acrylic_white",
  color: [0.0, 0.0, 0.0, 0.1],
}

{
  name: "GEO",
  index: "content",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "cuvette",
  type: "box",
  size: [5.0,5.0,21.875], //mm, half-lenght
  position: [0.0,0.0,0.625],
  material: "water",
  color: [0.0, 0.0, 0.0, 0.1],
}

{
  name: "GEO",
  index: "mask",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "inner",
  type: "box",
  size: [105.5, 105.5, 0.05],
//  size: [6.25,6.25,0.05],
  position: [0.0, 0.0, -0.05],
  rotation:  [0.0, 0.0, 0.0],
  material: "acrylic_black",
  color: [0.0, 0.0, 0.0, 0.1],
}

{
  name: "GEO",
  index: "hole_1",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "mask",
  type: "tube",
  r_max: 3.5,
  size_z: 0.05,
  position: [0.0, 34.9, 0.0], //18
  rotation:  [0.0, 0.0, 0.0],
  material: "air",
  color: [0.0, 0.0, 0.0, 0.1],
}

{
  name: "GEO",
  index: "hole_2",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "mask",
  type: "tube",
  r_max: 3.5,
  size_z: 0.05,
  position: [0.0, -34.9, 0.0], //9
  rotation:  [0.0, 0.0, 0.0],
  material: "air",
  color: [0.0, 0.0, 0.0, 0.1],
}


//////////////////
// PMTS
//////////////////
{
  name: "GEO",
  index: "lappd",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "inner",
  type: "pmtarray",
  pmt_model: "lappd",
  pmt_detector_type: "idpmt",
  sensitive_detector: "/mydet/pmt/inner",
  pos_table: "PMTINFO_LAPPD",
  orientation: "manual",
}

//{
//   name: "GEO",
//   index: "trigger_pmt",
//   valid_begin: [0, 0],
//   valid_end: [0, 0],
//   mother: "inner",
//   type: "pmtarray",
//   pmt_model: "h11934",
//   pmt_detector_type: "idpmt",
//   sensitive_detector: "/mydet/pmt/inner",
//   pos_table: "PMTINFO_TRIGGER_LAPPD_CENTER",
//   orientation: "manual",
////   orient_point: [-398.0, -367.0, -222.5],
//}

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
  mother: "inner",
  type: "tube",
  position: [0.0,20.0,48.587], //VME other side
  rotation:  [0.0, 180.0, 0.0],
  r_max: 12.808,
  size_z: 1.587, //half height
  material: "acrylic_black",
  surface: "acrylic_black",
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
