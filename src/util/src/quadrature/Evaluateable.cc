#include <RAT/Evaluateable.hh>

Evaluateable::Evaluateable() { this->ResetCallCount(); }

double Evaluateable::operator()(double x) {
  count++;
  double rv = this->Evaluate(x);
  return rv;
}

long Evaluateable::GetCallCount() {
  long rv = this->count;
  return rv;
}

void Evaluateable::ResetCallCount() { this->count = 0; }
