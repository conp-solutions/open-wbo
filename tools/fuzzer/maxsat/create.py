#!/usr/bin/env python
#
# Norbert Manthey, Copyright 2018, All rights reserved
#
# Methods in this file can be used to generate new formulas and print them to a file

import subprocess
import sys

# default call for the fuzzer to create WCNF files
fuzzer_call = "./wcnffuzzer"

def generate_wcnf_formula(return_as_string = False):

    attempt = 0

    # retry up to 5 times, before giving up
    while attempt < 5:
        attempt = attempt + 1
        fuzzer_process = subprocess.Popen(fuzzer_call, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = fuzzer_process.communicate()
        rc = fuzzer_process.returncode
        if rc == 0:
            break

    if attempt >= 5:
        print "fuzzer call returned with {}, abort fuzzing run".format(rc)
        sys.exit(1)

    lines = stdout.splitlines()
    seed = -1
    for line in lines:
        if line.startswith("c seed:"):
            print >>sys.stderr, "extract seed from line {}".format(line)
            seed = line.split()[2]
            break

    # make sure we have a file
    if seed == -1:
        print "did not find a seed in the fuzzer output, abort fuzzing run"
        sys.exit(1)

    # Return the formula via string, so no need to actually write a file and
    # return its name.
    if return_as_string:
        return stdout

    wcnf_name = "wcnffuzzer_" + str(seed) + ".wcnf"
    with open(wcnf_name, 'w') as outfile:
        outfile.write(stdout)

    # return the name of the file we just created
    return wcnf_name
