#ifndef __RAT_Classifier__
#define __RAT_Classifier__

#include <TObject.h>

#include <map>
#include <string>
#include <vector>

namespace RAT {
namespace DS {

class Classifier : public TObject {
 public:
  Classifier(std::string _name = "", std::string _tag = "") : TObject(), classifier_name(_name), tag(_tag) {}
  virtual ~Classifier() {}

  // Classifier name
  virtual const std::string &GetClassifierName() const { return classifier_name; }
  virtual void SetClassifierName(const std::string &_name) { classifier_name = _name; }
  virtual const std::string &GetTag() const { return tag; }
  virtual void SetTag(const std::string &_tag) { tag = _tag; }
  virtual const std::string GetFullName() const {
    if (tag.empty()) return classifier_name;
    return classifier_name + "__" + tag;
  }

  // Classifier Results
  virtual void SetClassificationResult(const std::string &name, double val) { classificationResults[name] = val; }
  virtual double GetClassificationResult(const std::string &name) {
    if (classificationResults.find(name) == classificationResults.end()) {
      return -9999;
    }
    return classificationResults.at(name);
  }

  // Classifier Results
  std::map<std::string, double> classificationResults;

  ClassDef(Classifier, 2);

 protected:
  std::string classifier_name;  // name of the classifier that produced this result
  std::string tag;              // appended label for this specific classifier result
};

}  // namespace DS
}  // namespace RAT

#endif
