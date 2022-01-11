#include <Rcpp.h>
#include "epiworld-common.hpp"

using namespace Rcpp;

/*******************************************************************************
 IMMUNE SYSTEM
 *******************************************************************************/

// Immune system
EPI_NEW_TOOL(immune_eff, TSEQ) {
  return *t->p00;
}

EPI_NEW_TOOL(immune_rec, TSEQ) {
  return *t->p01;
}

EPI_NEW_TOOL(immune_death, TSEQ) {
  return *t->p02;
}

EPI_NEW_TOOL(immune_trans, TSEQ) {
  CHECK_LATENT()
  return *t->p03;
}

//' @rdname new_epi_model
//' @param preval Baseline prevalence. What proportion of the population will
//' be assigned this tool.
//' @param susceptibility_reduction Efficacy level (probability of not becoming infected).
//' @param recovery Probability of recovery.
//' @param death Probability of death.
//' @param transm Probability of transmision.
//' @export
// [[Rcpp::export(invisible = true, rng = false)]]
int add_tool_immune(
    SEXP model,
    std::vector< bool > & baselineseq,
    double preval,
    double susceptibility_reduction = 0.1,
    double recovery = 0.1,
    double death    = 0.001,
    double transm   = 0.9
) {
  
  // Creating the tool
  epiworld::Tool<TSEQ> immune("Immune system");
  immune.set_susceptibility_reduction(immune_eff);
  immune.set_recovery(immune_rec);
  immune.set_death(immune_death);
  immune.set_transmisibility(immune_trans);
  immune.set_sequence_unique(baselineseq);
  
  // Getting the model
  Rcpp::XPtr< epiworld::Model<TSEQ> > mptr(model);
  
  immune.add_param(susceptibility_reduction, "immune susceptibility_reduction", *mptr);
  immune.add_param(recovery, "immune recovery", *mptr);
  immune.add_param(death, "immune death", *mptr);
  immune.add_param(transm, "immune transm", *mptr);
  
  // Adding the terms to the model
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
  return *t->p00;
}

EPI_NEW_TOOL(vaccine_rec, TSEQ) {
  return *t->p01;
}

EPI_NEW_TOOL(vaccine_death, TSEQ) {
  return *t->p02;
}

EPI_NEW_TOOL(vaccine_trans, TSEQ) {
  
  CHECK_LATENT()
  return *t->p03;
}

//' @rdname new_epi_model
//' @export
// [[Rcpp::export(invisible = true, rng = false)]]
int add_tool_vaccine(
    SEXP model,
    std::vector< bool > & baselineseq,
    double preval,
    double susceptibility_reduction = 0.9,
    double recovery = 0.4,
    double death    = 0.0001,
    double transm   = 0.5
) {
  
  // Creating the tool
  // Creating tools
  epiworld::Tool<TSEQ> vaccine("Vaccine");
  vaccine.set_susceptibility_reduction(vaccine_eff);
  vaccine.set_recovery(vaccine_rec);
  vaccine.set_death(vaccine_death);
  vaccine.set_transmisibility(vaccine_trans);
  
  // Adding it to the models
  Rcpp::XPtr< epiworld::Model<TSEQ> > mptr(model);
  
  
  vaccine.add_param(susceptibility_reduction, "vax susceptibility_reduction", *mptr);
  vaccine.add_param(recovery, "vax recovery", *mptr);
  vaccine.add_param(death, "vax death", *mptr);
  vaccine.add_param(transm, "vax transm", *mptr);
  
  mptr->add_tool(vaccine, preval);
  
  return 0;
  
}

/*******************************************************************************
 MASKS
 *******************************************************************************/

// Wearing a Mask
EPI_NEW_TOOL(mask_eff, TSEQ) {
  return *t->p00;
}

EPI_NEW_TOOL(mask_trans, TSEQ) {
  
  CHECK_LATENT()
  return *t->p01;
}

//' @rdname new_epi_model
//' @export
// [[Rcpp::export(invisible = true, rng = false)]]
int add_tool_mask(
    SEXP model,
    std::vector< bool > & baselineseq,
    double preval,
    double susceptibility_reduction = 0.3,
    double transm   = 0.1
) {
  
  // Creating the tool
  epiworld::Tool<TSEQ> mask("Face masks");
  mask.set_susceptibility_reduction(mask_eff);
  mask.set_transmisibility(mask_trans);
  
  // Adding it to the models
  Rcpp::XPtr< epiworld::Model<TSEQ> > mptr(model);
  
  mask.add_param(susceptibility_reduction, "mask susceptibility_reduction", *mptr);
  mask.add_param(transm, "mask transm", *mptr);
  
  mptr->add_tool(mask, preval);
  
  return 0;
  
}
