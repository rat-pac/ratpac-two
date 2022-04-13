{
  name: "GEO",
  index: "world",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "",
  type: "box",
  size: [2500.0,2500.0,2500.0],
  material: "air",
}

// Tank dimensions are from docDB-29
{
  name: "GEO",
  index: "outer_tank",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "world",
  type: "tube",
  r_max: 1397.2,
  size_z: 1397.2,
  position: [0.0, 0.0, 0.0],
  rotation: [0.0, 0.0, 0.0],
  material: "stainless_steel",
}

{
  name: "GEO",
  index: "inner_tank",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "outer_tank",
  type: "tube",
  r_max: 1371.6,
  size_z: 1371.6,
  position: [0.0, 0.0, 0.0],
  rotation: [0.0, 0.0, 0.0],
  material: "water",
}

{
  name: "GEO",
  index: "neck",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "tube",
  r_max: 111.76,
  size_z: 181.7,
  position: [0.0, 0.0, 1189.8],
  rotation: [0.0, 0.0, 0.0],
  material: "stainless_steel",
}

// Eos dimensions are from docDB-29
{
  name: "GEO",
  index: "eos_vessel",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "eos",
  // Cylindrical part
  r_min: 888.8,
  r_max: 914.4,
  size_z: 533.0,
  // Elliptical top/bottom
  top_radius: 914.4,
  top_height: 475.0,
  position: [0.0, 0.0, 0.0],
  rotation: [0.0, 0.0, 0.0],
  material: "acrylic_sno",
}

{
  name: "GEO",
  index: "eos_inner",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "eos",
  // Cylindrical part
  r_min: 0.0,
  r_max: 888.8,
  size_z: 533.0,
  // Elliptical top/bottom
  top_radius: 888.8,
  top_height: 449.4,
  rotation:  [0.0, 0.0, 0.0],
  position: [0.0, 0.0, 0.0],
  material: "wbls_5pct_WM_0820",
}

{
  name: "GEO",
  index: "port",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "tube",
  r_min: 304.8,
  r_max: 355.6,
  size_z: 5.1,
  position: [0.0, 0.0, 975.7],
  rotation: [0.0, 0.0, 0.0],
  material: "acrylic_sno",
}

// Acrylic legs dimensions are from docDB-29
{
  name: "GEO",
  index: "bottom_leg1",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "tube",
  r_min: 888.8,
  r_max: 914.4,
  phi_start: -20.0,
  phi_delta: 40.0,
  //size_z: 342.9,
  size_z: 419.3,
  //position: [0.0, 0.0, -875.9],
  position: [0.0, 0.0, -952.3],
  rotation: [0.0, 0.0, 0.0],
  material: "acrylic_sno",
}

{
  name: "GEO",
  index: "bottom_leg2",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "tube",
  r_min: 888.8,
  r_max: 914.4,
  phi_start: 70.0,
  phi_delta: 40.0,
  //size_z: 342.9,
  size_z: 419.3,
  //position: [0.0, 0.0, -875.9],
  position: [0.0, 0.0, -952.3],
  rotation: [0.0, 0.0, 0.0],
  material: "acrylic_sno",
}

{
  name: "GEO",
  index: "bottom_leg3",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "tube",
  r_min: 888.8,
  r_max: 914.4,
  phi_start: 160.0,
  phi_delta: 40.0,
  //size_z: 342.9,
  size_z: 419.3,
  //position: [0.0, 0.0, -875.9],
  position: [0.0, 0.0, -952.3],
  rotation: [0.0, 0.0, 0.0],
  material: "acrylic_sno",
}

{
  name: "GEO",
  index: "bottom_leg4",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "tube",
  r_min: 888.8,
  r_max: 914.4,
  phi_start: 250.0,
  phi_delta: 40.0,
  //size_z: 342.9,
  size_z: 419.3,
  //position: [0.0, 0.0, -875.9],
  position: [0.0, 0.0, -952.3],
  rotation: [0.0, 0.0, 0.0],
  material: "acrylic_sno",
}

{
  name: "GEO",
  index: "top_leg1",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "tube",
  r_min: 888.8,
  r_max: 914.4,
  phi_start: -20.0,
  phi_delta: 40.0,
  //size_z: 342.9,
  size_z: 419.3,
  //position: [0.0, 0.0, 875.9],
  position: [0.0, 0.0, 952.3],
  rotation: [0.0, 0.0, 0.0],
  material: "acrylic_sno",
}

{
  name: "GEO",
  index: "top_leg2",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "tube",
  r_min: 888.8,
  r_max: 914.4,
  phi_start: 70.0,
  phi_delta: 40.0,
  //size_z: 342.9,
  size_z: 419.3,
  //position: [0.0, 0.0, 875.9],
  position: [0.0, 0.0, 952.3],
  rotation: [0.0, 0.0, 0.0],
  material: "acrylic_sno",
}

{
  name: "GEO",
  index: "top_leg3",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "tube",
  r_min: 888.8,
  r_max: 914.4,
  phi_start: 160.0,
  phi_delta: 40.0,
  //size_z: 342.9,
  size_z: 419.3,
  //position: [0.0, 0.0, 875.9],
  position: [0.0, 0.0, 952.3],
  rotation: [0.0, 0.0, 0.0],
  material: "acrylic_sno",
}

{
  name: "GEO",
  index: "top_leg4",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "tube",
  r_min: 888.8,
  r_max: 914.4,
  phi_start: 250.0,
  phi_delta: 40.0,
  //size_z: 342.9,
  size_z: 419.3,
  //position: [0.0, 0.0, 875.9],
  position: [0.0, 0.0, 952.3],
  rotation: [0.0, 0.0, 0.0],
  material: "acrylic_sno",
}

//////////////////
// PMTS
//////////////////
//
{
  name: "GEO",
  index: "pmts_side",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "pmtarray",
  pmt_model: "r14688",
  pmt_detector_type: "idpmt",
  sensitive_detector: "/mydet/pmt/inner",
  pos_table: "PMTINFO_side_1cm_central_gap",
  orientation: "manual",
}

{
  name: "GEO",
  index: "pmts_bottom",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "pmtarray",
  pmt_model: "r14688",
  pmt_detector_type: "idpmt",
  sensitive_detector: "/mydet/pmt/inner",
  pos_table: "PMTINFO_bottom_ring_36",
  orientation: "manual",
}

{
  name: "GEO",
  index: "pmts_top",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "pmtarray",
  pmt_model: "r14688",
  pmt_detector_type: "idpmt",
  sensitive_detector: "/mydet/pmt/inner",
  pos_table: "PMTINFO_top_ring_36",
  orientation: "manual",
}

{
  name: "GEO",
  index: "pmts_central",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "pmtarray",
  pmt_model: "r11780_hqe",
  pmt_detector_type: "idpmt",
  sensitive_detector: "/mydet/pmt/inner",
  pos_table: "PMTINFO_side_hqe_central",
  orientation: "manual",
}

