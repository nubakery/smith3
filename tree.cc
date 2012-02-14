//
// Author : Toru Shiozaki
// Date   : Jan 2012
//

#include "tree.h"
#include <algorithm>

using namespace std;


BinaryContraction::BinaryContraction(shared_ptr<Tensor> o, shared_ptr<ListTensor> l) {
  // o is a target tensor NOT included in l
  target_ = o;
  tensor_ = l->front(); 
  shared_ptr<ListTensor> rest = l->rest();
  shared_ptr<Tree> tr(new Tree(rest));
  subtree_.push_back(tr);
}


void BinaryContraction::print() const {
  string indent = "";
  for (int i = 0; i != depth(); ++i) indent += "  ";
  cout << indent
       << (target_ ? (target_->str()+" = ") : "") << tensor_->str() << " * " << subtree_.front()->target()->str() << endl;
  for (auto i = subtree_.begin(); i != subtree_.end(); ++i) {
    (*i)->print();
  }
}


void BinaryContraction::factorize() {
  for (auto i = subtree_.begin(); i != subtree_.end(); ++i) (*i)->factorize();
}


void BinaryContraction::set_parent_subtree() {
  for (auto i = subtree_.begin(); i != subtree_.end(); ++i) {
    (*i)->set_parent(this);
    (*i)->set_parent_sub();
  }
}
void Tree::set_parent_sub() {
  for (auto i = bc_.begin(); i != bc_.end(); ++i) {
    (*i)->set_parent(this);
    (*i)->set_parent_subtree();
  }
}


bool BinaryContraction::dagger() const {
  return subtree_.front()->dagger();
}


Tree::Tree(const shared_ptr<ListTensor> l) {
  target_ = l->target();
  dagger_ = l->dagger();
  if (l->length() > 1) {
    shared_ptr<BinaryContraction> bc(new BinaryContraction(target_, l)); 
    bc_.push_back(bc);
  } else {
    shared_ptr<Tensor> t = l->front();
    t->set_factor(l->fac());
    op_.push_back(t); 
  }
}


Tree::Tree(shared_ptr<Equation> eq) : parent_(NULL) {

  // First make ListTensor for all the diagrams
  list<shared_ptr<Diagram> > d = eq->diagram();
  for (auto i = d.begin(); i != d.end(); ++i) {
    shared_ptr<ListTensor> tmp(new ListTensor(*i));
    // All internal tensor should be included in the active part
    tmp->absorb_all_internal();

    shared_ptr<Tensor> first = tmp->front();
    shared_ptr<ListTensor> rest = tmp->rest();

    // convert it to tree
    shared_ptr<Tree> tr(new Tree(rest));
    list<shared_ptr<Tree> > lt; lt.push_back(tr);
    shared_ptr<BinaryContraction> b(new BinaryContraction(lt, first));
    bc_.push_back(b);
  }

  factorize();
  set_parent_sub();
}


void Tree::print() const {
  string indent = "";
  for (int i = 0; i != depth(); ++i) indent += "  ";
  if (target_) {
    for (auto i = op_.begin(); i != op_.end(); ++i)
      cout << indent << target_->str() << "+= " << (*i)->str() << dagger_ << endl;
  }
  for (auto i = bc_.begin(); i != bc_.end(); ++i)
    (*i)->print();
}


bool Tree::done() const {
  // vectensor should be really a tensor 
#if 0
  bool out = true;
  for (auto i = tensor_.begin(); i != tensor_.end(); ++i) out &= ((*i)->length() == 1);
  return out;
#endif
}


void Tree::factorize() {
  list<list<shared_ptr<BinaryContraction> >::iterator> done;
#if 0
  for (auto i = bc_.begin(); i != bc_.end(); ++i) {
    (*i)->factorize();
  }
#endif
  for (auto i = bc_.begin(); i != bc_.end(); ++i) {
    if (find(done.begin(), done.end(), i) != done.end()) continue;
    auto j = i; ++j;
    for ( ; j != bc_.end(); ++j) {
// TODO check dagger
      if (*(*i)->tensor() == *(*j)->tensor() && (*i)->dagger() == (*j)->dagger()) {
        done.push_back(j);
        (*i)->subtree().insert((*i)->subtree().end(), (*j)->subtree().begin(), (*j)->subtree().end()); 
      }
    }
  }
  for (auto i = done.begin(); i != done.end(); ++i)
    bc_.erase(*i);
}


int BinaryContraction::depth() const { return parent_->depth(); }
int Tree::depth() const { return parent_ ? parent_->parent()->depth()+1 : 0; }
