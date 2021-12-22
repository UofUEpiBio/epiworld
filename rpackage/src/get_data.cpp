#include <Rcpp.h>
using namespace Rcpp;
#include "epiworld-common.hpp"

//' Extract database information from the model
//' 
//' @param what Character (string). What information to extract. leaving `what`
//' equal to `""` will print the available datum.
//' 
//' @export
//' @name get_info
// [[Rcpp::export(rng = false)]]
std::vector< int > get_today_variant(SEXP model, std::string what) {
  Rcpp::XPtr< epiworld::Model< TSEQ > > ptr(model);
  return ptr->get_db().get_today_variant(what);
}

//' @rdname get_info
//' @export
// [[Rcpp::export(rng = false)]]
int get_today_total(SEXP model, std::string what) {
  Rcpp::XPtr< epiworld::Model< TSEQ > > ptr(model);
  return ptr->get_db().get_today_total(what);
}

//' @rdname get_info
//' @export
// [[Rcpp::export(rng = false)]]
std::vector< int > get_hist_total(SEXP model, std::string what) {
  Rcpp::XPtr< epiworld::Model< TSEQ > > ptr(model);
  return ptr->get_db().get_hist_total(what);
}

//' @rdname get_info
//' @export
// [[Rcpp::export(rng = false)]]
std::vector< int > get_hist_variant(SEXP model, std::string what) {
  Rcpp::XPtr< epiworld::Model< TSEQ > > ptr(model);
  return ptr->get_db().get_hist_variant(what);
}




