#ifndef EPIWORLD_MODELS_SEIRD_HPP
#define EPIWORLD_MODELS_SEIRD_HPP

#include "../model-bones.hpp"

/**
 * @brief Template for a Susceptible-Exposed-Infected-Removed-Deceased (SEIRD) model
 * 
 * ![Model Diagram](../assets/img/seird.png)
 * 
 * @ingroup death_compartmental
*/
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSEIRD : public Model<TSeq>
{
  
public:
  static const int SUSCEPTIBLE = 0;
  static const int EXPOSED     = 1;
  static const int INFECTED    = 2;
  static const int REMOVED     = 3;
  static const int DECEASED    = 4;
  
  ModelSEIRD() = delete;

  /**
   * @brief Constructor for the SEIRD model.
   * 
   * @param vname Name of the model.
 * @param n Number of agents.
   * @param prevalence Initial prevalence of the disease.
   * @param transmission_prob Probability that a contact with an infected
   * agent infects.
   * @param latent_period Mean days in Exposed (carrying the virus, not yet
   * infectious).
   * @param recovery_rate Recovery rate of the disease.
   * @param death_rate Death rate of the disease.
   */
  ModelSEIRD(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double latent_period,
    epiworld_double recovery_rate,
    epiworld_double death_rate
  );
  
  ModelSEIRD<TSeq> & initial_states(
    std::vector< double > proportions_,
    std::vector< int > queue_ = {}
  ) override;

};



template<typename TSeq>
inline ModelSEIRD<TSeq>::ModelSEIRD(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double latent_period,
    epiworld_double recovery_rate,
    epiworld_double death_rate
)
{

    this->set_population_size(n);
  
  this->add_param(transmission_prob, "Transmission probability");
  this->add_param(latent_period, "Latent period");
  this->add_param(recovery_rate, "Recovery rate");
  this->add_param(death_rate, "Death rate");

  NaturalHistory<TSeq> nh(vname);
  nh.add_stage("Susceptible");
  nh.add_stage("Exposed", stage::carrier);
  nh.add_stage("Infected", stage::infectious);
  nh.add_stage("Removed");
  nh.add_stage("Deceased");
  nh.set_contact(DECEASED, 0.0);
  nh.infection(SUSCEPTIBLE, EXPOSED, "Transmission probability");
  nh.after(EXPOSED, INFECTED, Duration<TSeq>::geometric("Latent period"));
  nh.rate(INFECTED, REMOVED, "Recovery rate").effect(Effect::recovery);
  nh.rate(INFECTED, DECEASED, "Death rate").effect(Effect::death);
  this->set_natural_history(std::move(nh));

  this->add_virus(Virus<TSeq>(vname), Seed::proportion(prevalence));

  this->set_name("Susceptible-Exposed-Infected-Removed-Deceased (SEIRD)");
  
}

template<typename TSeq>
inline ModelSEIRD<TSeq> & ModelSEIRD<TSeq>::initial_states(
  std::vector< double > proportions_,
  std::vector< int > /**/
) {

  if (proportions_.size() != 3u)
    throw std::invalid_argument("-proportions_- must have three entries.");

  for (auto & v : proportions_)
    if ((v < 0.0) || (v > 1.0))
      throw std::invalid_argument(
        "-proportions_- must have values between 0 and 1."
      );

  if ((proportions_[1u] + proportions_[2u]) > 1.0)
    throw std::invalid_argument(
      "The last two terms of -proportions_- must add up to less than 1."
    );

  // Of the initial cases, a share starts Infected; of the others, shares
  // start Removed and Deceased
  this->nh_seeds[0u].split = {
    {EXPOSED, 1.0 - proportions_[0u]},
    {INFECTED, proportions_[0u]}
  };
  this->set_initial(REMOVED, proportions_[1u]);
  this->set_initial(DECEASED, proportions_[2u]);

  return *this;

}


#endif