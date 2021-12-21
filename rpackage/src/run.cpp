#include <Rcpp.h>
#include "epiworld-common.hpp"

using namespace Rcpp;

//' @export
//' @rdname new_model
// [[Rcpp::export(invisible = true)]]
int run_model(SEXP model, int nsteps, int seed) {
  
  Rcpp::XPtr< epiworld::Model<TSEQ> > ptr(model);
  
  ptr->init(nsteps, seed);  
  
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
    Rprintf("|");
    
  }

  Rprintf("\n");
  
  return 0;
  
  
}

