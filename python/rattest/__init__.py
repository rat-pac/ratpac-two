"""rattest __init__

Author: Andy Mastbaum
         <mastbaum@hep.upenn.edu>
Revision history:
        15-09-2015 F. Descamps: add handling of 'proclast' syntax
        13-10-2017 R. Lane: adding ability to fix seed and change KS threshold
        15-03-2018 R. Lane: changes needed for compatibility with ROOT 6
"""

from __future__ import print_function
from __future__ import absolute_import
from __future__ import division

import sys
import os.path
import shutil
import re
from math import sqrt

# ROOT changes into whatever directory it finds in the command line args
_argv = sys.argv
sys.argv = sys.argv[:1]
import ROOT
from ROOT import gROOT, gSystem, TFile, TH1, TCanvas, kRed, kBlue, kDashed

sys.argv = _argv

gROOT.SetBatch(True)
TH1.SetDefaultSumw2()
# Detect if we already loaded the dictionary, for example inside RAT already
if ROOT.TClassTable.GetID("RAT::DS::Root") == -1:
    gROOT.ProcessLine(".x " + os.path.join(os.environ['RATROOT'], "src/rootinit.C"))
from ROOT import RAT

def target_needs_update(target, sources):
    '''
    Determine if the target needs to be updated.
    Based on its timestamp and whether or not it already exists.
    '''
    if not os.path.exists(target):
        return True
    
    target_date = os.path.getmtime(target)

    for source in sources:
        if target_date < os.path.getmtime(source):
            return True

    return False

def find_outfile_name(filename):
    '''
    Find the name of the output file in a RAT macro.
    '''
    with open(filename, 'r', encoding="utf-8") as output_file:
        output_file_str = output_file.read()

    dsmatch = re.search(r'/rat/proc[last]*\s+outroot\s+/rat/procset\s+file\s+"(.+)"', output_file_str)
    socmatch = re.search(r'/rat/proc[last]*\s+outsoc\s+/rat/procset\s+file\s+"(.+)"', output_file_str)
    ntuplematch = re.search(r'/rat/proc[last]*\s+outntuple\s+/rat/procset\s+file\s+"(.+)"', output_file_str)

    if dsmatch:
        return dsmatch.group(1)
    
    if socmatch:
        return socmatch.group(1)
    
    if ntuplematch:
        return ntuplematch.group(1)
    
    return None

def dir_to_strlist(directory):
    '''
    From a ROOT directory, get all associated keys.
    '''
    return [obj.GetName() for obj in directory.GetListOfKeys()]

class RatTest:
    '''
    For setting up and running the different components of a RAT test.
    Runs the RAT simulation and the corresponding ROOT macro.
    Also compares and saves the histograms.
    '''

    def __init__(self, config_file):
        self.config_file = config_file
        self.testdir = os.path.abspath(os.path.dirname(config_file))
        # Create default name out of bottom level dir name
        self.name = os.path.basename(self.testdir)
        self.description = ''

        # Load testcase settings from file and store all attributes in this object
        params = {}
        # Set default of no fixing of random seed
        self.seed = -1
        # Set default of 1% threshold on KS tests
        self.KS_threshold = 0.01
        self.num_macros = 1
        exec(compile(open(self.config_file).read(), self.config_file, 'exec'), {}, params)
        for key, val in params.items():
            setattr(self, key, val)

        # Expand full paths
        self.rat_macro = os.path.abspath(os.path.join(self.testdir, self.rat_macro))
        self.root_macro = os.path.abspath(os.path.join(self.testdir, self.root_macro))
        self.rat_script = os.path.abspath(os.path.join(os.environ['RATROOT'], 'bin', 'rat'))
        self.rat_bin = os.path.abspath(os.path.join(os.environ['RATROOT'], 'bin', 'rat'))

        # Find name of file holding events from last macro
        mac = self.rat_macro
        suffix = ".mac"
        if self.num_macros > 1:
            suffix = "_" + str(self.num_macros-1) + ".mac"
        mac = self.rat_macro.replace(".mac", suffix)
        self.event_file = os.path.join(self.testdir, find_outfile_name(mac))
        print('Event file: {}'.format(self.event_file))

        # Generate names of standard and current output files
        self.standard_results = os.path.join(self.testdir, 'standard.root')
        self.current_results = os.path.join(self.testdir, 'current.root')

    def run(self, regen_mc=False, regen_plots=False, html=None):
        '''
        Run a full RAT test, including simulation and the ROOT macro.
        Includes writing results to the HTML file, if option is specified.
        '''
        print('=' * 5 + ' Run Test: {} '.format(self.name) + '=' * 5)
        self._do_run(regen_mc=regen_mc, regen_plots=regen_plots)

        if html:
            html.header(name=self.name, description=self.description)

        return self.compare_hists(self.standard_results, self.current_results, html=html)

    def update(self, regen_mc=False, regen_plots=False):
        '''
        Update the RAT tests.
        '''
        print('=' * 5 + ' Update Test: {} '.format(self.name) + '=' * 5)
        self._do_run(regen_mc=regen_mc, regen_plots=regen_plots)
        print('Copying current results to master file of standard results')
        shutil.copyfile(self.current_results, self.standard_results)

    def _do_run(self, regen_mc, regen_plots):
        '''
        Run the RAT simulation and the corresponding ROOT macro for processing the results.
        '''
        if regen_mc or target_needs_update(self.event_file, [self.rat_macro, self.rat_script, self.rat_bin]):
            self.run_rat()
        if regen_plots or target_needs_update(self.current_results, [self.event_file, self.root_macro]):
            self.run_root()

    def run_rat(self):
        '''
        Run the RAT simulation, possibly with a seed.
        '''
        #loop over macros to be run
        for i in range(0, self.num_macros):
            suffix = ".mac"
            if i > 0:
                suffix = "_" + str(i) + ".mac"
            mac = self.rat_macro.replace(".mac", suffix)
            if self.seed == -1:
                self.run_cmd_in_testdir('rat ' + os.path.basename(mac))
            else:
                self.run_cmd_in_testdir('rat -s ' + str(self.seed) + ' ' + os.path.basename(mac))

    def run_root(self):
        '''
        Run the ROOT macro to process the results of a RAT simulation.
        Expects that the ROOT file from the simulation has been generated.
        '''
        cwd = os.getcwd()
        try:
            os.chdir(self.testdir)
            gSystem.ChangeDirectory(self.testdir)

            root_file = TFile.Open(self.event_file)
            if not root_file or root_file.IsZombie():
                raise RuntimeError("File {} could not be opened. "
                                   "Please check the simulation logs to determine if "
                                   "there were any problems. "
                                   "Stopping the program here, "
                                   "not running the ROOT macro.".format(self.event_file))
            root_file.Close()

            if hasattr(self, 'output_type'): # New style compiled macro
                gROOT.ProcessLine('.x ./' + os.path.basename(self.root_macro) + '+("{}","{}")'.format(self.event_file, self.current_results))
            else: # Old style uncompiled macro used
                gROOT.ProcessLine('.x ./' + os.path.basename(self.root_macro) + '("{}","{}")'.format(self.event_file, self.current_results))

        finally:
            os.chdir(cwd)

    def run_cmd_in_testdir(self, cmd):
        '''
        Run any command in the test directory specified by the data attribute.
        '''
        cwd = os.getcwd()
        try:
            os.chdir(self.testdir)
            print('Running: {}'.format(cmd))
            os.system(cmd)

        finally:
            os.chdir(cwd)

    def compare_hists(self, master_fname, current_fname, html=None):
        '''
        Run the Kolmogorov-Smirnov (KS) test to compare histograms for consistency.
        '''
        master_file = TFile(master_fname)
        current_file = TFile(current_fname)

        master_objs = set(dir_to_strlist(master_file))
        current_objs = set(dir_to_strlist(current_file))
        m_only = master_objs.difference(current_objs)
        both = master_objs.intersection(current_objs)
        c_only = current_objs.difference(master_objs)

        if len(m_only) != 0:
            print('WARNING: master contains:', ', '.join(m_only), 'and current test does not.')
        if len(c_only) != 0:
            print('WARNING: current test contains:', ', '.join(c_only), 'and master does not.')

        if len(both) == 0:
            print("ERROR: current test file contains none of the histograms in master")
            overall_success = False

            if master_file:
                master_file.Close()
            if current_file:
                current_file.Close()

            return overall_success

        #failure rate set in config file (KS_threshold), defaults to 1% (independent of number of hists)
        #a perfect match for all distributions can be required by setting the KS_threshold equal to 1
        critical_probability = self.KS_threshold / len(both)
        overall_success = True

        for objname in both:
            c_obj = current_file.Get(objname)
            m_obj = master_file.Get(objname)

            prob = m_obj.KolmogorovTest(c_obj)
            if m_obj.GetEntries() == 0 and c_obj.GetEntries() == 0:
                prob = 1
            print("Comparing {}: {}".format(objname, c_obj.GetTitle()))

            #Require a perfect match if desired, else test against critical_probability
            if self.KS_threshold == 1.0:
                if prob == 1.0:
                    success = True
                else:
                    success = False
                    overall_success = False
            else:
                if prob > critical_probability:
                    success = True
                else:
                    success = False
                    overall_success = False

            success_message = "SUCCESS" if (success and overall_success) else "FAILURE"
            print("  {}: KS prob = {}".format(success_message, prob))

            plotfile = os.path.join(self.testdir, objname + '.png')
            self.draw_hist(plotfile, m_obj, c_obj)
            fit_parameters = self.get_fit(m_obj, c_obj)

            if html:
                html.hist(name=objname, description='', prob=prob,
                          success=success,
                          plotname=os.path.basename(plotfile),
                          parameters=fit_parameters)

        if master_file:
            master_file.Close()
        if current_file:
            current_file.Close()

        return overall_success

    def draw_hist(self, plotfile, master, current):
        '''
        Draw the histograms.
        '''
        c1 = TCanvas('c1', '', 600, 400)
        master.SetLineColor(kBlue)
        current.SetLineColor(kRed)
        current.SetLineStyle(kDashed)

        all_min = min(master.GetMinimum(), current.GetMinimum())
        all_max = max(master.GetMaximum(), current.GetMaximum())
        span = all_max - all_min
        master.SetMinimum(all_min - 0.1 * span)
        master.SetMaximum(all_max + 0.1 * span)

        master.Draw()
        current.Draw("SAME")

        if hasattr(self, 'plot_options'):
            if "logX" in self.plot_options and self.plot_options["logX"] and master.GetBinLowEdge(1) > 0.0:
                c1.SetLogx()

            if "logY" in self.plot_options and self.plot_options["logY"] and master.GetMinimum() > 0.0:
                c1.SetLogy()

        c1.Update()
        c1.Print(plotfile)
        c1.Close()

    def get_fit(self, master_hist, current_hist):
        '''
        Fit the functions.
        '''
        master_funclist = master_hist.GetListOfFunctions()
        current_funclist = current_hist.GetListOfFunctions()

        if not master_funclist.GetSize() or not current_funclist.GetSize():
            return None # Don't plot fit function parameters

        # Assume only 1 fit function
        master_func = master_funclist.At(0)
        current_func = current_funclist.At(0)

        # Make sure these are really functions
        if (master_func.Class().GetName() != 'TF1' or
                current_func.Class().GetName() != 'TF1'):
            return None

        npar = master_func.GetNpar()
        parameters = []
        for ipar in range(npar):
            parameter = {'name': master_func.GetParName(ipar),
                         'm_val': master_func.GetParameter(ipar),
                         'm_err': master_func.GetParError(ipar),
                         'c_val': current_func.GetParameter(ipar),
                         'c_err': current_func.GetParError(ipar)}
            parameter['delta'] = parameter['c_val'] - parameter['m_val']
            parameter['sigma'] = abs(parameter['delta']) / sqrt(parameter['m_err']**2
                                                                + parameter['c_err']**2)
            parameters.append(parameter)

        return parameters


class RatHtml:
    '''
    For generating an output HTML file from a RAT test.
    Stores information on the RAT test such as its name and description.
    Stores information on the resulting plots, including location and parameters.
    '''

    def __init__(self, htmlname):
        self.htmlname = htmlname
        self.plots = []
        self.name = ''
        self.description = ''

    def header(self, name, description):
        '''
        Assign the HTML file a name and description.
        '''
        self.name = name
        self.description = description

    def hist(self, name, description, prob, success, plotname, parameters):
        '''
        Append the properties and location of a plot to a list.
        All files in the list are to be written to an HTML file.
        '''
        p = {'name': name, 'description': description,
             'prob': prob, 'plotname': plotname,
             'parameters': parameters}

        if success:
            p['result'] = '<font color="green">Success</font>'
        else:
            p['result'] = '<font color="red">Failure</font>'

        self.plots.append(p)

    def write(self):
        '''
        Write the plots to an HTML file.
        '''
        self.plots.sort(key=lambda k: k['name'])

        with open(self.htmlname, 'w') as f:
            f.write('''
<html>
  <head>
    <title>RAT Test: %(name)s</title>
  </head>
  <body>
    <h1>RAT Test: %(name)s</h1>
    <p>%(description)s</p>

''' % {'name': self.name, 'description': self.description})

            for p in self.plots:
                f.write('''
     <hr>
     <h3>%(name)s: %(result)s</h3>
     <p>%(description)s</p>
     <center>''' % p)

                f.write('''
     <img src="%(plotname)s"><br>
     <font color="blue">Blue: Standard</font>,  <font color="red">Red: Current Code</font><br>
     <p>KS Probability: %(prob)f</p>''' % p)

                # Optional table showing fit parameters
                if p['parameters']:
                    f.write('''
         <table border="1">
         <tr><th>Parameter Name</th><th>Standard</th><th>Current</th><th>Diff</th></tr>
''')
                    for par in p['parameters']:
                        f.write('<tr><td>%(name)s</td><td>%(m_val)f (+/- %(m_err)1.3f)</td><td>%(c_val)f (+/- %(c_err)1.3f)</td><td>' % par)
                        if par['sigma'] > 2.0:
                            f.write('<font color="red">')
                        else:
                            f.write('<font color="green">')
                        f.write('%(sigma)1.1f&sigma;</font></td></tr>\n' % par)
                    f.write('</table>\n')

                f.write('     </center>\n')
            f.write('  </body>\n</html>\n')

def make_template(testname):
    '''
    Standalone function to make a template for a new RAT test.
    All it requires is the name of the test.
    '''
    # The directory where the templates are located.
    template_dir = os.path.join(os.environ['RATROOT'], 'python', 'rattest', 'templates')

    ## RAT test config
    with open(os.path.join(template_dir, 'template.config'), 'r', encoding="utf-8") as f:
        file_data = f.read()

    file_data = file_data.format(testname=testname)

    with open('rattest.config', 'w', encoding="utf-8") as f:
        f.write(file_data)

    ## RAT macro
    with open(os.path.join(template_dir, 'template.mac'), 'r', encoding="utf-8") as f:
        file_data = f.read()

    file_data = file_data.format(testname=testname)

    with open(f'{testname}.mac', 'w', encoding="utf-8") as f:
        f.write(file_data)

    ## ROOT macro
    with open(os.path.join(template_dir, 'template.C'), 'r', encoding="utf-8") as f:
        file_data = f.read()

    with open(f'{testname}.C', 'w', encoding="utf-8") as f:
        f.write(file_data)
