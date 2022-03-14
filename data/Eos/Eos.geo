{
  name: "GEO",
  index: "world",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "",
  type: "box",
  size: [2500.0,2500.0,2500.0], //mm, half-length
  material: "air",
  invisible: 1,
}

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
  color: [0.42, 0.47, 0.57, 0.95],
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
  color: [0.2, 0.2, 0.9, 0.2],
}

{
  name: "GEO",
  index: "neck",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "tube",
  r_max: 76.2,
  size_z: 181.7,
  position: [0.0, 0.0, 1189.8],
  rotation: [0.0, 0.0, 0.0],
  material: "stainless_steel",
  color: [0.42, 0.47, 0.57, 0.95],
}

{
  name: "GEO",
  index: "eos_vessel",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "eos",
  r_min: 888.8,
  r_max: 914.4,
  size_z: 533.0,
  top_radius: 914.4,
  top_height: 475.0,
  position: [0.0, 0.0, 0.0],
  rotation: [0.0, 0.0, 0.0],
  material: "acrylic_sno",
  //color: [0.12, 0.70, 0.95, 0.1],
  color: [0.6, 0.2, 0.2, 0.6],
}

{
  name: "GEO",
  index: "eos_inner",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "eos",
  r_min: 0.0,
  r_max: 888.8,
  size_z: 533.0,
  top_radius: 888.8,
  top_height: 449.4,
  rotation:  [0.0, 0.0, 0.0],
  position: [0.0, 0.0, 0.0],
  material: "wbls_5pct_WM_0820",
  color: [0.4, 0.2, 0.9, 0.2],
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
  pos_table: "PMTINFO_side_1cm",
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
  pos_table: "PMTINFO_bottom_ring",
  orientation: "manual",
}

{
  name: "GEO",
  index: "pmts_hqe",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "inner_tank",
  type: "pmtarray",
  pmt_model: "r11780_hqe",
  pmt_detector_type: "idpmt",
  sensitive_detector: "/mydet/pmt/inner",
  pos_table: "PMTINFO_hqe_top_ring",
  orientation: "manual",
}
