#include <Rcpp.h>
using namespace Rcpp;
#include "epiworld-common.hpp"

//' Extract database information from the model
//' //' 
//' @export
//' @name get_info
// [[Rcpp::export(rng = false)]]
DataFrame get_today_variant(SEXP model) {
  
  Rcpp::XPtr< epiworld::Model< TSEQ > > ptr(model);
  
  std::vector< std::string > status;
  std::vector< int > id;
  std::vector< int > counts;
  
  ptr->get_db().get_today_variant(status,id,counts);
  
  return DataFrame::create(
    _["status"] = wrap(status),
    _["id"]     = wrap(id),
    _["counts"] = wrap(counts)
  );
  
}

//' @rdname get_info
//' @export
// [[Rcpp::export(rng = false)]]
DataFrame get_today_total(SEXP model) {
  
  Rcpp::XPtr< epiworld::Model< TSEQ > > ptr(model);
  
  std::vector< std::string > status;
  std::vector< int > counts;
  
  ptr->get_db().get_today_total(status,counts);
  
  return DataFrame::create(
    _["status"] = wrap(status),
    _["counts"] = wrap(counts)
  );
  
}

//' @rdname get_info
//' @export
// [[Rcpp::export(rng = false)]]
DataFrame get_hist_total(SEXP model) {
  
  Rcpp::XPtr< epiworld::Model< TSEQ > > ptr(model);
  
  std::vector< int > date;
  std::vector< std::string > status;
  std::vector< int > counts;
  
  ptr->get_db().get_hist_total(date,status,counts);
  
  return DataFrame::create(
    _["date"] = wrap(date),
    _["status"] = wrap(status),
    _["counts"] = wrap(counts)
  );
  
}

//' @rdname get_info
//' @export
// [[Rcpp::export(rng = false)]]
DataFrame get_hist_variant(SEXP model) {
  
  Rcpp::XPtr< epiworld::Model< TSEQ > > ptr(model);
  
  std::vector< int > date;
  std::vector< int > id;
  std::vector< std::string > status;
  std::vector< int > counts;
  
  ptr->get_db().get_hist_variant(date,id,status,counts);
  
  return DataFrame::create(
    _["date"] = wrap(date),
    _["id"] = wrap(id),
    _["status"] = wrap(status),
    _["counts"] = wrap(counts)
  );
  
}

