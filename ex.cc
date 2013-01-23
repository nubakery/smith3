//
// SMITH3 - generates spin-free multireference electron correlation programs.
// Filename: ex.cc
// Copyright (C) 2012 Toru Shiozaki
//
// Author: Toru Shiozaki <shiozaki@northwestern.edu>
// Maintainer: Shiozaki group
//
// This file is part of the SMITH3 package.
//
// The SMITH3 package is free software; you can redistribute it and\/or modify
// it under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// The SMITH3 package is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with the SMITH3 package; see COPYING.  If not, write to
// the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//


#include "ex.h"
#include <algorithm>

using namespace std;
using namespace smith;

Ex::Ex(const std::string& oa, const std::string& ob)
  : Operator(oa, ob) {};



shared_ptr<Ex> Ex::copy() const {
  shared_ptr<Ex> tmp(new Ex(a_->label(), b_->label()));
  return tmp;
}


bool Ex::identical(shared_ptr<Ex> o) const {
  bool out = true;
  auto j = o->op().begin();
  for (auto i = op_.begin(); i != op_.end(); ++i, ++j) {
    if (get<1>(*i) != get<1>(*j) || !(*get<0>(*i))->identical(*get<0>(*j))) {
      out = false;
      break;
    }
  }
  return out;
}


void Ex::print() const {
  for (auto& i : op_) {
    cout << (*get<0>(i))->str(false) << " ";
  }
  // (non active) operator
  if (num_nodagger() + num_dagger() != 0) {
    cout << "{";
    for (auto& i : op_) {
      if (get<1>(i) == -1 || get<1>(i) == 2) continue;
      cout << (*get<0>(i))->str() << rho(get<2>(i))->str();
    }
    cout << "} ";
  }
}


// this function make a possible permutation of indices.
pair<bool, double> Ex::permute(const bool proj) {
  // if there is active daggered and no-daggered operators, you cannot do this as it changes the expression
  if ((num_active_nodagger() && num_active_dagger()) || (!proj))
    return make_pair(false, 1.0);

  const vector<int> prev = perm_;
  const int size = prev.size();
  bool next = next_permutation(perm_.begin(), perm_.end());

  vector<int> map(size);
  vector<int> imap(size); // inverse map
  for (int i = 0; i != size; ++i) {
    const int ii = prev.at(i);
    for (int j = 0; j != size; ++j) {
      const int jj = perm_.at(j);
      if (ii == jj) {
        map[i] = j;
        imap[j] = i;
        break;
      }
    }
  }
  // permute indices (I don't remember why op_ is list...)
  vector<tuple<shared_ptr<Index>*, int, int> > tmp(size*2);
  auto oiter = op_.begin();
  for (int i = 0; i != size; ++i, ++oiter) {
    tmp[map[i]*2  ] = *oiter; ++oiter; 
    tmp[map[i]*2+1] = *oiter; 
  }

  // find sign.
  vector<int> act(size);
  oiter = op_.begin();
  for (int i = 0; i != size; ++i, ++oiter) {
    if ((*get<0>(*oiter))->active()) act[i] += 1; ++oiter; 
    if ((*get<0>(*oiter))->active()) act[i] += 1;
  }
  int f = 0;
  for (int i = 0; i != size; ++i) {
    const int inew = i;
    // if #active op is either zero or two, the sign won't change.
    if (act[imap[i]] != 1) continue;
    // operators originally in the left side && target is larger than mine
    for (int j = 0; j != imap[i]; ++j) {
      if (inew < map[j]) f += act[j];
    }
  }
  const double out = (f&1) ? -1.0 : 1.0;

  oiter = op_.begin();
  for (auto t = tmp.begin(); t != tmp.end(); ++t, ++oiter) *oiter = *t; 

  return make_pair(next, out);
}






