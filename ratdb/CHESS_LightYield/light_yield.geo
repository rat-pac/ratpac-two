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
  size: [3000.0,3000.0,600.0], //mm, half-length
  material: "air",
}

{
  name: "GEO",
  index: "darkbox",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "world",
  type: "box",
  size: [762.0,762.0,508.0], //mm, half-length
  material: "acrylic_black", //acrylic_black
  invisible: 1, // omitted for visualization
}

{
  name: "GEO",
  index: "inner",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "darkbox",
  type: "box",
  size: [711.2,711.2,457.2], //mm, half-length
  material: "air",
  invisible: 1, // omitted for visualization
}

/////////////////////////////
// CONTAINER
/////////////////////////////

{
  name: "GEO",
  index: "vessel",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "inner",
  type: "tube",
  size_z: 15.0,
  r_max: 15.0,
  position: [500.0, 560.0, -157.5],
  rotation:  [0.0, 0.0, 0.0],
  material: "acrylic_sno",
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
  size_z: 12.5,
  r_max: 10.0,
  position: [0.0, 0.0, -0.675],
  material: "water",
  color: [0.0, 0.0, 1.0, 0.25],
}

{
  name: "GEO",
  index: "air_gap",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "content",
  type: "tube",
  size_z: 0.5,
  r_max: 10.0,
  position: [0.0, 0.0, 12.0],
  material: "air",
  color: [0.0, 0.0, 0.0, 0.1],
}

{
  name: "GEO",
  index: "pmtholder",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 1, // omitted for visualization
  mother: "inner",
  type: "box",
  size: [35.0,35.0,15.0], //mm, half-lenght
  position: [515.5,575.5,-187.7],
  material: "acrylic_black",
  color: [0.0, 0.0, 0.0, 0.1],
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
  mother: "vessel",
  type: "tube",
  position: [0.0, 0.0, 13.4125], // -144.0785
  rotation:  [0.0, 0.0, 0.0],
  r_max: 12.8,
  size_z: 1.5875, // half height
  material: "acrylic_black",
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
  position: [0.0, 0.0, -1.157],
  r_max: 3.175,
  size_z: 0.05, //half height
  material: "acrylic_white", //strontium, acrylic_white
  color: [0.1, 1.0, 1.0, 0.8],
}

//////////////////
// PMTS
//////////////////
{
  name: "GEO",
  index: "light_yield_pmt",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner",
  type: "pmtarray",
  pmt_model: "r7081_rev", //r7081_rev
  pmt_detector_type: "idpmt",
  sensitive_detector: "/mydet/pmt/inner",
  pos_table: "PMTINFO_LIGHT_YIELD",
  orientation: "manual",
  orient_point: [500.0, 560.0, -150.0],
}

{
  name: "GEO",
  index: "trigger_pmt",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "pmtholder",//inner
  type: "pmtarray",
  pmt_model: "h11934",
  pmt_detector_type: "idpmt",
  sensitive_detector: "/mydet/pmt/inner",
  pos_table: "PMTINFO_TRIGGER",
  orientation: "manual",
}

