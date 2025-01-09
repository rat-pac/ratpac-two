#!/usr/bin/env python3
#-*-python-*-

from __future__ import print_function
from __future__ import absolute_import
from __future__ import division

import sys
import os.path
import argparse
import webbrowser
from rattest import RatTest, RatHtml, make_template

#### Parse command line options
parser = argparse.ArgumentParser()

parser.add_argument('input',
                    nargs='+', type=str,
                    help='RAT test(s) to run. Must be a directory or directories.')

parser.add_argument('-u', '--update',
                    action='store_true', dest='update', default=False,
                    help='Update "standard" histogram with current results.')

parser.add_argument('-m', '--regen-mc',
                    action='store_true', dest='regen_mc', default=False,
                    help='Force Monte Carlo to be regenerated.')

parser.add_argument('-r', '--regen-plots',
                    action='store_true', dest='regen_plots', default=False,
                    help='Force histograms to be regenerated.')

parser.add_argument('-t', '--text-only',
                    action='store_false', dest='web', default=True,
                    help='Do not open web pages with plots.')

parser.add_argument('--make-template', type=str, dest='template', default=None,
                    help='Write a template rattest to current directory for you to edit. '\
                    'Supplied name is used for .mac and .C files.')

args = parser.parse_args()

if args.template is not None:
    make_template(args.template)
    print('Writing test template to current directory...')
    sys.exit(0)

#### Open runfile
configname = 'rattest.config'
htmlname = 'results.html'
success = True
for dirname in args.input:
    for dirpath, _, filenames in os.walk(dirname):
        if configname in filenames:
            testcase = RatTest(os.path.join(dirpath, configname))
            if args.update:
                testcase.update(regen_mc=args.regen_mc, regen_plots=args.regen_plots)
            else:
                testcase_html = RatHtml(os.path.join(dirpath, htmlname))
                result = testcase.run(regen_mc=args.regen_mc, regen_plots=args.regen_plots,
                                      html=testcase_html)
                testcase_html.write()
                if args.web:
                    webbrowser.open('file://'+os.path.abspath(testcase_html.htmlname), new=1)
                success = (success and result)

if not success:
    sys.exit(1)
