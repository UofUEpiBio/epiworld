#include <Rcpp.h>
using namespace Rcpp;
#include "epiworld-common.hpp"


//' @export
//' @rdname new_epi_model
// [[Rcpp::export(invisible = true, rng = false)]]
int edgelist_from_file(
    SEXP model, 
    std::string fname,
    int skip,
    bool directed
    ) {
  
  Rcpp::XPtr< epiworld::Model<TSEQ> > ptr(model);
  ptr->pop_from_adjlist(fname, skip, directed);
  
  return 0;
  
}

//' @export
//' @rdname new_epi_model
// [[Rcpp::export(invisible = true)]]
int edgelist_from_vec(
    SEXP model,
    const std::vector< unsigned int > & source,
    const std::vector< unsigned int > & target,
    bool directed,
    int min_id = -1,
    int max_id = -1
    ) {
  
  Rcpp::XPtr< epiworld::Model<TSEQ> > ptr(model);
  epiworld::AdjList al(source, target, directed, min_id, max_id);
  ptr->pop_from_adjlist(al);
  
  return 0;
  
}
