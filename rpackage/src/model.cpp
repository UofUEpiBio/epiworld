#include <Rcpp.h>
using namespace Rcpp;
#include "epiworld-common.hpp"

//' Creates a new model
//' @export
// [[Rcpp::export]]
SEXP new_epi_model() {
  
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
  EPIWORLD_CLOCK_START("(01) Run model")
    
  // Initializing the simulation
  EPIWORLD_RUN((*ptr)) 
  {
    
    // We can execute these components in whatever order the
    // user needs.
    ptr->update_status();
    ptr->mutate_variant();
    ptr->next();
    
    // In this case we are applying degree sequence rewiring
    // to change the network just a bit.
    ptr->rewire_degseq(floor(ptr->size() * .1));
    
  }

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
SEXP verbose_on_epi_model(SEXP x) {
  Rcpp::XPtr< epiworld::Model<TSEQ> > ptr(x);
  ptr->verbose_on();
  return x;
}

//' @export
//' @rdname new_epi_model
// [[Rcpp::export(invisible=true, rng=false)]]
SEXP verbose_off_epi_model(SEXP x) {
  Rcpp::XPtr< epiworld::Model<TSEQ> > ptr(x);
  ptr->verbose_off();
  return x;
}
