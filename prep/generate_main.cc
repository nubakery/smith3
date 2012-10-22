//
// SMITH3 - generates spin-free multireference electron correlation programs.
// Filename: main.cc
// Copyright (C) 2012 Toru Shiozaki
//
// Author: Toru Shiozaki <shiozaki@northwestern.edu>
// Maintainer: Shiozaki group
//
// This file is part of the SMITH3 package.
//
// The SMITH3 package is free software; you can redistribute it and/or modify
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

#include <iostream>
#include <tuple>
#include <string>
#include <memory>
#include <algorithm>
#include <vector>
#include <list>
#include <cassert>
#include <array>
#include <sstream>
#include <initializer_list>

#include "constants.h"
#include "tensor.h"
#include "diagram.h"
#include "equation.h"

using namespace std;

//const string theory = "MP2";
const string theory = "CAS_all_active";

using namespace SMITH3::Prep;

tuple<vector<shared_ptr<Tensor> >, vector<shared_ptr<Tensor> >, vector<shared_ptr<Tensor> >, vector<shared_ptr<Tensor> > > create_proj() {
  vector<shared_ptr<Tensor> > lp, lt, ls, td;
  array<string, 3> label = {{"c", "x", "a"}};

  int cnt = 0;
  for (auto& i : label) {
    for (auto& j : label) {
      for (auto& k : label) {
        for (auto& l : label) {
#if 0     // MP2 
          if (l == "c" && k == "c" && j == "a" && i == "a") {
#endif
#if 0     // test case for rdm0
          if (l == "c" && k == "x" && j == "x" && i == "x") {
#endif
#if  0    // CASPT2 test case  
          if ((l == "x" && k == "x" && j == "a" && i == "a") 
          ||  (l == "x" && k == "x" && j == "x" && i == "a")) {
#endif
#if 1     // general eight configuration CASPT2 case 
          if (!( l == "x" && k == "x" && j == "x" && i == "x") && !( l == "a" || k == "a" ) && !(j == "c" || i == "c") && !(l == "c" && k == "x") && !(i == "a" && j == "x")) {
#endif
            stringstream ss; ss << cnt;
            lp.push_back(shared_ptr<Tensor>(new Tensor("proj", ss.str(), {l, k, j, i}))); 
            td.push_back(shared_ptr<Tensor>(new Tensor("t2dagger", ss.str(), {l, k, j, i}))); 
            lt.push_back(shared_ptr<Tensor>(new Tensor("t2", ss.str(), {j, i, l, k}))); 
            ls.push_back(shared_ptr<Tensor>(new Tensor("r", ss.str(), {j, i, l, k}))); 
            ++cnt;
          }
        }
      }
    }
  }

  return tie(lp, lt, ls, td);
};

int main() {

  // generate common header
  cout << header() << endl;

  vector<shared_ptr<Tensor> > proj_list, t_list, r_list, t_dagger;
  tie(proj_list, t_list, r_list, t_dagger) = create_proj();

  // make f and H tensors here
  vector<shared_ptr<Tensor> > f = {shared_ptr<Tensor>(new Tensor("f1", "", {"g", "g"}))};
  vector<shared_ptr<Tensor> > H = {shared_ptr<Tensor>(new Tensor("v2", "", {"g", "g", "g", "g"}))};
  vector<shared_ptr<Tensor> > dum = {shared_ptr<Tensor>(new Tensor("proj", "e", {}))};
  
  cout << "  string theory=\"" << theory << "\"; " << endl;
  
  for (auto& i : proj_list) cout << i->generate();
  for (auto& i : t_list)    cout << i->generate();
  for (auto& i : r_list)    cout << i->generate();
  for (auto& i : f)         cout << i->generate();
  for (auto& i : H)         cout << i->generate();
  for (auto& i : dum)       cout << i->generate();
  for (auto& i : t_dagger)  cout << i->generate();
  
  // residual equations
  shared_ptr<Equation> eq0(new Equation("da", {proj_list, f, t_list}));
  shared_ptr<Equation> eq1(new Equation("db", {proj_list, t_list}, "e0"));
  shared_ptr<Equation> eq2(new Equation("dc", {proj_list, H}));
  eq0->merge(eq1);
  eq0->merge(eq2);
  cout << eq0->generate({});

  // energy equations
  shared_ptr<Equation> eq3(new Equation("ea", {dum, t_dagger, H}));
  shared_ptr<Equation> eq4(new Equation("eb", {dum, t_dagger, r_list}));
  eq3->merge(eq4);
  cout << eq3->generate({eq0});
 

  // done generate the footer
  cout << footer(eq0->tree_label(), eq3->tree_label()) << endl;


  return 0;
}


