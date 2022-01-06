#include <Rcpp.h>
using namespace Rcpp;
#include "epiworld-common.hpp"

//' Creates a new model
//' @export
// [[Rcpp::export]]
SEXP new_epi_model() {
  
  Rcpp::XPtr< epiworld::Model< TSEQ > > model(new epiworld::Model<TSEQ>);
  
  model.attr("class") = "epi_model";
  
  return model;
  
}


//' @export
//' @rdname new_epi_model
// [[Rcpp::export(invisible = true, rng = false)]]
int init_epi_model(SEXP model, int nsteps, int seed) {
  
  Rcpp::XPtr< epiworld::Model<TSEQ> > ptr(model);
  
  ptr->init(nsteps, seed);  
  
  return 0;
  
}

//' @export
//' @rdname new_epi_model
// [[Rcpp::export(invisible = true)]]
int run_epi_model(SEXP model) {
  
  Rcpp::XPtr< epiworld::Model<TSEQ> > ptr(model);
  
  // Creating a progress bar

    
  ptr->run();

  return 0;

}

//' @export
//' @param prop Proportion of rewiring
//' @rdname new_epi_model
// [[Rcpp::export(invisible = true)]]
int set_rewire_degseq(SEXP model, double prop) {
  
  Rcpp::XPtr< epiworld::Model<TSEQ> > ptr(model);
  ptr->set_rewire_fun(epiworld::rewire_degseq<TSEQ>);
  ptr->set_rewire_prop(prop);
  
  return 0;
  
}

//' @export
//' @param pname String (character scalar). Name of the parameter to update.
//' @param value Double (numeric scalar). New value for the parameter.
// [[Rcpp::export(invisible=true, rng=false)]]
int update_epi_params(SEXP model, std::string pname, double value) {

  Rcpp::XPtr< epiworld::Model<TSEQ> > ptr(model);
  std::map< std::string, double > & params = ptr->params();
  if (params.find(pname) == params.end())
    stop("The parameter " + pname + " does not exist in the model.");
  
  // If found, then it can be passed
  params[pname] = value;
  
  return 0;
  
}

//' @export
//' @rdname new_epi_model
// [[Rcpp::export(invisible=true, rng=false, name = "print.epi_model")]]
SEXP print_epi_model(SEXP x) {
  Rcpp::XPtr< epiworld::Model<TSEQ> > ptr(x);
  ptr->print();
  return x;
}

//' @export
//' @rdname new_epi_model
// [[Rcpp::export(invisible=true, rng=false)]]
SEXP reset_epi_model(SEXP x) {
  Rcpp::XPtr< epiworld::Model<TSEQ> > ptr(x);
  ptr->reset();
  return x;
}

//' @export
//' @rdname new_epi_model
// [[Rcpp::export(invisible=true, rng=false)]]
int verbose_on_epi_model(SEXP model) {
  Rcpp::XPtr< epiworld::Model<TSEQ> > ptr(model);
  ptr->verbose_on();
  return 0;
}

//' @export
//' @rdname new_epi_model
// [[Rcpp::export(invisible=true, rng=false)]]
int verbose_off_epi_model(SEXP model) {
  Rcpp::XPtr< epiworld::Model<TSEQ> > ptr(model);
  ptr->verbose_off();
  return 0;
}

//' @export
//' @param seed Integer. Seed to be passed.
//' @rdname new_epi_model
// [[Rcpp::export(invisible=true, rng=false)]]
int set_seed_epi_model(SEXP model, int seed) {
  Rcpp::XPtr< epiworld::Model<TSEQ> > ptr(model);
  ptr->seed(seed);
  return 0;
}
