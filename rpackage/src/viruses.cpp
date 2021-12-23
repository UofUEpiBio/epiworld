#include <Rcpp.h>
#include "epiworld-common.hpp"

using namespace Rcpp;

// Defining mutation and transmission functions
EPI_MUTFUN(covid19_mut, TSEQ) {
  
  if (EPI_RUNIF() < *v->p00)
  {
    // Picking a location at random
    int idx = std::floor(EPI_RUNIF() * v->get_sequence()->size());
    TSEQ tmp_seq = *v->get_sequence();
    tmp_seq[idx] = !v->get_sequence()->at(idx); 
    
    // Updating its sequence
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
  return *v->p00;
    
    
}

EPI_RECFUN(post_covid, TSEQ) {
  
  epiworld::Tool<TSEQ> immunity;
  immunity.set_sequence(*v->get_sequence());
  immunity.set_efficacy(post_rec_efficacy);
  immunity.set_param("Immune sys learning rate", *m);
  immunity.set_param("post-covid immunity", *m);
  
  p->add_tool(m->today(), immunity);
  
}

//' @export
//' @rdname new_epi_model
// [[Rcpp::export(invisible = true, rng = false)]]
int add_virus_covid19(
    SEXP model,
    std::vector< bool > baselineseq,
    double preval,
    double mutrate = 0.00005,
    double post_immunity = .95
  ) {
  
  epiworld::Virus<TSEQ> covid19(baselineseq, "COVID19");
  covid19.set_mutation(covid19_mut);
  covid19.set_post_recovery(post_covid);

  Rcpp::XPtr< epiworld::Model<TSEQ> > ptr(model);
  
  covid19.add_param(mutrate, "covid19 mutation rate", *ptr);
  covid19.add_param(post_immunity, "post-covid immunity", *ptr);
  
  ptr->add_virus(covid19, preval); 

  return 0;
  
}
