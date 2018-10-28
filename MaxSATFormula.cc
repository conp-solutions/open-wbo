/*!
 * \author Ruben Martins - ruben@sat.inesc-id.pt
 *
 * @section LICENSE
 *
 * Open-WBO, Copyright (c) 2013-2015, Ruben Martins, Vasco Manquinho, Ines Lynce
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <iostream>

#include "MaxSATFormula.h"

using namespace openwbo;

MaxSATFormula *MaxSATFormula::copyMaxSATFormula() {
  assert(format == _FORMAT_MAXSAT_);

  MaxSATFormula *copymx = new MaxSATFormula();
  copymx->setInitialVars(nVars());

  for (int i = 0; i < nVars(); i++)
    copymx->newVar();

  for (int i = 0; i < nSoft(); i++)
    copymx->addSoftClause(getSoftClause(i).weight, getSoftClause(i).clause);

  for (int i = 0; i < nHard(); i++)
    copymx->addHardClause(getHardClause(i).clause);

  copymx->setProblemType(getProblemType());
  copymx->updateSumWeights(getSumWeights());
  copymx->setMaximumWeight(getMaximumWeight());
  copymx->setHardWeight(getHardWeight());

  return copymx;
}

// Adds a new hard clause to the hard clause database.
void MaxSATFormula::addHardClause(const vec<Lit> &lits) {
  hard_clauses.push();

  new (&hard_clauses[hard_clauses.size() - 1]) Hard(lits);
  n_hard++;
}

// Adds a new soft clause to the hard clause database.
void MaxSATFormula::addSoftClause(uint64_t weight, const vec<Lit> &lits) {
  soft_clauses.push();
  vec<Lit> vars;
  Lit assump = lit_Undef;

  new (&soft_clauses[soft_clauses.size() - 1])
      Soft(lits, weight, assump, vars);
  n_soft++;
}

// Adds a new soft clause to the hard clause database with predefined relaxation
// variables.
void MaxSATFormula::addSoftClause(uint64_t weight, const vec<Lit> &lits,
                                  vec<Lit> &vars) {
  soft_clauses.push();
  Lit assump = lit_Undef;

  new (&soft_clauses[soft_clauses.size() - 1])
      Soft(lits, weight, assump, vars);
  n_soft++;
}

int MaxSATFormula::nInitialVars() {
  return n_initial_vars;
} // Returns the number of variables in the working MaxSAT formula.

void MaxSATFormula::setInitialVars(int vars) { n_initial_vars = vars; }

int MaxSATFormula::nVars() {
  return n_vars;
} // Returns the number of variables in the working MaxSAT formula.

int MaxSATFormula::nSoft() {
  return n_soft;
} // Returns the number of soft clauses in the working MaxSAT formula.

int MaxSATFormula::nHard() {
  return n_hard;
} // Returns the number of hard clauses in the working MaxSAT formula.

void MaxSATFormula::newVar(int v) {
  if(v == -1) n_vars++;
  else if(v > n_vars) n_vars = v;
} // Increases the number of variables in the working MaxSAT formula.

// Makes a new literal to be used in the working MaxSAT formula.
Lit MaxSATFormula::newLiteral(bool sign) {
  Lit p = mkLit(nVars(), sign);
  newVar();
  return p;
}

void MaxSATFormula::setProblemType(int type) {
  problem_type = type;
} // Sets the problem type.

int MaxSATFormula::getProblemType() {
  return problem_type; // Return the problem type.
}

// 'ubCost' is initialized to the sum of weights of the soft clauses.
void MaxSATFormula::updateSumWeights(uint64_t weight) {
  if (weight != hard_weight)
    sum_soft_weight += weight;
}

// The initial 'currentWeight' corresponds to the maximum weight of the soft
// clauses.
void MaxSATFormula::setMaximumWeight(uint64_t weight) {
  if (weight > max_soft_weight && weight != hard_weight)
    max_soft_weight = weight;
}

uint64_t MaxSATFormula::getMaximumWeight() { return max_soft_weight; }

void MaxSATFormula::setHardWeight(uint64_t weight) {
  hard_weight = weight;
} // Sets the weight of hard clauses.

Soft &MaxSATFormula::getSoftClause(int pos) {
  assert(pos < nSoft());
  return soft_clauses[pos];
}

Hard &MaxSATFormula::getHardClause(int pos) {
  assert(pos < nHard());
  return hard_clauses[pos];
}

void MaxSATFormula::addPBConstraint(PB *p) {

  // Add constraint to formula data structure.
  if (p->isClause()) {
    addHardClause(p->_lits);
  } else if (p->isCardinality()) {
    if (!p->_sign) {
      p->changeSign();
    }
    cardinality_constraints.push(new Card(p->_lits, p->_rhs));

  } else {
    if (!p->_sign) {
      p->changeSign();
    }

    pb_constraints.push(new PB(p->_lits, p->_coeffs, p->_rhs, p->_sign));
  }
}

int MaxSATFormula::newVarName(char *varName) {
  int id = varID(varName);
  if (id == var_Undef) {
    id = nVars();
    newVar();
    std::string s(varName);
    std::pair<std::string, int> nv(s, id);
    std::pair<int, std::string> ni(id, s);
    _nameToIndex.insert(nv);
    _indexToName.insert(ni);
  }
  return id;
}

int MaxSATFormula::varID(char *varName) {
  std::string s(varName);

  nameMap::const_iterator iter = _nameToIndex.find(s);
  if (iter != _nameToIndex.end()) {
    return iter->second;
  }
  return var_Undef;
}

void MaxSATFormula::convertPBtoMaxSAT() {
  assert(objective_function != NULL);
  vec<Lit> unit_soft(1);

  // Convert objective function to soft clauses
  for (int i = 0; i < objective_function->_lits.size(); i++) {
    assert(objective_function->_coeffs[i] > 0);
    unit_soft[0] = ~objective_function->_lits[i];
    addSoftClause(objective_function->_coeffs[i], unit_soft);

    // Updates the maximum weight of soft clauses.
    setMaximumWeight(objective_function->_coeffs[i]);
    // Updates the sum of the weights of soft clauses.
    updateSumWeights(objective_function->_coeffs[i]);
  }

  if (getMaximumWeight() == 1)
    setProblemType(_UNWEIGHTED_);
  else
    setProblemType(_WEIGHTED_);
}

/** print literals into a stream */
inline std::ostream& operator<<(std::ostream& other, const Lit& l)
{
    if (l == lit_Undef) { other << "lUndef"; }
    else if (l == NSPACE::lit_Error) { other << "lError"; }
    else { other << (NSPACE::sign(l) ? "-" : "") << NSPACE::var(l) + 1; }
    return other;
}

/** print elements of a std::vector */
template <typename T>
inline std::ostream& operator<<(std::ostream& other, const vec<T>& data)
{
    for (int i = 0 ; i < data.size(); ++ i) {
        other << " " << data[i];
    }
    return other;
}

void MaxSATFormula::dumpFormula() const
{
  std::cout << "MaxSAT formula (" << soft_clauses.size() << " soft, " << hard_clauses.size() << " hard)" << std::endl;
  uint64_t weight_sum = 0;
  for(int index = 0 ; index < soft_clauses.size(); ++index)
  {
    std::cout << "soft(" << index << "/" << soft_clauses.size() << "): w: " << soft_clauses[index].weight << " c: " << soft_clauses[index].clause << std::endl;
    weight_sum += soft_clauses[index].weight;
  }
  for(int index = 0 ; index < hard_clauses.size(); ++index)
  {
    std::cout << "hard(" << index << "/" << hard_clauses.size() << "): c: " << hard_clauses[index].clause << std::endl;
  }

  // print wcnf
  const uint64_t top = weight_sum + 1;
  std::cout << "p wcnf " << n_initial_vars << " " << soft_clauses.size() + hard_clauses.size() << " " << top << std::endl;
  for(int index = 0 ; index < soft_clauses.size(); ++index)
    std::cout << soft_clauses[index].weight << " " << soft_clauses[index].clause << " 0" << std::endl;
  for(int index = 0 ; index < hard_clauses.size(); ++index)
    std::cout << top << " " << hard_clauses[index].clause << " 0" << std::endl;
}
