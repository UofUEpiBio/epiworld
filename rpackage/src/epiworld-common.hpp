#ifndef EPIWORLD_RPKG_HPP
#define EPIWORLD_RPKG_HPP

#define printf_epiworld Rprintf

#include "epiworld/epiworld.hpp"

#define TSEQ std::vector<bool>

// If before the third day of infection, then
// no infectious
#define CHECK_LATENT()                 \
if ((m->today() - v->get_date()) <= 3) \
  return 0.0;

// PARAMETERS
enum epipar {
  MUTATION_PROB,
  VACCINE_EFFICACY,
  VACCINE_RECOVERY,
  VACCINE_DEATH,
  IMMUNE_EFFICACY,
  VARIANT_MORTALITY,
  BASELINE_INFECCTIOUSNESS,
  IMMUNE_LEARN_RATE
};

#endif