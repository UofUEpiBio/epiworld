#include <Rcpp.h>
#include "epiworld-common.hpp"

using namespace Rcpp;

/*******************************************************************************
 IMMUNE SYSTEM
 *******************************************************************************/

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

//' @rdname new_epi_model
//' @export
// [[Rcpp::export(invisible = true, rng = false)]]
int add_tool_immune(
    SEXP model,
    std::vector< bool > & baselineseq,
    double preval
) {
  
  // Creating the tool
  epiworld::Tool<TSEQ> immune("Immune system");
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

/*******************************************************************************
 VACCINE
*******************************************************************************/

// If before the third day of infection, then
// no infectious
#define CHECK_LATENT()                 \
if ((m->today() - v->get_date()) <= 3) \
  return 0.0;

EPI_NEW_TOOL(vaccine_eff, TSEQ) {
  return EPI_PARAMS(VACCINE_EFFICACY);
}

EPI_NEW_TOOL(vaccine_rec, TSEQ) {
  return EPI_PARAMS(VACCINE_RECOVERY);
}

EPI_NEW_TOOL(vaccine_death, TSEQ) {
  return EPI_PARAMS(VARIANT_MORTALITY);
}

EPI_NEW_TOOL(vaccine_trans, TSEQ) {
  
  CHECK_LATENT()
  return EPI_PARAMS(VACCINE_EFFICACY);
}

//' @rdname new_epi_model
//' @export
// [[Rcpp::export(invisible = true, rng = false)]]
int add_tool_vaccine(
    SEXP model,
    std::vector< bool > & baselineseq,
    double preval
) {
  
  // Creating the tool
  // Creating tools
  epiworld::Tool<TSEQ> vaccine("Vaccine");
  vaccine.set_efficacy(vaccine_eff);
  vaccine.set_recovery(vaccine_rec);
  vaccine.set_death(vaccine_death);
  vaccine.set_transmisibility(vaccine_trans);
  
  // Adding it to the models
  Rcpp::XPtr< epiworld::Model<TSEQ> > mptr(model);
  mptr->add_tool(vaccine, preval);
  
  return 0;
  
}

/*******************************************************************************
 MASKS
 *******************************************************************************/

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
  epiworld::Tool<TSEQ> mask("Face masks");
  mask.set_efficacy(mask_eff);
  mask.set_transmisibility(mask_trans);
  
  // Adding it to the models
  Rcpp::XPtr< epiworld::Model<TSEQ> > mptr(model);
  mptr->add_tool(mask, preval);
  
  return 0;
  
}
