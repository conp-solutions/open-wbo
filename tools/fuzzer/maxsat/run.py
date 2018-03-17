#!/usr/bin/env python
#
# Norbert Manthey, Copyright 2018, All rights reserved
#
# Methods in this file can be used to run a solver on a benchmark

import subprocess
import sys

def extract_result(stdout):
    result = "UNKNOWN"
    value = -1
    model = {}

    lines = stdout.splitlines()
    for line in lines:
        # store result line
        if line.startswith("s "):
            if len(line.split()) < 2:
                print >>sys.stderr, "found invalid status line, set to ERROR!"
                result = "ERROR"
            else:
                result = line.split()[1].rstrip().lstrip()

        # store optimal value
        if line.startswith("o "):
            if len(line.split()) != 2:
                print >>sys.stderr, "found invalid optimum line {}, skip".format(line)
                result = "ERROR"
            else:
                new_value = int(line.split()[1].rstrip().lstrip())
                if new_value > value and value != -1:
                    print >>sys.stderr, "warning: reported optimum increased from {} to {}, ignore!".format(value, new_value)
                else:
                    value = new_value

        # store model
        if line.startswith("v "):
            token = line.split()
            for truth_value in token[1:-1]:
                lit = int(truth_value)
                var = abs(lit)
                model[var] = (var == lit)
    return (result, value, model)


def run_benchmark(wcnf, solver, timeout):
    cmd = "timeout -s 15 -k " + str(timeout+1) + " " + str(timeout) + " " + solver + " " + wcnf
    print >>sys.stderr, "run {}".format(cmd)
    solver_process = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = solver_process.communicate()
    rc = solver_process.returncode
    if rc < 120:
        (result, value, model) = extract_result(stdout)
        return (rc, result, value, model)
    else:
        return (rc, "ERROR", -1, {})
