#include <Rcpp.h>
#include "epiworld-common.hpp"

using namespace Rcpp;

// Wearing a Mask
EPI_NEW_TOOL(mask_eff, TSEQ) {
  return 0.8;
}

EPI_NEW_TOOL(mask_trans, TSEQ) {
  
  CHECK_LATENT()
  return 0.05;
}

//' @rdname new_epi_model
//' @export
// [[Rcpp::export(invisible = true, rng = false)]]
int add_tool_mask(
    SEXP model,
    std::vector< bool > & baselineseq,
    double preval
) {
  
  // Creating the tool
  epiworld::Tool<TSEQ> mask;
  mask.set_efficacy(mask_eff);
  mask.set_transmisibility(mask_trans);
  
  // Adding it to the models
  Rcpp::XPtr< epiworld::Model<TSEQ> > mptr(model);
  mptr->add_tool(mask, preval);
  
  return 0;
  
}
