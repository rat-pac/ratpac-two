{
  name: "GEO",
  index: "world",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "",
  type: "sphere",
  r_max: 4500.0,
  material: "validwater",
  invisible: 0,
}

{
  name: "GEO",
  index: "detector",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "world",
  type: "sphere",
  r_max: 3500,
  material: "validwater",
  color: [1.0, 0.5, 0.0, 0.75]
}

//////////////////
// PMTS
//////////////////
{
  name: "GEO",
  index: "pmts",
  valid_begin: [0, 0],
  valid_end: [0, 0],
  mother: "world",
  type: "pmtarray",
  pmt_model: "validPMT",
  pmt_detector_type: "idpmt",
  sensitive_detector: "/mydet/pmt/inner",
  pos_table: "PMTINFO_valid",
  orientation: "manual",
}
