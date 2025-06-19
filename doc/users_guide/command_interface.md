# Controlling ratpac-two via macro files 

This section of the guide will provide a high-level overview of controlling ratpac-two, which is accomplished through the use of macro files.
Macro files are plain text files containing a sequence of commands that control ratpac-two.
Using macros, you can configure various aspects of a ratpac experiment, such as:

* Detector geometry, materials, and calibration constants (via RATDB, see :doc:`database`).
* Data processing chain (processors for digitization, reconstruction, output, etc., see :doc:`processors`).
* Data processing chain (processors for digitization, reconstruction, output, etc.).
* Primary particle sources (event generators).
* Run-specific parameters like the number of events and random number seeds.

Macro files are structured to follow your ratpac-two workflow.
This is typically a **pipeline** connecting event **producers** to a sequence of **processors**.
Each producer creates or loads an event and then passes it through the processors.
Each processor can modify the event, record information, or simply observe it, and the order in which you declare them determines how every event will be handled.
Ignoring the finer details of event pileup scenarios, the conceptual operation of ratpac-two looks like this:

```
Configuration of ratpac parameters (RATDB, etc.)

Producer 1:
Event 1 -> Processor 1 -> Processor 2 -> ... -> Processor N
Event 2 -> Processor 1 -> Processor 2 -> ... -> Processor N
...
Event M ->  Processor 1 -> Processor 2 -> ... -> Processor N

Producer 2:
Event 1 -> Processor 1 -> Processor 2 -> ... -> Processor N
...
```

First, we will briefly review ratpac-two commands and their syntax.
Then we will work through an example synthesizing multiple commands into a macro file. 
A comprehensive understanding requires extensive knowledge of the software.
Revisiting this guide is recommended as you read through the rest of the documentation.

```{contents} Table of Contents
:depth: 2
```



## 1. ratpac-two Commands and Workflow

After successfully installing ratpac-two and loading the associated environment, ratpac-two is accessed via a command line interface by running the `rat` command.
`rat` can execute a sequence of macro files if multiple are provided on the command line.

```bash
rat example.mac
```

This will execute `example.mac`

In addition to macro files, ratpac-two can be configured with various flags, which can be accessed by running `rat` with the `--help` flag

```
rat --help

options:
 -b, --database          URL to database
 -d, --debug             Enable debug printing
 -i, --input             Set default input filename
 -l, --log               Set log filename
 -o, --output            Set default output filename
 -p, --python            Set python processors
 -q, --quiet             Quiet mode, only show warnings
 -r, --run               Simulated run number
 -s, --seed              Set random number seed
 -x, --vector            Set default vector filename
 -v, --verbose           Enable verbose printing
 -V, --version           Show program version and exit
 -g, --vis               Load G4UI visualization
```

We will start our discussion of controlling ratpac-two by inspecting the structure of a single command.


### 1.1 Command Syntax
ratpac-two inherits its command interface from Geant4‘s G4RunManager.
[G4 command reference](https://github.com/natl/language-geant4-macro/blob/master/G4command.txt) 

The syntax rules are straightforward:
 * **Commands:** A command consists of a command name followed by zero or more parameters, separated by spaces. For example:
 
   `/rat/procset update 5`
   
	  Here, `/rat/procset` is the command, and `update` and `5` are parameters.

 * **Comments:** Lines beginning with a hash symbol `#` are treated as comments and are ignored by the interpreter. 
      They are essential for documenting macros.  
      Comments can also be used inline with commands, e.g.

      `/run/beamOn 10 #Run 10 events`
   
 * **Blank Lines:** Blank lines are also ignored and can be used to improve macro readability.
* **No Leading Whitespace:** Commands must start at column 1.
       Any spaces or tabs before the command cause an "unknown command" error. For example:

```
# Note the additional whitespace below
    /run/beamOn 1
```

Running a macro with this indentation will produce an "unknown command" error.


### 1.2. Command Hierarchy

Commands in ratpac-two (and Geant4) are organized into a hierarchical structure, much like a filesystem. This structure helps in organizing commands logically by function. For example:
 * `/run/` commands control aspects of Geant4 simulation runs (e.g., `/run/initialize`, `/run/beamOn`).
 * `/generator/` commands control generation of particle sources and their properties (e.g., `/generator/add`, `/generator/vtx/set`).
 * `/rat/` is the base directory for all commands specific to ratpac-two functionalities. Within `/rat/`, further subdirectories exist, such as:
   * `/rat/db/` for RAT Database interactions.
   * `/rat/proc/` (or `/rat/procset/`) for managing data processing modules (processors).


### 1.3. Command Execution Order

ratpac-two will execute the commands in a macro file sequentially from top to bottom.
The exact ordering of commands will depend on the specific task to be accomplished by ratpac-two.
In the next section, we will provide an example macro outlining a workflow suitable for detector simulations to illustrate usage.

Macro files can include other macro files using the `/control/execute` command. For example, in your macro file you can add:
```
/control/execute common_detector_setup.mac
```
When this command is encountered, the specified macro file (common_detector_setup.mac in this case) is immediately read and its commands are executed in place before continuing with the original macro.
This feature is extremely useful for modularizing simulation configurations.
Nested macros are allowed. A file invoked with `/control/execute` can itself contain additional `/control/execute` commands, and each included file is processed immediately when encountered.
If the file cannot be found, Geant4 prints an error and stops executing the current macro.

Users can create libraries of common settings, such as standard detector geometry definitions or output format configurations.
Specific simulation scenarios can then be composed by including these base macros and then overriding or adding specific parameters as needed.
This approach promotes reusability, consistency across different studies, and better organization of complex simulation setups, mirroring how functions or modules are used in programming to avoid code duplication and improve maintainability.

A minimal macro might contain just a few commands that cannot run independently without other setup commands:

```
# mini.mac
/rat/proc noise
/rat/proc splitevdaq
```


### 1.4 Example Macro

Now let's take a look at an example macro file.
This macro uses the internal simulation tools in ratpac-two to produce 2 MeV electron interactions in the example geometry.
It instantiates a series of processors that add Poisson dark noise to photosensors, simulate readout electronics, and save the processed output in the `outntuple` format.
This example serves as a template for how one might structure a simulation macro:

The macro is broken down into:

 1. ratpac configuration (Physics configurations & RATDB definitions)
 2. `/run/initialize`
 3. Logical ordering of processors 
 4. Generator definitions

**NOTE** Processors are specified **before** generators.

**Example macro:**
```
#-------------------------------------------------------------------------------
# Informational Header
#-------------------------------------------------------------------------------
# example.mac - An example macro file for this tutorial
# ratpac-two Tutorial
# Date (YYYY-MM-DD): 2025-06-01 

#-------------------------------------------------------------------------------
# Physics Settings
#-------------------------------------------------------------------------------

# Example commands that allow us to select what physics processes we would like
# to simulate.  For example, 2 MeV electrons do not require muon and hadronic 
# physics processes

/glg4debug/glg4param omit_muon_processes  1.0
/glg4debug/glg4param omit_hadronic_processes  1.0

# We disable particle tracking in this example to save on RAM during runtime.
/tracking/verbose 0 

#-------------------------------------------------------------------------------
# RATDB Configuration
#-------------------------------------------------------------------------------
# Set your experiment (explained later in the guide)
/rat/db/set DETECTOR experiment "Validation"
# For this example we will use the ratpac validation geometry
/rat/db/set DETECTOR geo_file "Validation/Valid.geo"

#-------------------------------------------------------------------------------
# Initialize the Simulation
#-------------------------------------------------------------------------------

#This command 'locks in' the geometry and parameters above for the simulation
/run/initialize

#-------------------------------------------------------------------------------
# Add Event Processors
#-------------------------------------------------------------------------------

# Next we add our processors.
# Simulate Poisson dark noise in the PMTs
/rat/proc noise
# Simulate the DAQ and waveform digitization with trigger.
/rat/proc splitevdaq
/rat/procset trigger_threshold 1.0
# Print the event count every 10 events.
/rat/proc count
/rat/procset update 10
# Save the results in a root file using outntuple
/rat/proc outntuple

#-------------------------------------------------------------------------------
# Add Event Generators
#-------------------------------------------------------------------------------

#Generators are specified after processors; here we use a basic particle gun
/generator/add combo gun:point:Poisson
# 2.0 MeV electrons at the center of the detector along the negative z axis
/generator/vtx/set e- 0.0 0.0 -1.0 2.0
/generator/pos/set 0.0 0.0 0.0

##### RUN ###########
# Run the event generator 100 times
/run/beamOn 100
# End of macro
```

Let's discuss the format and styling of this macro by providing additional detail on some of the most important commands above



## 2. Essential Geant4 Commands for ratpac-two Simulation

Since ratpac-two is built upon the Geant4 toolkit, a wide array of standard Geant4 User Interface (UI) commands are available and often essential for controlling simulations.
For users new to Geant4, understanding these core commands is a prerequisite for effectively using ratpac-two.
This section focuses on the most commonly used Geant4 commands relevant in the context of a ratpac-two simulation.


### 2.1. Run Control

These commands manage the initialization and execution of simulation runs and dictate the order of commands passed to ratpac-two in a macro.

* `/run/initialize`: This is one of the most critical commands in any Geant4-based simulation. 
      It must be issued before the first run can start. 
      This command triggers the construction of the detector geometry, calculation of physics tables, and preparation of user actions. 
      In ratpac-two, it ensures that all detector parameters from RATDB are processed and the simulation world is built.

   The typical placement of this command is after setting up detector parameters (e.g., via `/rat/db/` commands) but before defining event processors or starting the event loop.

   **Note:** The run manager must be initialized *after* the geometry has been fully configured and *before* issuing any `/run/beamOn` command.
   Calling `/run/beamOn` prior to `/run/initialize` will cause Geant4 to halt with an error similar to `G4RunManager::BeamOn() called before initialization`.

 * `/run/beamOn <numberOfEvents>`: This command starts a simulation run, processing the specified number of events.

   Multiple `/run/beamOn` commands can appear in a single macro, for instance, to simulate different particle types or energies sequentially after reconfiguring the event generator.


### 2.2. Verbosity Control

Geant4 provides extensive control over the amount of information printed during a simulation.
These commands are invaluable for debugging and understanding the simulation process.
Verbosity is typically controlled by an integer level, where 0 means minimal output and higher values provide more detail.

 * `/run/verbose <level>`: Controls the verbosity of the run manager (e.g., messages about run initialization and termination).
 * `/event/verbose <level>`: Controls verbosity related to event processing (e.g., information at the beginning and end of each event).
 * `/tracking/verbose <level>`: Provides detailed information about particle tracking, including step-by-step details of particle interactions and movement through the geometry.
      This is particularly useful for debugging physics processes or identifying issues with the detector geometry.
  
By strategically increasing verbosity for specific components like tracking or particular physics processes, new users can gain a much clearer picture of what Geant4 (and by extension ratpac-two) is doing "under the hood" for each event. 
Observing this textual output can be more illuminating for learning than just examining final results or visualizations, as it connects abstract concepts of particle interactions and geometry definitions to concrete simulation steps.


### 2.3. Detector Visualization

Basic visualization commands let you quickly open a viewer and inspect the geometry.
Note that to successfully run the visualizer, you must start rat with the `--vis` flag, e.g. `rat visualization.mac --vis`.
Without this option rat may exit with an error when the visualization commands are executed.
Useful commands include:

* `/vis/open OGL` - open an OpenGL viewer.
* `/vis/drawVolume` - draw the full detector geometry.
* `/vis/viewer/update` - refresh the current viewer after changes.
* `/glg4vis/reset` - reset the viewpoint to default settings.
* `/glg4vis/upvector x y z` - choose the "up" direction for the viewer.


### 2.4. Other Useful UI Commands

 * `/control/execute <macroFile>`: As mentioned in Section 1.3, this command executes another macro file. 
      It is a standard Geant4 command.
 * `/control/loop <macroFile> <counterName> <initialValue> <finalValue> <stepSize>`: This command allows for looping.
      The specified <macroFile> is executed multiple times. In each iteration, the Geant4 UI variable <counterName> is set to values from <initialValue> to <finalValue> with an increment of <stepSize>.
      The <counterName> can then be used within <macroFile> (e.g., `{counterName}`).
 * `/control/foreach <macroFile> <variableName> <valueList>`: Similar to `/control/loop`, but iterates over a discrete list of values provided in <valueList> (space-separated). 
      The <variableName> is set to each value in the list for each execution of <macroFile>.
      The availability of Geant4's looping commands (`/control/loop`, `/control/foreach`) directly within the macro system allows users to perform simple parameter scans (e.g., varying particle energy, source position, or even a RATDB parameter if set within the looped macro) without resorting to external scripting.
      This is a powerful built-in feature for conducting systematic studies efficiently.
 * `exit`: Terminates an interactive ratpac-two session.

The following table summarizes some of the most common Geant4 UI commands useful for ratpac-two users.

**Table 2.1:** Common Geant4 UI Commands for ratpac-two

| Command             | Typical Parameters | Description                                                            | Example Usage in ratpac-two Context  |
|---------------------|--------------------|------------------------------------------------------------------------|--------------------------------------|
| `/run/initialize`   | (none)             | Initializes detector geometry, physics lists, and run conditions.      | `/run/initialize`                    |
| `/run/beamOn`       | `<numberOfEvents>` | Starts a simulation run for the specified number of events.            | `/run/beamOn 1000`                   |
| `/run/verbose`      | `<level>`          | Sets the verbosity level for run-related messages (0 = quiet).         | `/run/verbose 1`                     |
| `/event/verbose`    | `<level>`          | Sets the verbosity level for event-related messages.                   | `/event/verbose 1`                   |
| `/tracking/verbose` | `<level>`          | Sets the verbosity level for particle-tracking messages.               | `/tracking/verbose 1`                |
| `/control/execute`  | `<filename>`       | Executes commands from the specified macro file.                       | `/control/execute detector_setup.mac`|
| `exit`              | (none)             | Exits an interactive ratpac-two session.                               | `exit`                               |

This set of commands provides a foundational toolkit for basic simulation control within ratpac-two.



### 3. Navigating ratpac-two Specific Commands: The /rat/ Directory

While ratpac-two leverages the standard Geant4 command interface, it also introduces a suite of custom commands to manage its unique features and functionalities.
These ratpac-two-specific commands are neatly organized under the `/rat/` command directory.
This clear separation into the `/rat/` namespace is a deliberate design choice that prevents potential conflicts with standard Geant4 commands or commands from other Geant4-based applications.
Such organization makes the command space more predictable and manageable for users and developers alike.

Commands within the `/rat/` directory provide fine-grained control over various components of the ratpac-two framework, including its parameter database (RATDB), the sequence of data processing modules (processors), specialized event generators, custom physics configurations, and I/O settings.
The internal structure of subdirectories within `/rat/` (e.g., `/rat/db`, `/rat/proc`) often mirrors the modular C++ class structure within the ratpac-two source code.

For example, commands found in `/rat/db/` are typically implemented by messenger classes associated with RAT::DB or similar database management classes in the C++ backend.
An awareness of this correlation can be beneficial for advanced users or those delving into the source code to understand command implementations.
Developers extending the command interface typically create a messenger class whose name mirrors the subdirectory.

Following this pattern, a new `/rat/foo/` directory would pair with `FooMessenger` in `src/cmd/src/FooMessenger.cc`, keeping the command layout synchronized with the source tree.
The following table gives a high-level overview of the common subdirectories expected within `/rat/` and their primary functions.
The exact commands and their detailed functionalities are determined by analyzing the ratpac-two source code.
Subsequent sections of this guide will elaborate on commands within these key areas.

**Table 3.1:** Overview of Key /rat/ Command Subdirectories

| Subdirectory | Primary Function | Key Commands | Example Usage |
| :-------------- | :---------------------------------------------------------------------------------------------------------- | :------------------------------------------------------------- | :----------------------------------------- |
| `/rat/db/`      | Manages interaction with the RAT Database (RATDB), including loading and modifying detector parameters.     | `/rat/db/load`, `/rat/db/set`, `/rat/db/server`, `/rat/db/run` | `/rat/db/load myfile.ratdb` |
| `/rat/proc/`    | Controls the chain of ratpac-two processors (data processing modules) and their configurations.             | `/rat/proc`, `/rat/procset`                                    | `/rat/proc MyProcessor`          |
| `/rat/physics`  | Configures ratpac-two specific physics options, custom physics lists, or specialized physics processes.     | `/rat/physics/add`, `/rat/physics/select`                      | `/rat/physics/enableCerenkov true` |
| (others)        | Other specialized control directories as defined within the ratpac-two framework for specific components.   | (dependent on ratpac-two features)                             | (varies) |

The `/rat/db/load` command is implemented by `DBMessenger` in `src/cmd/src/DBMessenger.cc`.

Understanding this top-level structure helps users navigate to the relevant command set for the aspect of the simulation they wish to control.

For instance, to modify a material property, one would look for commands under `/rat/db/`.
To add a new analysis step to the event processing, commands under `/rat/proc/` would be relevant.



## 4. Configuring the ratpac-two Environment with RATDB via Macros

The RAT Database (RATDB) is a cornerstone of ratpac-two's flexibility, serving as a centralized repository for all parameters that define the simulated experiment using JSON-like structure.
This includes detailed descriptions of detector geometry, material properties, optical parameters (like refractive indices, absorption lengths, Rayleigh scattering lengths), PMT characteristics (quantum efficiency, transit time spread), electronics and DAQ settings, and even fundamental physics constants.
The ability for analysis code to trivially access the same detector geometry and physics parameters used in the detailed simulation is a key design feature facilitated by RATDB.
This comprehensive parameterization is crucial for achieving "As Microphysical as Reasonably Achievable" (AMARA) simulations.

RATDB parameters are typically stored in external files, often using a like JSON format, which is both human-readable and easily parsable.
The choice of JSON for RATDB files offers significant advantages: users can inspect or manually tweak parameters using standard text editors, and the structured nature of JSON (key-value pairs, arrays, nested objects) is well-suited for representing complex detector configurations.
Furthermore, many programming languages have built-in JSON parsers, making it easier to integrate ratpac-two configurations with external scripts or analysis tools if necessary.
Macros provide the interface to load these database files and, importantly, to modify specific parameters at runtime before the simulation initializes.


### 4.1. Key RATDB Macro Commands

The following commands, primarily under the `/rat/db/` path, are used to interact with RATDB.
The exact syntax and full range of commands can be found by inspecting the RAT::DB related messenger classes in the ratpac-two source code.

 * `/rat/db/load <filename>` :
     This command loads data from the specified file into RATDB. 
     Load material optical properties, e.g., `/rat/db/load OPTICS.ratdb`
* `/rat/db/set <TABLE>[<index>] <field> <json_value>`:
    Modify a value in the specified RATDB table. `<TABLE>` may include an index such as
    `name:AV` to select a particular entry.  The last argument is interpreted as JSON
    and converted to the appropriate data type automatically.  For example
    `/rat/db/set MATERIALS name:LS LIGHT_YIELD 10000.0`
    `/rat/db/set PMTPROPS pmt_model:R12345 QE [0.20,0.22,0.25,0.22,0.20]`
    The ability to modify RATDB parameters at runtime via `/rat/db/set` is extremely powerful for systematic studies.
     Users can employ macro control flow (like `/control/loop`) to iterate over different material properties, PMT efficiencies, or minor geometry variations without needing to create and manage numerous distinct RATDB files.
     This greatly streamlines the process of studying the impact of detector parameter uncertainties.
* `/rat/db/server <url>`:
  Connects to a remote CouchDB instance holding RATDB tables.
* `/rat/db/run <run_number>`:
  Sets the default run number for database lookups.


### 4.2. Timing of RATDB Commands

It is crucial to understand that RATDB parameters defining the detector geometry, fundamental material properties, or physics constants generally need to be set before the `/run/initialize` command is issued. 
The `/run/initialize` step uses the information in RATDB to construct the simulation world. 
Changes made to these fundamental parameters after initialization may not take effect or could lead to inconsistent states.
The interaction between macros and RATDB highlights ratpac-two's flexibility. 
While default or baseline parameters are loaded from files (which might be shared and version-controlled within a collaboration), macros provide a dynamic overlay. 
This allows users to make temporary changes or specific adjustments for a given simulation run without altering the underlying database files, which is particularly beneficial in collaborative environments.

**Table 4.1:** Essential RATDB Macro Commands

| Task                                     | Command Syntax (Conceptual)                                             | Example                                                                |
|:-----------------------------------------|:------------------------------------------------------------------------|:-----------------------------------------------------------------------|
| Load a RATDB file                        | `/rat/db/load <filename.ratdb>`                                         | `/rat/db/load special_definitions.ratdb`                               |
| Set a scalar parameter in a table        | `/rat/db/set <TABLE>[<idx>] <param> <json_value>`                       | `/rat/db/set MATERIALS name:Acrylic DENSITY 1.19`                      |
| Set an array parameter in a table        | `/rat/db/set <TABLE>[<idx>] <param> <json_array>`                       | `/rat/db/set OPTICS name:Water RAYLEIGH_LENGTH [30000.,35000.,40000.]` |
| Connect to CouchDB server                | `/rat/db/server <url>`                                                  | `/rat/db/server http://localhost:5984`                                 |
| Set default run number                   | `/rat/db/run <run_number>`                                              | `/rat/db/run 42`                                                       |



## 5. Orchestrating the Workflow: Processor Commands

In ratpac-two, the concept of "processors" is central to defining the simulation and analysis workflow for each event. 
Processors are modular units of C++ code, each designed to perform a specific task. 
The sequence of these processors, assembled by the user in a macro file, dictates the entire chain of operations applied to every event generated. 
This user-defined chain effectively forms a data processing pipeline.

Examples of tasks handled by processors include:
 * Collecting and processing PMT hits.
 * Simulating electronics response and digitization.
 * Simulating trigger logic.
 * Performing event reconstruction (e.g., vertex fitting, energy estimation).
 * Writing output data to files (e.g., ROOT ntuples).

The order in which processors are added to the chain is critical, as they typically operate on the output of preceding processors. 
For instance, PMT digitization must occur after optical photons have been tracked to the PMTs, and event reconstruction generally requires digitized data.


### 5.1. Key Processor Macro Commands

Processors are controlled with a small set of macro commands:
* `/rat/proc <ProcessorName>` adds the named processor to the processing chain. The order of these commands determines the execution order. Use `/rat/proclast <ProcessorName>` to place a processor after any that were specified on the command line.
* `/rat/procset <parameter> <value>` sets a parameter on the most recently added processor. `ProcBlockManager` only allows `procset` after a successful `proc` command.
    For example, `/rat/proc noise` followed by `/rat/procset update 100` configures the `noise` processor to report every 100 events.

It is important to note that ratpac-two is designed to be extensible, allowing users and collaborations to develop their own custom processors in C++ to address unique experimental requirements or implement new analysis algorithms.
While the development of such custom processors is beyond the scope of the user guide, the macro commands for adding and configuring processors are designed to work uniformly with both standard and custom-developed modules. 
This extensibility means the list of available processors can grow beyond the set shipped with the core ratpac-two distribution.


### 5.2. Typical Processor Chain Example

A conceptual processor chain for a typical simulation might look like this:
 * mc_particle_tracking: (Assumed processor for Geant4 tracking of primary and secondary particles).
 * pmt_response: Simulates PMT hit generation from optical photons.
 * daq_electronics: Simulates front-end electronics, digitization, and waveform generation.
 * trigger: Applies a trigger logic to the digitized data.
 * reconstructor_bonsai: Performs event reconstruction using an algorithm like BONSAI.
 * output_ntuple: Writes selected event information (raw data, reconstructed quantities) to an output file.

A macro would typically contain a series of `/rat/proc` and `/rat/procset` commands to build this chain.


### 5.3. Table of Processor Control Commands and Common Processors

The following table outlines the essential commands for processor control and lists some common types of processors one might expect in ratpac-two.
These commands are implemented by `ProcBlockManager` in the source tree.

**Table 5.1:** Essential Processor Control Commands

| Command / Item   | Purpose / Description                                       | Example                 |
|:-----------------|:------------------------------------------------------------|:------------------------|
| `/rat/proc`      | Adds a processor to the event processing chain.             | `/rat/proc noise`       |
| `/rat/proclast`  | Adds a processor after all other processors specified.      | `/rat/proclast outroot` |
| `/rat/procset`   | Sets a parameter for the most recently added processor.     | `/rat/procset update 5` |

This structure allows users to build customized simulation and analysis workflows by selecting and configuring the appropriate sequence of processors.



## 6. Defining Event Sources: The Generator Commands

Event generators in ratpac-two are responsible for defining the initial state of particles that the simulation will track. 
This includes their type, energy, momentum, starting position, and time of creation. 
ratpac-two provides a flexible system for event generation, controlled by commands primarily under the `/generator/` path.
The available commands are defined in the source tree (e.g. `GLG4PrimaryGeneratorMessenger` and the various generator classes).
The examples below are verified directly against those implementations.


### 6.1. Key Generator Macro Commands

 * `/generator/add <generator_type> [options_string]`:
   This is the primary command for activating a new event generator instance.
   * <generator_type>: A string identifying the kind of generator to add. Examples might include:
     * gun: For a simple particle gun.
     * combo: A versatile generator that allows piecing together separate vertex, position, and time generators.
     * isotopes: For simulating radioactive decays from specific isotopes or decay chains.
     * supernova: For simulating bursts of neutrinos from a supernova.
     * hepevt or lhe: For reading particle events from external files in standard formats.
     * (The actual list of available types must be determined from the ratpac-two source code.)
   * [options_string]: An optional string passed directly to the generator for its internal configuration. For the combo generator, this string might specify the sub-generators to use, e.g., gun:point:poisson (particle gun kinematics, point-like position, Poisson time distribution).
   Example:
   Add a combo generator using a particle gun, point source, and Poisson time distribution
   /generator/add combo gun:point:poisson
   The combo generator, as suggested by the example, indicates a modular and highly flexible approach to defining event characteristics. Users can effectively mix and match different components (e.g., particle type/energy from a "gun2" model, position from a "point" or "volume" model, time distribution from a "poisson" or "fixed_interval" model) to construct complex event profiles simply by changing parts of the options_string or subsequent configuration commands.
 * `/generator/vtx/set <particle_name_or_PDGcode> [energy_value][dx][dy][dz]`:
* `/generator/vtx/set <args>`:
  Set the vertex generator state for the most recently added generator.  Argument
  details depend on the generator type.  For the built‑in `gun` vertex generator,
  the format is
  `/generator/vtx/set pname momx_MeV momy_MeV momz_MeV KE_MeV [polx poly polz mult]`.
  An isotropic 2 MeV electron can be produced with `/generator/vtx/set e- 0 0 0 2.0`.
* `/generator/pos/set <x> <y> <z>` or `/generator/pos/set <position_generator_name>`:
  Sets the position for event generation. For the built-in `point` position generator the
  arguments are three numbers interpreted in millimetres.
* `/generator/rate/set <rate>`:
  Sets the event rate for the current generator in events per second.  This string is passed
  directly to the time generator (e.g. `GLG4TimeGen_Uniform::SetState`).

Example sequence for generating isotropic 2 MeV positrons at the detector center, note that setting a generator to 0 0 0 defaults to isotropic emission:
`/generator/add combo gun:point:poisson`
`/generator/vtx/set e+ 0 0 0 2.0`
`/generator/pos/set 0 0 0`
`/generator/rate/set 1.0`
Commands like `/generator/vtx/set`, `/generator/pos/set`, and `/generator/rate/set` apply to the generator most recently added with `/generator/add`.
This contextual application is a common pattern in such command interfaces.


### 6.2. Multiple Generators

ratpac-two supports the use of multiple generators simultaneously. 
Events from different active generators will be interleaved according to their specified rates, and can even pile up if they occur in coincidence (i.e., within the same event window). 
This capability is essential for simulating realistic experimental conditions where multiple physics processes or background sources contribute to the collected data. 
It allows for comprehensive studies of signal-to-background ratios, pile-up effects, and coincidence detection schemes.


### 6.3. Table of Common Generator Configurations

Given the identified gap in existing documentation for generator commands, the following table provides conceptual examples of how common event sources might be configured. The precise command names and parameters must be validated against the ratpac-two source code.

**Table 6.1:** Common Event Generator Configurations via Macros (Conceptual)

| Task/Generator Type | Key `/generator/add` Command | Subsequent Configuration Commands | Example Macro Snippet | Brief Explanation |
| :------------------ | :--------------------------- | :-------------------------------- | :-------------------- | :---------------- |
| Single Particle Gun | `/generator/add gun` | `/generator/vtx/set <pname> px_MeV py_MeV pz_MeV KE_MeV` <br> `/generator/pos/set <x> <y> <z>` <br> `/generator/rate/set <R>` | `/generator/add gun` <br> `/generator/vtx/set e- 0 0 1 5.0` <br> `/generator/pos/set 0 0 0` | Fires a 5 MeV electron from the origin along +z. |
| Volumetric Isotope Decay | `/generator/add isotope <isotope_name>:<volume_name>` | `/generator/rate/set <R>` | `/generator/add isotope Bi214:ScintillatorVolume` <br> `/generator/rate/set 10` | Simulates Bi214 decays uniformly within the volume. |
| Combo Generator (Point Source) | `/generator/add combo gun:point:poisson` | `/generator/vtx/set <pname> px_MeV py_MeV pz_MeV KE_MeV` <br> `/generator/pos/set <x> <y> <z>` <br> `/generator/rate/set <R>` | `/generator/add combo gun:point:poisson` <br> `/generator/vtx/set e- 0 0 0 2.0` <br> `/generator/pos/set 0 0 500` <br> `/generator/rate/set 1` | Generates isotropic 2 MeV electrons at z=500 mm with Poisson timing. |
| Read from HEPEVT file | `/generator/add hepevt <file.hepevt>` | `/generator/rate/set <R>` (optional) | `/generator/add hepevt myevents.dat` | Reads particle events sequentially from the file. |

These examples illustrate the intended flexibility. Users should consult any example macros provided with a specific generator for validated syntax.



## 7. Conclusion and Further Learning

Macro files are the cornerstone of user interaction with ratpac-two, offering a powerful and flexible plain-text interface to control nearly every aspect of the simulation environment. From defining the fundamental detector parameters via RATDB interactions to specifying the types of physics events to generate and orchestrating the complex chain of data processing steps, macros empower users to tailor ratpac-two to their specific research needs.

Given that ratpac-two documentation is an evolving landscape, users are encouraged to explore the following resources:
 * This Guide: Keep this document as a reference for macro commands and concepts.
 * ratpac-two GitHub Repository: The primary source for the code, and potentially updated documentation or examples within its /doc directory or elsewhere. The source code itself, particularly the Messenger classes, is the definitive reference for command definitions.

Experimentation is key. Start with simple macros, make incremental changes, and use verbosity commands extensively to observe the effects of your commands. As familiarity grows, so will the ability to harness the full potential of ratpac-two for advanced particle physics simulations.
