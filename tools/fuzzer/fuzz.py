#!/usr/bin/env python
#
# Norbert Manthey, Copyright 2018, All rights reserved
#
# This file can be used to let MaxSAT solver(s) run on a given input file, and
# to then evaluate whether there is a mismatch in the output of the solvers
#
# Usage: fuzz.py wcnf.wcnf 'solver1-call ...' ...

import os
import subprocess
import sys

# import the modules we need here
from maxsat import create
from maxsat import run
from maxsat import check

# number of seconds to be allowed for a single solver run (currently, implemented via timout in the shell call)
timeout = 10

# store a CNF for a solver specific failure
solver_failures = {}


def read_input(wcnf, is_string = False):
    var = -1
    cls = -1
    top = 0
    hard_clauses = []
    soft_clauses = []
    weights = []
    weighted = True
    found_header = False
    weight_sum = 0
    lines = []

    # get the input, either from string or file
    if is_string:
	lines = wcnf.splitlines()
    else:
        with open(wcnf, 'r') as f:
            lines = f.read().splitlines()

    # iterate through all the lines of input
    for line in lines:

        # drop comments everywhere
        if line.startswith('c'):
            continue

        # check for the header of unweighted formulas
        if line.startswith('p cnf'):
            if found_header:
                print "detected second p-line: {}, abort".format(line)
                sys.exit(1)
            token = line.split()
            if len(token) < 4:
                print "ill-formed p-line: {}, abort".format(line)
                sys.exit(1)
            var = int(token[2])
            cls = int(token[3])
            top = 1
            weighted = False
            found_header = True
            continue

        # check for the header of weighted formulas
        if line.startswith('p wcnf'):
            if found_header:
                print "detected second p-line: {}, abort".format(line)
                sys.exit(1)
            token = line.split()
            if len(token) < 5:
                print "ill-formed p-line: {}, abort".format(line)
                sys.exit(1)
            var = int(token[2])
            cls = int(token[3])
            top = int(token[4])
            found_header = True
            continue

        # bail if no header found yet
        if not found_header:
                print "expected 'p' header before {}, abort".format(line)
                sys.exit(1)

        # anything else here must be a clause
        token = line.split()
        weight = 1
        if weighted:
            weight = int(token[0])
            clause = [int(t) for t in token[1:]]
            if weight >= top:
                hard_clauses.append(clause)
            else:
                soft_clauses.append(clause)
                weight_sum = weight_sum + weight
                weights.append(weight)
        else:
            clause = [int(t) for t in token]
            soft_clauses.append(clause)
            weight_sum = weight_sum + 1
            weights.append(1)
            # keep track of the theoretical worst case value (all false)
            top = top + 1

    print >>sys.stderr, "sum of weights in formula: {}".format(weight_sum)
    if weight_sum > top:
        print >>sys.stderr, "warning: sum of weights {} is greater than top value {}".format(weight_sum, top)
    return (var, cls, top, hard_clauses, soft_clauses, weights)


def main():
    if len(sys.argv) < 2:
        print "usage: {} iterations 'solver1-call' ...".format(sys.argv[0])
        sys.exit(0)

    iterations = int(sys.argv[1])

    found_failure = 0
    create_only = False

    if iterations < 0:
	create_only = True
        iterations = -iterations

    for iteration in xrange(iterations):

        wcnf = create.generate_wcnf_formula()
        print >>sys.stderr, "run iteration {}/{} with file {}".format(iteration + 1, iterations, wcnf)

        (var, cls, top, hard_clauses, soft_clauses, _) = read_input(wcnf)
        if var < 0:
            print "did not find a header in the given formula {}, abort".format(wcnf)
            sys.exit(1)

        print >>sys.stderr, "parsed formula {} {} {}".format(var, cls, top)

        if create_only:
            continue

        results = {}
        for solver in sys.argv[2:]:
            results[solver] = run.run_benchmark(wcnf, solver, timeout)
            # print >>sys.stderr, "solver: {} terminated with {}".format(solver, results[solver])

        iteration_failure = check.compare_results(wcnf, results, solver_failures, var, cls, top, hard_clauses, soft_clauses)
        if iteration_failure == 1:
            found_failure = 1
        else:
            os.remove(wcnf)

    print "Defect Summary ({} found):".format(len(solver_failures))
    for key in solver_failures.keys():
        print "On {}, found {}".format(solver_failures[key], key)

    return found_failure

if __name__ == "__main__":
    sys.exit(main())
