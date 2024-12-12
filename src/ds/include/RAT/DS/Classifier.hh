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
  Classifier() : TObject(), classificationLabels(std::vector<std::string>{""}), classifier_name("") {
    int index = 0;
    for (auto &L : classificationLabels) {
      this->nameIndexMap[L] = index++;
      this->classificationResults.push_back(0.0);
    }
  }
  Classifier(std::string name, std::vector<std::string> labels)
      : TObject(), classificationLabels(labels), classifier_name(name) {
    int index = 0;
    for (auto &L : labels) {
      this->nameIndexMap[L] = index++;
      this->classificationResults.push_back(0.0);
    }
  }
  virtual ~Classifier() {}

  // Classifier name
  virtual const std::string &GetClassifierName() const { return classifier_name; }
  virtual void SetClassifierName(const std::string &_name) { classifier_name = _name; }

  // Classifier Results
  virtual void SetClassificationResult(const std::string &name, double val) {
    classificationResults[nameIndexMap[name]] = val;
  }
  virtual double GetClassificationResult(const std::string &name) { return classificationResults[nameIndexMap[name]]; }

  // Classifier Results
  std::vector<std::string> classificationLabels;
  std::vector<double> classificationResults;
  std::map<std::string, int> nameIndexMap;

  ClassDef(Classifier, 1);

 protected:
  std::string classifier_name;
};

}  // namespace DS
}  // namespace RAT

#endif
