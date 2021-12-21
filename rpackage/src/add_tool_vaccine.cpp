#include <Rcpp.h>
#include "epiworld-common.hpp"

using namespace Rcpp;


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

//' @rdname new_model
//' @export
// [[Rcpp::export(invisible = true)]]
int add_tool_vaccine(
    SEXP model,
    std::vector< bool > & baselineseq,
    double preval
) {
  
  // Creating the tool
  // Creating tools
  epiworld::Tool<TSEQ> vaccine;
  vaccine.set_efficacy(vaccine_eff);
  vaccine.set_recovery(vaccine_rec);
  vaccine.set_death(vaccine_death);
  vaccine.set_transmisibility(vaccine_trans);
  
  // Adding it to the models
  Rcpp::XPtr< epiworld::Model<TSEQ> > mptr(model);
  mptr->add_tool(vaccine, preval);
  
  return 0;
  
}