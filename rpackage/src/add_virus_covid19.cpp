#include <Rcpp.h>
#include "epiworld-common.hpp"

using namespace Rcpp;

// Defining mutation and transmission functions
EPI_MUTFUN(covid19_mut, TSEQ) {
  
  if (EPI_RUNIF() < EPI_PARAMS(MUTATION_PROB))
  {
    // Picking a location at random
    int idx = std::floor(EPI_RUNIF() * v->get_sequence()->size());
    TSEQ tmp_seq = *v->get_sequence();
    tmp_seq[idx] = !v->get_sequence()->at(idx); 
    
    // UpTSEQing its sequence
    v->set_sequence(tmp_seq);
    
    return true;
  }
  
  return false;
  
}

// We assume individuals cannot become reinfected with the
// same variant
EPI_NEW_TOOL(post_rec_efficacy, TSEQ) 
{
  
  const auto vseq = v->get_sequence();
  const auto tseq = t->get_sequence();
  
  // If different, then no help
  for (unsigned int i = 0; i < vseq->size(); ++i)
    if (vseq->at(i) != tseq->at(i))
      return 0.0;
    
  // If completely matches, then it is almost 100% efficacy
  return 0.95;
    
    
}

EPI_RECFUN(post_covid, TSEQ) {
  
  epiworld::Tool<TSEQ> immunity;
  immunity.set_sequence(*v->get_sequence());
  immunity.set_efficacy(post_rec_efficacy);
  p->add_tool(m->today(), immunity);
  
}

//' @export
//' @rdname new_epi_model
// [[Rcpp::export(invisible = true, rng = false)]]
int add_virus_covid19(
    SEXP model,
    std::vector< bool > baselineseq,
    double preval
  ) {
  
  epiworld::Virus<TSEQ> covid19(baselineseq);
  covid19.set_mutation(covid19_mut);
  covid19.set_post_recovery(post_covid);

  Rcpp::XPtr< epiworld::Model<TSEQ> > ptr(model);    
  ptr->add_virus(covid19, preval); 

  return 0;
  
}
