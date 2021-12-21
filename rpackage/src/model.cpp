#include <Rcpp.h>
using namespace Rcpp;
#include "epiworld-common.hpp"

//' Create a new model
//' @export
// [[Rcpp::export]]
SEXP new_model() {
  
  Rcpp::XPtr< epiworld::Model< TSEQ > > model(new epiworld::Model<TSEQ>);
  
  // Setting up the model parameters, these are six
  model->params().resize(8u, 0.0);
  model->params()[MUTATION_PROB]            = 0.005;
  model->params()[VACCINE_EFFICACY]         = 0.90;
  model->params()[VACCINE_RECOVERY]         = 0.20;
  model->params()[VACCINE_DEATH]            = 0.0001;
  model->params()[IMMUNE_EFFICACY]          = 0.10;
  model->params()[VARIANT_MORTALITY]        = 0.001;
  model->params()[BASELINE_INFECCTIOUSNESS] = 0.90;
  model->params()[IMMUNE_LEARN_RATE]        = 0.05;

  model.attr("class") = "epiworld";
  
  return model;
  
}
