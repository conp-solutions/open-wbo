/* Norbert Manthey, Copyright 2018, All rights reserved
 *
 * This file writes (w)cnf formulas
 */

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

// return a number between -maxvar and maxvar, not considering 0
int randLit(int maxvar)
{
	int i = (rand() % (2*maxvar)) - maxvar; // 0 to (2n - 1) -n == -n to n-1
	i = i >= 0 ? i + 1 : i; // avoid 0
	return i;
}

// return a number that is close to the other number
int relatedLit(int l, int maxvar)
{
	int v = l > 0 ? l : -l;
	int i = (v + maxvar - 5 + ( rand() % 10 )) % maxvar;
	int sign = rand() % 100 < 50;
	i = sign ? -i : i;
	return i >= 0 ? i + 1 : i;
}

int main(int argc, char **argv) {
	
	unsigned seed = time(NULL)*getpid();
	int maxvar = 1 << (20 + 2); // largest variable, 4M
	int maxcls = 1 << (20 + 5); // highest number of clauses, 32M
	int maxweight = 1 << (20 + 5); // weights above this value count as top weight, 32M
        int invalid = 0; // make sure we to only 
        int wcnf = 1; // the printed file is a wcnf

	if(argc>1) maxvar = atoi(argv[1]);
	if(argc>2) maxcls = atoi(argv[2]);
	if(argc>3) maxweight = atoi(argv[3]);
	if(argc>4) seed = atoi(argv[4]);
	if(argc>5) invalid = atoi(argv[5]) != 0;

	// print some kind of header
	stringstream banner;
	banner << "c wcnffuzzer, Norbert Manthey, 2018" << endl
	       << "c seed: " << seed << endl
	       << "c maxvar: " << maxvar << endl
	       << "c maxcls: " << maxcls << endl
	       << "c maxweight: " << maxweight << endl
               << "c invalid: " << invalid << endl;

	// print to stderr, to see even if piped into SAT solver directly
	cout << banner.str(); cerr << banner.str();
	srand(seed);

	// create the size of the formula
	maxvar = (rand() % maxvar) + 1; // do not have an empty formula
	int mincls = maxcls > (3*maxvar) ? 3*maxvar : maxcls/2;
	mincls = (mincls == 0 ? 1 : mincls);
	maxcls = maxcls > mincls ? maxcls : maxcls + 1;
	maxcls = (rand() % (maxcls - mincls)) + mincls;
	if(rand() % 10 == 0 ) wcnf = 0;

        if(invalid && rand() % 1000 < 7) maxweight = -maxweight;
        if(invalid && rand() % 10000 < 3) maxweight -= 1;
        if(invalid && rand() % 10000 < 3) maxweight -= 1;
	
	if(wcnf) cout << "p wcnf " << maxvar << " " << maxcls << " " << maxweight << endl;
	else cout << "p cnf " << maxvar << " " << maxcls << endl;

	// from time to time, do not adhere to the limits in "p cnf"
	int varviolation = rand() % 100;
	varviolation = (varviolation < 90) ? 0 : varviolation - 90;
	int clsviolation = rand() % 100;
	clsviolation = (clsviolation < 90) ? 0 : clsviolation - 90;
	int weightviolation = rand() % 100;
	weightviolation = (weightviolation < 90) ? 0 : weightviolation - 90;
	cout << "c violate var: " << varviolation << " cls: " << clsviolation << " top: " << weightviolation << endl;
        if(invalid)
        {
        	maxvar += varviolation;
	        maxcls += clsviolation;
        	maxweight += weightviolation;
        }
	cout << "c with violation: p wcnf " << maxvar << " " << maxcls << " " << maxweight << endl;

	// generate and print formula
	vector<int> clause;
	stringstream s;
	for(int c = 0; c < maxcls; ++c)
	{
		int size = rand() % 5 + 1 + (c < 30 ? 0 : 1); // only allow "natural" unit clauses in the first 30 clauses
		int weight = rand() % ( 2 * maxweight > 2 ? 2 * maxweight - 1 : 1) + 1; // uniformly distribute weights, including weight 0
                if(!invalid) weight = weight > maxweight ? maxweight : weight; // make sure we stay withing maxweight, if invalid is not enabled
                if(rand() % 1000 < 3) weight = 0;
                if(invalid && rand() % 100000 < 3) weight = -weight;

		clause.resize(size);
		int l = randLit(maxvar);
		clause[0] = l;
		for(int i = 1; i < size; ++ i)
		{
			l = relatedLit(l, maxvar); // this can produce tautologies
			clause[i] = l;
		}
		// remove duplicate elements, might result in units
		sort(clause.begin(), clause.end());
    		auto last = unique(clause.begin(), clause.end());
		clause.erase(last, clause.end());

		// print clause efficiently via stringstream
		s.str("");
		if(wcnf) s << weight << " ";
		for(int l : clause) s << l << " ";
		s << "0" << endl;
		cout << s.str();
	}

        cerr << "c" << endl << "c usage: " << argv[0] << " maxvar maxcls maxweight seed invalid" << endl;

	return 0;
}
