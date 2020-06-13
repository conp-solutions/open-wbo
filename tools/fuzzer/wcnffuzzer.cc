/* Norbert Manthey, Copyright 2018, All rights reserved
 *
 * This file writes (w)cnf formulas
 */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

// return a number between -maxvar and maxvar, not considering 0
int randLit(int maxvar)
{
    int i = (rand() % (2 * maxvar)) - maxvar; // 0 to (2n - 1) -n == -n to n-1
    i = i >= 0 ? i + 1 : i;                   // avoid 0
    return i;
}

// return a number that is close to the other number
int relatedLit(int l, int maxvar)
{
    int v = l > 0 ? l : -l;
    int i = (v + maxvar - 5 + (rand() % 10)) % maxvar;
    int sign = rand() % 100 < 50;
    i = sign ? -i : i;
    return i >= 0 ? i + 1 : i;
}

int main(int argc, char **argv)
{

    unsigned seed = time(NULL) * getpid();
    int maxvar = 500;                       // largest variable, 4M
    int maxcls = 2000;                      // highest number of clauses, 32M
    int64_t maxweight = 200;                // weights above this value count as top weight, 32M
    uint64_t top = 0;                       // sum of all soft weights
    int invalid = 0;                        // make sure we to only
    int wcnf = 1;                           // the printed file is a wcnf
    int soft_clauses = 0, hard_clauses = 0; // count and use to print statistics
    int unweighted = 0;                     // turn all weights into '1'
    int huge_weights = 0;                   // try to get weights close to 63 bit values, and the sum close to 1 << 64
    uint64_t max_weights = ~(0ULL) - 1;     // this is the maximum we can get

    if (argc > 1) seed = atoi(argv[1]);
    if (argc > 2) maxvar = atoi(argv[2]);
    if (argc > 3) maxcls = atoi(argv[3]);
    if (argc > 4) {
        maxweight = atoi(argv[4]);
        if (maxweight < 0) {
            huge_weights = (rand() % 100) < 15;
            maxweight = -maxweight;
        }
    } else {
        huge_weights = (rand() % 100) < 15;
    }
    if (argc > 5) invalid = atoi(argv[5]) != 0;

    // print some kind of header
    stringstream banner;
    banner << "c wcnffuzzer, Norbert Manthey, 2018" << endl
           << "c seed: " << seed << endl
           << "c maxvar: " << maxvar << endl
           << "c maxcls: " << maxcls << endl
           << "c maxweight: " << maxweight << endl
           << "c invalid: " << invalid << endl;

    // print to stderr, to see even if piped into SAT solver directly
    cout << banner.str();
    cerr << banner.str();
    srand(seed);

    // create the size of the formula
    maxvar = (rand() % maxvar) + 1; // do not have an empty formula
    int mincls = maxcls > (3 * maxvar) ? 3 * maxvar : maxcls / 2;
    mincls = (mincls == 0 ? 1 : mincls);
    maxcls = maxcls > mincls ? maxcls : maxcls + 1;
    maxcls = (rand() % (maxcls - mincls)) + mincls;
    if (rand() % 10 == 0) wcnf = 0; // with 10%, create a unweighted formula
    if (wcnf == 0) {
        unweighted = (rand() % 10) < 5; // with 40% (of all unweighted formulas) make the formula a unweighted weighted formula
        if (unweighted) wcnf = 1;
    }

    if (unweighted)
        cout << "c enable unweighted wcnf" << endl;
    else
        huge_weights = (rand() % 100) < 15;

    cout << "c enable very large weights: " << (huge_weights ? "yes" : "no") << endl;

    if (invalid && rand() % 1000 < 7) maxweight = -maxweight;
    if (invalid && rand() % 10000 < 3) maxweight -= 1;
    if (invalid && rand() % 10000 < 3) maxweight -= 1;

    // pre-generate all weights
    uint64_t weight_sum = 0;
    vector<int64_t> weights;
    for (int c = 0; c < maxcls; ++c) {
        int weight = rand() % (2 * maxweight > 2 ? 2 * maxweight - 1 : 1) + 1; // uniformly distribute weights, including weight 0
        if (!invalid)
            weight = weight > maxweight ? maxweight : weight; // make sure we stay withing maxweight, if invalid is not enabled
        if (invalid && rand() % 1000 < 3) weight = 0;         // for now we do not rely on weight == 0
        if (invalid && rand() % 100000 < 3) weight = -weight;

        if (huge_weights) weight = (int)(max_weights / maxcls) > 2 * weight ? (max_weights / maxcls) - weight : weight;
        if (unweighted) weight = 1; // overwrite the weight

        if (weight > 0) weight_sum += weight;
        weights.push_back(weight);
    }
    top = weight_sum + 1;

    if (wcnf)
        cout << "p wcnf " << maxvar << " " << maxcls << " " << top << endl;
    else
        cout << "p cnf " << maxvar << " " << maxcls << endl;

    // from time to time, do not adhere to the limits in "p cnf"
    int varviolation = rand() % 100;
    varviolation = (varviolation < 90) ? 0 : varviolation - 90;
    int clsviolation = rand() % 100;
    clsviolation = (clsviolation < 90) ? 0 : clsviolation - 90;
    int weightviolation = rand() % 100;
    weightviolation = (weightviolation < 90) ? 0 : weightviolation - 90;
    cout << "c violate var: " << varviolation << " cls: " << clsviolation << " top: " << weightviolation << endl;
    if (invalid) {
        maxvar += varviolation;
        maxcls += clsviolation;
        maxweight += weightviolation;
    }
    cout << "c with violation: p wcnf " << maxvar << " " << maxcls << " " << top << endl;

    // make sure we also have weights for the potential new invalid clauses
    for (; weights.size() < (size_t)maxcls;) {
        int weight = rand() % (2 * maxweight > 2 ? 2 * maxweight - 1 : 1) + 1; // uniformly distribute weights, including weight 0
        if (!invalid)
            weight = weight > maxweight ? maxweight : weight; // make sure we stay withing maxweight, if invalid is not enabled
        weight_sum += weight;                                 // from here on the value can only get smaller

        if (invalid && rand() % 1000 < 3) weight = 0; // for now we do not rely on weight == 0
        if (invalid && rand() % 100000 < 3) weight = -weight;

        if (unweighted) weight = 1; // overwrite the weight

        weights.push_back(weight);
    }

    // make sure to lift up all weights for hard clauses to the top value
    int64_t hard_sum = 0, soft_sum = 0, prev_sum = 0;
    for (size_t i = 0; i < weights.size(); ++i) {
        if (weights[i] >= maxweight) {
            weights[i] = top + (weights[i] - maxweight);
            hard_clauses++;
            hard_sum += weights[i];
        } else {
            soft_clauses++;
            soft_sum += weights[i];
            if(soft_sum < prev_sum) {
                cout << "c error: sum of weights overflowed, abort" << endl;
                exit(1);
            }
        }
    }
    cout << "c hard clauses: " << hard_clauses << endl
         << "c soft clauses: " << soft_clauses << endl
         << "c sum hard weights: " << hard_sum << endl
         << "c sum soft weights: " << soft_sum << endl;

    // generate and print formula
    vector<int> clause;
    stringstream s;
    for (int c = 0; c < maxcls; ++c) {
        int size = rand() % 5 + 1 + (c < 30 ? 0 : 1); // only allow "natural" unit clauses in the first 30 clauses


        clause.resize(size);
        int l = randLit(maxvar);
        clause[0] = l;
        for (int i = 1; i < size; ++i) {
            l = relatedLit(l, maxvar); // this can produce tautologies
            clause[i] = l;
        }
        // remove duplicate elements, might result in units
        sort(clause.begin(), clause.end());
        auto last = unique(clause.begin(), clause.end());
        clause.erase(last, clause.end());

        // print clause efficiently via stringstream
        s.str("");
        if (wcnf) s << weights[c] << " ";
        for (int l : clause) s << l << " ";
        s << "0" << endl;
        cout << s.str();
    }

    // as everything else is also print to screen, print the usage last, so that it's visible
    cerr << "c" << endl << "c usage: " << argv[0] << " [seed [maxvar [maxcls [maxweight [invalid]]]]]" << endl;

    return 0;
}
