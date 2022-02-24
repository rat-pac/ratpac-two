{
  name: "GEO",
  index: "world",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "",
  type: "box",
  size: [2500.0,2500.0,2500.0], //mm, half-length
  material: "water",
}

{
  name: "GEO",
  index: "eos",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "world",
  type: "eos",
  r_min: 888.8,
  r_max: 914.4,
  size_z: 533.0,
  top_radius: 914.4,
  top_height: 475.0,
  offset: 0,
  rotation:  [0.0, 0.0, 0.0],
  position: [0.0, 0.0, 0.0],
  material: "acrylic_uvt",
}

{
  name: "GEO",
  index: "eos_inner",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  invisible: 0, // omitted for visualization
  mother: "world",
  type: "eos",
  r_min: 0.0,
  r_max: 888.8,
  size_z: 533.0,
  top_radius: 888.8,
  top_height: 449.4,
  offset: 1.0,
  rotation:  [0.0, 0.0, 0.0],
  position: [0.0, 0.0, 0.0],
  material: "wbls_5pct_WM_0820",
}


//////////////////
// PMTS
//////////////////
//
{
  name: "GEO",
  index: "pmts",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "world",
  type: "pmtarray",
  pmt_model: "r14688",
  pmt_detector_type: "idpmt",
  sensitive_detector: "/mydet/pmt/inner",
  pos_table: "PMTINFO",
  orientation: "manual",
}

{
  name: "GEO",
  index: "pmts_hqe",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "world",
  type: "pmtarray",
  pmt_model: "r11780_hqe",
  pmt_detector_type: "idpmt",
  sensitive_detector: "/mydet/pmt/inner",
  pos_table: "PMTINFO_HQE",
  orientation: "manual",
}

