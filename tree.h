//
// Author : Toru Shiozaki
// Date   : Jan 2012
//

#ifndef __TREE_H
#define __TREE_H

#include <memory>
#include "equation.h"
#include "listtensor.h"

class Tree;

class BinaryContraction {
  protected:
    // target_ = tensor_ * subtrees
    std::shared_ptr<Tensor> target_;
    std::shared_ptr<Tensor> tensor_;
    std::list<std::shared_ptr<Tree> > subtree_;

    // Tree that has this
    // TODO in principle, I can write a very general iterator by using this.
    // could not used shared_ptr since it creates cyclic reference 
    Tree* parent_;

  public:
    BinaryContraction(std::shared_ptr<Tensor> o, std::shared_ptr<ListTensor> l);
    BinaryContraction(std::list<std::shared_ptr<Tree> > o, std::shared_ptr<Tensor> t) : subtree_(o), tensor_(t) {};
    ~BinaryContraction() {};

    std::list<std::shared_ptr<Tree> >& subtree() { return subtree_; };
    std::shared_ptr<Tensor> tensor() { return tensor_; };
    std::shared_ptr<Tensor> target() { return target_; };
    void print() const;

    void factorize();
    void set_parent(Tree* o) { parent_ = o; };
    void set_parent_subtree();

    bool dagger() const;
    Tree* parent() { return parent_; };

    int depth() const;

};


class Tree : public std::enable_shared_from_this<Tree> {
  protected:
    // recursive structure. BinaryContraction contains Tree.
    std::list<std::shared_ptr<BinaryContraction> > bc_;
    // note that target_ can be NULL (at the very beginning) 
    std::shared_ptr<Tensor> target_;

    std::list<std::shared_ptr<Tensor> > op_;

    bool dagger_;

    // TODO raw pointer should be replaced by some better mean
    BinaryContraction* parent_;

  public:
    Tree(const std::shared_ptr<Equation> eq);
    Tree(const std::shared_ptr<ListTensor> l);
    ~Tree() {};

    bool done() const;
    void print() const;

    std::shared_ptr<Tensor> target() const { return target_; };

    std::string generate() const;

    void set_parent(BinaryContraction* o) { parent_ = o; };
    void set_parent_sub();
    bool dagger() const { return dagger_; };

    // factorize (i.e., perform factorize in BinaryContraction)
    void factorize();

    int depth() const;

};


#endif
