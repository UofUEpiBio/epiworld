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
    const std::vector< int > & source,
    const std::vector< int > & target,
    bool directed
    ) {
  
  Rcpp::XPtr< epiworld::Model<TSEQ> > ptr(model);
  epiworld::AdjList al(source, target, directed);
  ptr->pop_from_adjlist(al);
  
  return 0;
  
  
}
