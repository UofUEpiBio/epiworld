#include <Rcpp.h>
#include "epiworld-common.hpp"

using namespace Rcpp;

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
  
  EPI_NEW_MUTFUN_LAMBDA(covid19_mut, TSEQ) {
    
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
    
  };
  
  EPI_NEW_VIRUSFUN_LAMBDA(post_covid, TSEQ) {
    
    // We assume individuals cannot become reinfected with the
    // same variant
    EPI_NEW_TOOL_LAMBDA(post_rec_susceptibility_reduction, TSEQ) {
      return *t->p00;
    };
    
    epiworld::Tool<TSEQ> immunity;
    immunity.set_sequence(*v->get_sequence());
    immunity.set_susceptibility_reduction(post_rec_susceptibility_reduction);
    // immunity.set_param("Immune sys learning rate", *m);
    immunity.set_param("post-covid immunity", *m);
    
    p->add_tool(m->today(), immunity);
    
  };
  
  epiworld::Virus<TSEQ> covid19(baselineseq, "COVID19");
  covid19.set_mutation(covid19_mut);
  covid19.set_post_recovery(post_covid);

  Rcpp::XPtr< epiworld::Model<TSEQ> > ptr(model);
  
  covid19.add_param(mutrate, "covid19 mutation rate", *ptr);
  covid19.add_param(post_immunity, "post-covid immunity", *ptr);
  
  ptr->add_virus(covid19, preval); 

  return 0;
  
}
