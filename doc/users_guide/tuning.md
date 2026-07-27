# MC tuning options

Some studies do not need a full, faithful simulation. When you are fitting a PMT
time PDF you only care about the photons that actually reach a PMT, and when you
are profiling the non-optical part of an event you do not want the optical
simulation running at all.

The `MC` RATDB table exposes a **tuning** hook for exactly these cases. Tuning
changes the physics that is simulated or the information that is retained, so it
is strictly opt-in: with no tuning configured, ratpac-two behaves exactly as it
always has.

```{contents} Table of Contents
:depth: 2
```

## Enabling a tuning option

Two RATDB fields control tuning, and both must be set before `/run/initialize`:

| Field | Type | Meaning |
| --- | --- | --- |
| `tuning` | bool | Master switch. Tuning is applied only when this is `true`. |
| `tuning_option` | string | Which tuning to apply. |

Neither field is present in the default `ratdb/MC.ratdb`, so set them from your
macro:

```
/rat/db/set MC tuning true
/rat/db/set MC tuning_option "profile"

/run/initialize
```

Only one option can be active at a time — `tuning_option` is a single string, not
a list.

The fields are read once per run, in `Gsim::BeginOfRunAction`. Changing them
between `/run/beamOn` calls in the same macro will take effect on the next run.

## Available options

### `timepdf` — keep only photon tracks that hit a PMT

Optical-photon trajectories are discarded unless the photon entered a PMT
optical model. Photons are still fully simulated and still produce
photoelectrons; only the *stored trajectories* of the photons that never
reached a PMT are dropped.

This is what you want when generating a PMT hit-time PDF: you get the full
arrival-time information without writing out the overwhelming majority of
photons that were absorbed or lost in the detector.

```
/rat/db/set MC tuning true
/rat/db/set MC tuning_option "timepdf"

/run/initialize

/tracking/storeTrajectory 1
```

Because this option filters trajectories, it only does anything when trajectory
storage is enabled with `/tracking/storeTrajectory 1`. See
`macros/examples/timepdf.mac` for a complete working macro.

### `profile` — kill optical photons as soon as they are emitted

Every optical photon is killed on its first step, so no optical propagation is
simulated at all. Scintillation and Cherenkov photons are still *generated* —
the generating processes run normally, and the energy they consume is still
accounted for — but the photons are stopped before they travel anywhere.

The consequence is that **no photoelectrons and no PMT hits are produced**. A
20-event run of `macros/examples/timepdf.mac` gives 320 PEs across 309 MCPMTs
untuned, and 0 PEs across 0 MCPMTs with `profile` enabled.

```
/rat/db/set MC tuning true
/rat/db/set MC tuning_option "profile"

/run/initialize
```

Use this to profile or debug the non-optical part of an event — primary
particle transport, energy deposition, secondaries — without paying for the
optical simulation, or as a fast baseline when you want to confirm that a
change in the output is optical in origin.

Note that killed photons are still created as tracks. If trajectory storage is
on they are written out with a single step each, so `profile` does **not** by
itself reduce the track count in the output — the same 20-event run stored 14811
tracks tuned versus 14768 untuned, while wall-clock dropped from 27.1 s to
20.4 s (most of what remains is fixed geometry setup). Combine it with
`/tracking/storeTrajectory 0` or the `prune` processor (`mc.track:opticalphoton`)
if output size is the concern.

## Error handling

Tuning fails soft, never fatally:

* `tuning` absent or `false` — no tuning is applied, silently. This is the
  default and keeps older `MC` tables working unchanged.
* `tuning` true, `tuning_option` absent — no tuning is applied, silently.
* `tuning` true, `tuning_option` unrecognized — no tuning is applied and a
  warning is logged:

  ```
  Gsim: Unknown MC tuning option "..."; no tuning will be applied
  ```

When an option *is* applied, `Gsim` logs it at info level, so check the run
header to confirm the tuning you asked for is actually active:

```
Gsim: Time-PDF tuning enabled; only optical-photon trajectories that interact with a PMT will be saved
Gsim: Profile tuning enabled; optical photons will be killed as soon as they are emitted
```

## Adding a new tuning option

The dispatch lives in `Gsim::BeginOfRunAction` (`src/core/src/Gsim.cc`). Add a
branch comparing `tuning_option` against your new name, and have it set whatever
flag your feature reads.

Reset that flag to its default at the top of the same block, next to the
existing resets. `BeginOfRunAction` runs once per run, and the flags for
`profile` and `timepdf` are static/member state that would otherwise leak from a
tuned run into an untuned one later in the same macro.
