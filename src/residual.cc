//
// SMITH3 - generates spin-free multireference electron correlation programs.
// Filename: residual.cc
// Copyright (C) 2013 Matthew MacLeod
//
// Author: Matthew MacLeod <matthew.macleod@northwestern.edu>
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


#include "constants.h"
#include "residual.h"

using namespace std;
using namespace smith;


OutStream Residual::create_target(const int i) const {
  OutStream out;

  out.tt << "class Task" << i << " : public Task {" << endl;
  out.tt << "  protected:" << endl;
  out.tt << "    std::shared_ptr<Tensor> " << target_name__(label_) << "_;" << endl;
  out.tt << "    IndexRange closed_;" << endl;
  out.tt << "    IndexRange active_;" << endl;
  out.tt << "    IndexRange virt_;" << endl;
  if (label_ == "deci")
    out.tt << "    IndexRange ci_;" << endl;
  out.tt << "    const bool reset_;" << endl;
  out.tt << "" << endl;
  out.tt << "    void compute_() {" << endl;
  out.tt << "      if (reset_) " << target_name__(label_) << "_->zero();" << endl;
  out.tt << "    }" << endl;
  out.tt << "" << endl;
  out.tt << "  public:" << endl;
  out.tt << "    Task" << i << "(std::vector<std::shared_ptr<Tensor>> t, const bool reset);" << endl;

  out.cc << "Task" << i << "::Task" << i << "(vector<shared_ptr<Tensor>> t, const bool reset) : reset_(reset) {" << endl;
  out.cc << "  " << target_name__(label_) << "_ =  t[0];" << endl;
  out.cc << "}" << endl << endl << endl;

  out.tt << "    ~Task" << i << "() {}" << endl;
  out.tt << "};" << endl << endl;

  out.ee << "  auto " << label_ << "q = make_shared<Queue>();" << endl;
  out.ee << "  auto tensor" << i << " = vector<shared_ptr<Tensor>>{" << target_name__(label_) << "};" << endl;
  out.ee << "  auto task" << i << " = make_shared<Task" << i << ">(tensor" << i << ", reset);" << endl;
  out.ee << "  " << label_ << "q->add_task(task" << i << ");" << endl << endl;

  return out;
}


OutStream Residual::generate_task(const int ip, const int ic, const vector<string> op, const string scalar, const int i0, bool der, bool diagonal) const {
  stringstream tmp;

  // when there is no gamma under this, we must skip for off-digonal
  string indent = "";

  if (diagonal) {
    tmp << "  shared_ptr<Task" << ic << "> task" << ic << ";" << endl;
    tmp << "  if (diagonal) {" << endl;
    indent += "  ";
  }
  tmp << indent << "  auto tensor" << ic << " = vector<shared_ptr<Tensor>>{" << merge__(op, label_) << "};" << endl;
  tmp << indent << "  " << (diagonal ? "" : "auto ") << "task" << ic
                << " = make_shared<Task" << ic << ">(tensor" << ic << (der || label_=="deci" ? ", cindex" : ", pindex") << (scalar.empty() ? "" : ", this->e0_") << ");" << endl;

  const bool is_gamma = op.front().find("Gamma") != string::npos;
  if (!is_gamma) {
    if (parent_) {
      assert(parent_->parent());
      tmp << indent << "  task" << ip << "->add_dep(task" << ic << ");" << endl;
      tmp << indent << "  task" << ic << "->add_dep(task" << i0 << ");" << endl;
    } else {
      assert(depth() == 0);
      tmp << indent << "  task" << ic << "->add_dep(task" << i0 << ");" << endl;
    }
    tmp << indent << "  " << label_ << "q->add_task(task" << ic << ");" << endl;
  }
  if (diagonal)
    tmp << "  }" << endl;

  if (!is_gamma)
    tmp << endl;

  OutStream out;
  if (!is_gamma) {
    out.ee << tmp.str();
  } else {
    out.gg << tmp.str();
  }
  return out;
}


OutStream Residual::generate_compute_header(const int ic, const list<shared_ptr<const Index>> ti, const vector<shared_ptr<Tensor>> tensors, const bool no_outside) const {
  vector<string> labels;
  for (auto i = ++tensors.begin(); i != tensors.end(); ++i)
    labels.push_back((*i)->label());
  const int ninptensors = count_distinct_tensors__(labels);

  bool need_e0 = false;
  for (auto& s : tensors)
    if (!s->scalar().empty()) need_e0 = true;

  const int arraysize = label_ == "deci" ? 4 : 3;

  const int nindex = ti.size();
  OutStream out;
  out.tt << "class Task" << ic << " : public Task {" << endl;
  out.tt << "  protected:" << endl;
  // if index is empty give dummy arg
  out.tt << "    class Task_local : public SubTask<" << (ti.empty() ? 1 : nindex) << "," << ninptensors << "> {" << endl;
  out.tt << "      protected:" << endl;
  out.tt << "        const std::array<std::shared_ptr<const IndexRange>," << arraysize << "> range_;" << endl << endl;

  out.tt << "        const Index& b(const size_t& i) const { return this->block(i); }" << endl;
  out.tt << "        const std::shared_ptr<const Tensor>& in(const size_t& i) const { return this->in_tensor(i); }" << endl;
  out.tt << "        const std::shared_ptr<Tensor>& out() const { return this->out_tensor(); }" << endl;
  if (need_e0)
    out.tt << "        const double e0_;" << endl;
  out.tt << endl;

  out.tt << "      public:" << endl;
  // if index is empty use dummy index 1 to subtask
  if (ti.empty()) {
    out.tt << "        Task_local(const std::array<std::shared_ptr<const Tensor>," << ninptensors <<  ">& in, std::shared_ptr<Tensor>& out," << endl;
    out.tt << "                   std::array<std::shared_ptr<const IndexRange>," << arraysize << ">& ran" << (need_e0 ? ", const double e" : "") << ")" << endl;
    out.tt << "          : SubTask<1," << ninptensors << ">(std::array<const Index, 1>(), in, out), range_(ran)" << (need_e0 ? ", e0_(e)" : "") << " { }" << endl;
  } else {
    out.tt << "        Task_local(const std::array<const Index," << nindex << ">& block, const std::array<std::shared_ptr<const Tensor>," << ninptensors <<  ">& in, std::shared_ptr<Tensor>& out," << endl;
    out.tt << "                   std::array<std::shared_ptr<const IndexRange>," << arraysize << ">& ran" << (need_e0 ? ", const double e" : "") << ")" << endl;
    out.tt << "          : SubTask<" << nindex << "," << ninptensors << ">(block, in, out), range_(ran)" << (need_e0 ? ", e0_(e)" : "") << " { }" << endl;
  }
  out.tt << endl;
  out.tt << "        void compute() override;" << endl;

  out.dd << "void Task" << ic << "::Task_local::compute() {" << endl;

  if (!no_outside) {
    list<shared_ptr<const Index>> ti_copy = ti;
    if (depth() == 0) {
      if (ti.size() > 1)
        for (auto i = ti_copy.begin(), j = ++ti_copy.begin(); i != ti_copy.end(); ++i, ++i, ++j, ++j)
          swap(*i, *j);
    }

    int cnt = 0;
    for (auto i = ti_copy.rbegin(); i != ti_copy.rend(); ++i)
      out.dd << "  const Index " << (*i)->str_gen() << " = b(" << cnt++ << ");" << endl;
    out.dd << endl;
  }

  return out;
}


OutStream Residual::generate_compute_footer(const int ic, const list<shared_ptr<const Index>> ti, const vector<shared_ptr<Tensor>> tensors) const {
  vector<string> labels;
  for (auto i = ++tensors.begin(); i != tensors.end(); ++i)
    labels.push_back((*i)->label());
  const int ninptensors = count_distinct_tensors__(labels);
  assert(ninptensors > 0);

  bool need_e0 = false;
  for (auto& s : tensors)
    if (!s->scalar().empty()) need_e0 = true;

  const int arraysize = label_ == "deci" ? 4 : 3;

  OutStream out;
  out.dd << "}" << endl << endl << endl;

  out.tt << "    };" << endl;
  out.tt << "" << endl;
  out.tt << "    std::vector<std::shared_ptr<Task_local>> subtasks_;" << endl;
  out.tt << "" << endl;

  out.tt << "    void compute_() override {" << endl;
  out.tt << "      for (auto& i : subtasks_) i->compute();" << endl;
  out.tt << "    }" << endl << endl;

  out.tt << "  public:" << endl;
  out.tt << "    Task" << ic << "(std::vector<std::shared_ptr<Tensor>> t,  std::array<std::shared_ptr<const IndexRange>," << arraysize << "> range" << (need_e0 ? ", const double e" : "") << ");" << endl;

  out.cc << "Task" << ic << "::Task" << ic << "(vector<shared_ptr<Tensor>> t, array<shared_ptr<const IndexRange>," << arraysize << "> range" << (need_e0 ? ", const double e" : "") << ") {" << endl;
  out.cc << "  array<shared_ptr<const Tensor>," << ninptensors << "> in = {{";
  for (auto i = 1; i < ninptensors + 1; ++i)
    out.cc << "t[" << i << "]" << (i < ninptensors ? ", " : "");
  out.cc << "}};" << endl << endl;

  // over original outermost indices
  if (!ti.empty()) {
    out.cc << "  subtasks_.reserve(";
    for (auto i = ti.begin(); i != ti.end(); ++i) {
      if (i != ti.begin()) out.cc << "*";
      out.cc << (*i)->generate_range() << "->nblock()";
    }
    out.cc << ");" << endl;
  }
  // loops
  string indent = "  ";
  for (auto i = ti.begin(); i != ti.end(); ++i, indent += "  ")
    out.cc << indent << "for (auto& " << (*i)->str_gen() << " : *" << (*i)->generate_range() << ")" << endl;
  // add subtasks
  if (!ti.empty()) {
    out.cc << indent  << "subtasks_.push_back(make_shared<Task_local>(array<const Index," << ti.size() << ">{{";
    for (auto i = ti.rbegin(); i != ti.rend(); ++i) {
      if (i != ti.rbegin()) out.cc << ", ";
      out.cc << (*i)->str_gen();
    }
    out.cc << "}}, in, t[0], range" << (need_e0 ? ", e" : "") << "));" << endl;
  } else {
    out.cc << indent  << "subtasks_.push_back(make_shared<Task_local>(in, t[0], range" << (need_e0 ? ", e" : "") << "));" << endl;
  }
  out.cc << "}" << endl << endl << endl;

  out.tt << "    ~Task" << ic << "() {}" << endl;
  out.tt << "};" << endl << endl;
  return out;
}


OutStream Residual::generate_bc(const shared_ptr<BinaryContraction> i) const {
  OutStream out;
  if (depth() != 0) {
    const string bindent = "  ";
    string dindent = bindent;

    out.dd << target_->generate_get_block(dindent, "o", "out()", true);
    out.dd << target_->generate_scratch_area(dindent, "o", "out()", true); // true means zero-out

    list<shared_ptr<const Index>> ti = depth() != 0 ? i->target_indices() : i->tensor()->index();

    // inner loop will show up here
    // but only if outer loop is not empty
    list<shared_ptr<const Index>> di = i->loop_indices();
    vector<string> close2;
    if (ti.size() != 0) {
      out.dd << endl;
      for (auto iter = di.rbegin(); iter != di.rend(); ++iter, dindent += "  ") {
        string index = (*iter)->str_gen();
        out.dd << dindent << "for (auto& " << index << " : *" << (*iter)->generate_range("_") << ") {" << endl;
        close2.push_back(dindent + "}");
      }
    } else {
      int cnt = 0;
      for (auto k = di.begin(); k != di.end(); ++k, cnt++) out.dd << dindent << "const Index " <<  (*k)->str_gen() << " = b(" << cnt << ");" << endl;
      out.dd << endl;
    }

    // retrieving tensor_
    out.dd << i->tensor()->generate_get_block(dindent, "i0", "in(0)");
    out.dd << i->tensor()->generate_sort_indices(dindent, "i0", "in(0)", di) << endl;
    // retrieving subtree_
    string inlabel("in("); inlabel += (same_tensor__(i->tensor()->label(), i->next_target()->label()) ? "0)" : "1)");
    out.dd << i->next_target()->generate_get_block(dindent, "i1", inlabel);
    out.dd << i->next_target()->generate_sort_indices(dindent, "i1", inlabel, di) << endl;

    // call dgemm
    {
      pair<string, string> t0 = i->tensor()->generate_dim(di);
      pair<string, string> t1 = i->next_target()->generate_dim(di);
      if (t0.first != "" || t1.first != "") {
        out.dd << dindent << GEMM << "(\"T\", \"N\", ";
        string tt0 = t0.first == "" ? "1" : t0.first;
        string tt1 = t1.first == "" ? "1" : t1.first;
        string ss0 = t1.second== "" ? "1" : t1.second;
        out.dd << tt0 << ", " << tt1 << ", " << ss0 << "," << endl;
        out.dd << dindent << "       1.0, i0data_sorted, " << ss0 << ", i1data_sorted, " << ss0 << "," << endl
           << dindent << "       1.0, odata_sorted, " << tt0;
        out.dd << ");" << endl;
      } else {
        string ss0 = t1.second== "" ? "1" : t1.second;
        out.dd << dindent << "odata_sorted[0] += ddot_(" << ss0 << ", i0data_sorted, 1, i1data_sorted, 1);" << endl;
      }
    }

    if (ti.size() != 0) {
      for (auto iter = close2.rbegin(); iter != close2.rend(); ++iter)
        out.dd << *iter << endl;
      out.dd << endl;
    }
    // Inner loop ends here

    // sort buffer
    {
      out.dd << i->target()->generate_sort_indices_target(bindent, "o", di, i->tensor(), i->next_target());
    }
    // put buffer
    {
      string label = target_->label();
      // new interface requires indices for put_block
      out.dd << bindent << "out()->put_block(odata";
      list<shared_ptr<const Index>> ti = depth() != 0 ? i->target_indices() : i->tensor()->index();
      for (auto i = ti.rbegin(); i != ti.rend(); ++i)
        out.dd << ", " << (*i)->str_gen();
      out.dd << ");" << endl;
    }
  } else {  // now at bc depth 0
    // making residual vector...
    list<shared_ptr<const Index>> proj = i->target_index();
    list<shared_ptr<const Index>> res;
    assert(!(proj.size() & 1));
    for (auto i = proj.begin(); i != proj.end(); ++i, ++i) {
      auto j = i; ++j;
      res.push_back(*j);
      res.push_back(*i);
    }
    auto residual = make_shared<Tensor>(1.0, target_name__(label_), res);
    vector<shared_ptr<Tensor>> op2 = { i->next_target() };
    out << generate_compute_operators(residual, op2, i->dagger());
  }


  return out;
}


shared_ptr<Tensor> Residual::create_tensor(list<shared_ptr<const Index>> dm) const {
 return make_shared<Tensor>(1.0, "r", dm);
}






