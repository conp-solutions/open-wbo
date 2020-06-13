#!/usr/bin/env python
#
# Norbert Manthey, Copyright 2018, All rights reserved
#
# Methods in this file can be used to check the output of solvers

import sys

def compare_results(wcnf, results, solver_failures, var, cls, top, hard_clauses, soft_clauses):
    detected_failure = 0
    solvers = results.keys()

    code_mapping = {
        'SATISFIABLE': 10,
        'UNSATISFIABLE': 20,
        'OPTIMUM': 30,
        'UNKNOWN': 40,
        'ERROR': 50
    }

    summary = {}
    # check single solver integrity
    for solver in solvers:
        # print >>sys.stderr, "evaluate {} for solver {}".format(results[solver], solver)
        (rc, result, value, model) = results[solver]

        # assertion
        if rc == 134 and solver + "::assertion" not in solver_failures:
            print "Solver {} crashes with an assertion".format(solver)
            detected_failure = 1
            solver_failures[solver + "::assertion"] = wcnf

        # sigsev
        if rc == 139 and solver + "::sigsev" not in solver_failures:
            print "Solver {} crashes with an segmentation fault".format(solver)
            detected_failure = 1
            solver_failures[solver + "::sigsev"] = wcnf

        # wrong return code
        if result not in code_mapping:
            if solver + "::unknown-result" not in solver_failures:
                print "Unknown result {} for solver {}".format(result, solver)
                detected_failure = 1
                solver_failures[solver + "::unknown-result"] = wcnf
        elif (rc != code_mapping[result] and rc != 124 and code_mapping[result] != "SATISFIABLE"):
            if solver + "::wrong-returncode" not in solver_failures:
                print "Return code does not match printed status for solver {}".format(solver)
                detected_failure = 1
                solver_failures[solver + "::wrong-returncode"] = wcnf

        # no optimum value
        if result == "OPTIMUM" and value < 0 and solver + "::no-value" not in solver_failures:
            print "No optimum value given for solver {}".format(solver)
            detected_failure = 1
            solver_failures[solver + "::no-value"] = wcnf

        # too high optimum
        if result == "OPTIMUM" and value > top and solver + "::optimum-toohigh" not in solver_failures:
            print "No optimum value given for solver {}".format(solver)
            detected_failure = 1
            solver_failures[solver + "::optimum-toohigh"] = wcnf

        # unsatisfiable for hard clauses
        #if result == "UNSATISFIABLE" and not hard_clauses and solver + "::invalid-unsat" not in solver_failures:
        #    print "Solver {} reports unsatisfiable without hard clauses in the problem".format(solver)
        #    detected_failure = 1
        #    solver_failures[solver + "::invalid-unsat"] = wcnf

    return detected_failure

