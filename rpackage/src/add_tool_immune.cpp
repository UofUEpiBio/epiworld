#include <Rcpp.h>
#include "epiworld/epiworld.hpp"
#include "epiworld-common.hpp"

using namespace Rcpp;

// Immune system
EPI_NEW_TOOL(immune_eff, TSEQ) {
  return EPI_PARAMS(IMMUNE_EFFICACY);
}

EPI_NEW_TOOL(immune_rec, TSEQ) {
  return EPI_PARAMS(IMMUNE_EFFICACY);
}

EPI_NEW_TOOL(immune_death, TSEQ) {
  return EPI_PARAMS(VARIANT_MORTALITY);
}

EPI_NEW_TOOL(immune_trans, TSEQ) {
  CHECK_LATENT()
  return EPI_PARAMS(BASELINE_INFECCTIOUSNESS);
}

//' @rdname new_model
//' @export
// [[Rcpp::export(invisible = true)]]
int add_tool_immune(
    SEXP model,
    std::vector< bool > & baselineseq,
    double preval
) {
  
  // Creating the tool
  epiworld::Tool<TSEQ> immune;
  immune.set_efficacy(immune_eff);
  immune.set_recovery(immune_rec);
  immune.set_death(immune_death);
  immune.set_transmisibility(immune_trans);
  immune.set_sequence_unique(baselineseq);
  
  // Adding it to the models
  Rcpp::XPtr< epiworld::Model<TSEQ> > mptr(model);
  mptr->add_tool(immune, preval);
  
  return 0;
  
}
