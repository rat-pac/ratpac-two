{
name: "GEO",
index: "SphereShield",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "eos_inner",
type: "sphere",

material: "tungsten",
position: [0, 0, 0],

r_max: 20.0,
color: [0.0, 1.0, 0.0, 0.8],
}
{
name: "GEO",
index: "SphereModerator",
valid_begin: [0, 0],
valid_end: [0, 0],
mother: "SphereShield",
type: "sphere",

material: "polypropylene",
position: [0, 0, 0],

r_max: 10.0,

color: [0.0, 0.0, 1.0, 0.8],
}
