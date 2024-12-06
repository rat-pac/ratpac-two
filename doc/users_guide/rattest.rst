Creating and Running `rattest` Tests
------------------------------------

Introduction
````````````

Rattest is a framework for creating unit and functional tests for RAT. These tests should be simple, testing only one aspect of the simulation. For instance, a test of attenuation in acrylic consist of a single light source in a world volume of acrylic -- no PMTs or other geometry. 

At minimum, a test consists of a RAT macro and a ROOT macro -- the Monte Carlo and the analysis. New (simplified) geometries, modified RATDB databases, etc. can also be included. When run, these tests are compared to a standard via a KS test, and a web page is created with histograms (standard and current) and KS test results. The standard RAT logs and output ROOT file is also available for analysis.

The existing rattests are included with the standard RAT distribution, in `$RATSHARE/test/`, with the functional tests in `$RATSHARE/test/full/<test-name>`. To run a single test, `cd` to the test directory and simply run `python3 $RATSHARE/python/rattest.py <test-name>` where `<test-name>` corresponds to a folder in `$RATSHARE/test/full`. Rattest will iterate through the directory structure to find the test, run the RAT macro, run the ROOT macro on the output, and generate a report page.

The `rattest.py` script takes the following options::

    usage: rattest.py [-h] [-u] [-m] [-r] [-t] [--make-template TEMPLATE] input [input ...]

    positional arguments:
      input                 RAT test(s) to run. Must be a directory or directories.

    options:
      -h, --help            show this help message and exit
      -u, --update          Update "standard" histogram with current results.
      -m, --regen-mc        Force Monte Carlo to be regenerated.
      -r, --regen-plots     Force histograms to be regenerated.
      -t, --text-only       Do not open web pages with plots.
      --make-template TEMPLATE
                            Write a template rattest to current directory for you to edit. Supplied name is used for .mac and .C files.

Existing RAT Tests
``````````````````

::

    acrylic_attenuation
     Tests the attenuation length of acrylic by generating photons in an acrylic block and checking track lengths

Automated rattests
``````````````````

Every time a PR is submitted to ratpac-two, the rattests in $RATROOT/test/full (FIXME currently just acrylic_attentuation) are ran via GitHub actions using a Github hosted runner. PRs that do not pass all of the rattests will not be merged in, unless the reasons for the test failures are intended. If a test begins to fail because of an intended change, the `standard.root` file should be updated.

The workflow file that controls the running of the rattests can be found at $RATSHARE/.github/workflows/rattests.yml. To add a test, one must add another job to the workflow file using the same format used by the other rattests.

Downstream forks and experiment repositories are encouraged to create their own rattests using the same workflow setup and the `rattest.py` script.

Writing a RAT Test
``````````````````

1. Create a new folder in `$RATSHARE/test/full` with useful but short name for your test
2. Create a `rattest.config` file, like this::

    #!python
    # -*- python-*-
    description = '''Tests the attenuation length of acrylic by generating photons in an acrylic block and checking track lengths'''
  
    rat_macro = 'acrylic_attenuation.mac'
    root_macro = 'acrylic_attenuation.C'

The RAT macro and ROOT macro do not need to have the same name as the test, they just have to be consistent with the actual filenames. `rattest` will find your ROOT output file, so you don't have to worry about it.

3. If necessary, create a RAT geometry (.geo) and any modified RATDB (.ratdb). As an example, `acrylic_attenuation` uses the default RATDBs (the default behavior), but the following geometry::

    // -------- GEO[world]
    {
    name: "GEO",
    index: "world",
    valid_begin: [0, 0],
    valid_end: [0, 0],
    mother: "",
    type: "box",
    size: [10000.0, 10000.0, 10000.0],
    material: "acrylic_polycast",
    }

RAT will prefer a geometry or database in your test directory, and defaults to the ones in `$RATSHARE/ratdb`.

4. Create your RAT macro.

Keep things as simple as possible, and turn off as many options as possible. The `acrylic_attenuation` RAT macro::

    /glg4debug/glg4param omit_muon_processes  1.0
    /glg4debug/glg4param omit_hadronic_processes  1.0
    
    /rat/db/set DETECTOR geo_file "acrylic_sphere.geo"
    
    /run/initialize
    
    # BEGIN EVENT LOOP
    /rat/proc count
    /rat/procset update 50
    
    /rat/proc outroot
    /rat/procset file "acrylic_attenuation.root"
    
    # END EVENT LOOP
    /tracking/storeTrajectory 1
    
    /generator/add combo pbomb:point
    /generator/vtx/set 100 100
    /generator/pos/set  0.0 0.0 0.0 
    
    
    /generator/add combo pbomb:point
    /generator/vtx/set 100 200
    /generator/pos/set  0.0 0.0 0.0
    
    ...
    
    /run/beamOn 500

5. Write a ROOT macro

The ROOT macro should create a histogram that captures the benchmark you are looking for. It should consist of a single `void` function with the same name as the macro ie `acrylic_attentuation(std::string event_file, std::string outfile)`. `rattest` will automatically fill in the function arguments when it calls the root macro.

Basically, do your analysis, make a histogram, and output it with `[histogram name]->Write()`. Note that when using `Draw()` to make histograms, you'll probably want the `"goff"` option.

`rattest` will pull histogram names from this macro automatically for creation of the results page.

The ROOT macro from `acrylic_attenuation`::

    void acrylic_attenuation(std::string event_filename, std::string out_filename)
    {
      TFile *event_file = new TFile(event_filename.c_str(),"READ");
      TTree *T = (TTree*)event_file->Get("T");
      TFile *out_file = new TFile(out_filename.c_str(),"RECREATE");

      TH1F *acr_attn_100 = new TH1F("acr_attn_100", "Photon track length (100 nm)", 50, 0, 50);
      acr_attn_100->SetXTitle("Track length (mm)");
      acr_attn_100->SetYTitle("Count");
      T->Draw("mc.track.GetLastMCTrackStep()->length>>acr_attn_100","TMath::Abs(1.23997279736421566e-03/(mc.track.GetLastMCTrackStep()->ke)-100)<10","goff");
      //acr_attn_100->Fit("expo");
      //acr_attn_100->Draw("goff");
      acr_attn_100->Write();

      ...
    
    }

6. Test it

 Run your RAT macro with the usual `rat [macro name]`, then, in ROOT, run the contents of your analysis macro and ensure that you get what you were looking for.

7. Create a standard

 From the test directory, run `python3 rattest.py -u [your test name]`. This will create the file `standard.root`, which will be the basis for comparison until the next time you run `rattest` with the `-u` option. Take a look at `results.html` to see how things worked out.

This is pretty much it. If you run `python3 rattest.py [your test name]` again, you should get a results page (which will open in your default browser unless you specified the `-t` option) with very similar results.

If you think the test is useful to others, commit it to the RAT repository with svn. Be sure to commit only the `rattest.config`, RAT and ROOT macro, any geometry or RATDB files, and `standard.root`.
