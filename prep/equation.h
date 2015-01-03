#ifndef __EQUATION_H
#define __EQUATION_H

#include "diagram.h"
#include <sstream>
#include <memory>
#include <list>
#include <vector>
#include <string>

namespace SMITH3 {
namespace Prep {

class Equation {
  protected:
    std::list<std::shared_ptr<Diagram>> diagram_;
    std::string label_;
    double fac_;
    std::string tree_type_;
    std::string tree_name_;
    std::pair<bool, bool> braket_;
    bool ci_derivative_;

  public:
    Equation(const std::string l, const std::initializer_list<std::vector<std::shared_ptr<Tensor>> > in) : Equation(l, in, 1.0, "", std::make_pair(false,false)) { }
    Equation(const std::string l, const std::initializer_list<std::vector<std::shared_ptr<Tensor>> > in, const double d) : Equation(l, in, d, "", std::make_pair(false,false)) { }
    Equation(const std::string l, const std::initializer_list<std::vector<std::shared_ptr<Tensor>> > in, const std::string scalar) : Equation(l, in, 1.0, scalar, std::make_pair(false,false)) { }
    Equation(const std::string l, const std::initializer_list<std::vector<std::shared_ptr<Tensor>> > in, const double d, const std::string scalar) : Equation(l, in, d, scalar, std::make_pair(false,false)) { }
    // with ci derivative
    Equation(const std::string l, const std::initializer_list<std::vector<std::shared_ptr<Tensor>> > in, const std::pair<bool,bool> brkt) : Equation(l, in, 1.0, "", brkt) { }
    Equation(const std::string l, const std::initializer_list<std::vector<std::shared_ptr<Tensor>> > in, const double d, const std::pair<bool,bool> brkt) : Equation(l, in, d, "", brkt) { }
    Equation(const std::string l, const std::initializer_list<std::vector<std::shared_ptr<Tensor>> > in, const std::string scalar, const std::pair<bool,bool> brkt) : Equation(l, in, 1.0, scalar, brkt) { }
    // end ctor
    Equation(const std::string l, const std::initializer_list<std::vector<std::shared_ptr<Tensor>> > in, const double d, const std::string scalar, const std::pair<bool,bool> bk) : label_(l), fac_(d), tree_type_(""), braket_(bk) {

      // mkm assert(bk.first && bk.second);
      if (bk.first == false && bk.second == false) {
        ci_derivative_ = false;
      } else {
        ci_derivative_ = true;
      }


      std::list<int> max;
      for (auto& i : in) max.push_back(i.size());

      std::list<std::list<std::shared_ptr<Tensor>>> out;
      std::list<int> current(in.size(), 0);
      std::list<int> start = current;
      do {
        // set the current vector
        std::list<std::shared_ptr<Tensor>> cc;
        auto inp = in.begin();
        for (auto i = current.begin(); i != current.end(); ++i, ++inp) cc.push_back((*inp)[*i]);
        out.push_back(cc);

        // get the next vector
        auto m = max.rbegin();
        for (auto i = current.rbegin(); i != current.rend(); ++i, ++m) {
          if (++*i == *m) {
            *i = 0;
          } else {
            break;
          }
        }
      } while (current != start);

      // construct Diagrams
      int cnt = 0;
      for (auto& i : out) {
        std::stringstream ss; ss << label_ << cnt;
        if (!ci_derivative_) {
          if (d == 1.0) {
            diagram_.push_back(std::make_shared<Diagram>(i, ss.str(), scalar));
          } else {
            diagram_.push_back(std::make_shared<Diagram>(i, ss.str(), d, scalar));
          }
        } else {
          if (d == 1.0) {
            diagram_.push_back(std::make_shared<Diagram>(i, ss.str(), scalar, bk));
          } else {
            diagram_.push_back(std::make_shared<Diagram>(i, ss.str(), d, scalar, bk));
          }
        }
        ++cnt;
      }

    }


    std::string tree_label() const { return "t" + label_; }

    void set_tree_type(std::string ttype, std::string tname = "") { tree_type_=ttype; tree_name_ = tname.empty() ? ttype : tname; }

    void merge(std::shared_ptr<Equation> o) {
      diagram_.insert(diagram_.end(), o->diagram_.begin(), o->diagram_.end());
    }

    // mkm std::string generate(std::initializer_list<std::shared_ptr<Equation>> o) const {
    std::string generate() const {
      std::stringstream ss;
      for (auto& i : diagram_) ss << i->construct_str();
      for (auto& i : diagram_) ss << i->diagram_str();
      for (auto& i : diagram_) ss << i->equation_str();

      for (auto i = diagram_.begin(); i != diagram_.end(); ++i)
        if (i != diagram_.begin())
          ss << "  " << diagram_.front()->eqn_label() << "->merge(" << (*i)->eqn_label() << ");" << std::endl;
      if (ci_derivative_) ss << "  " << diagram_.front()->eqn_label() << "->absorb_ket();" << std::endl;
      ss << "  " << diagram_.front()->eqn_label() << "->duplicates();" << std::endl;
      ss << "  " << diagram_.front()->eqn_label() << "->active();" << std::endl;

      if (!tree_type_.empty() && tree_type_ == "residual") {
          ss << "  auto " << tree_label() << " = make_shared<Residual>(e" << diagram_.front()->label() << ", \"" << tree_name_ << "\");" << std::endl;
      } else if (!tree_type_.empty() && tree_type_ == "energy") {
          ss << "  auto " << tree_label() << " = make_shared<Energy>(e" << diagram_.front()->label() << ", \"" << tree_name_ << "\");" << std::endl;
      } else if (!tree_type_.empty() && tree_type_ == "dedci") {
          ss << "  auto " << tree_label() << " = make_shared<Dedci>(e" << diagram_.front()->label() << ", \"" << tree_name_ << "\");" << std::endl;
      } else if (!tree_type_.empty() && tree_type_ == "correction") {
          ss << "  auto " << tree_label() << " = make_shared<Correction>(e" << diagram_.front()->label() << ", \"" << tree_name_ << "\");" << std::endl;
      } else if (!tree_type_.empty() && tree_type_ == "density") {
          ss << "  auto " << tree_label() << " = make_shared<Density>(e" << diagram_.front()->label() << ", \"" << tree_name_ << "\");" << std::endl;
      } else if (!tree_type_.empty() && tree_type_ == "density1") {
          ss << "  auto " << tree_label() << " = make_shared<Density1>(e" << diagram_.front()->label() << ", \"" << tree_name_ << "\");" << std::endl;
      } else if (!tree_type_.empty() && tree_type_ == "density2") {
          ss << "  auto " << tree_label() << " = make_shared<Density2>(e" << diagram_.front()->label() << ", \"" << tree_name_ << "\");" << std::endl;
      } else {
          throw std::logic_error("prep/equation.cc error, tree must be of derived type");
      }

      ss << std::endl;
      return ss.str();
    };


};

}}


#endif
