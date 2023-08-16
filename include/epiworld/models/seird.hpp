#ifndef EPIWORLD_MODELS_SEIRD_HPP
#define EPIWORLD_MODELS_SEIRD_HPP

/**
 * @brief Template for a Susceptible-Exposed-Infected-Removed-Deceased (SEIRD) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param prevalence epiworld_double Initial prevalence the immune system
 * @param transmission_rate epiworld_double Transmission rate of the virus
 * @param avg_incubation_days epiworld_double Average incubation days of the virus
 * @param recovery_rate epiworld_double Recovery rate of the virus.
 * @param death_rate epiworld_double Death rate of the virus.
 */
template<typename TSeq = int>
class ModelSEIRD : public epiworld::Model<TSeq>
{
private:
    static const int SUSCEPTIBLE = 0;
    static const int EXPOSED     = 1;
    static const int INFECTED    = 2;
    static const int REMOVED     = 3;
    static const int DECEASED    = 4;

public:

    ModelSEIRD() {};

    ModelSEIRD(
        ModelSEIRD<TSeq> & model,
        std::string vname,
        epiworld_double prevalence,
        epiworld_double transmission_rate,
        epiworld_double avg_incubation_days,
        epiworld_double recovery_rate,
        epiworld_double death_rate
    );

    ModelSEIRD(
        std::string vname,
        epiworld_double prevalence,
        epiworld_double transmission_rate,
        epiworld_double avg_incubation_days,
        epiworld_double recovery_rate,
        epiworld_double death_rate
    );
    
    epiworld::UpdateFun<TSeq> update_susceptible = [](
      epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
    ) -> void
    {
      
      // Sampling how many individuals
      int ndraw = m->rbinom();
      
      if (ndraw == 0)
        return;
      
      // Drawing from the set
      int nviruses_tmp = 0;
      for (int i = 0; i < ndraw; ++i)
      {
        // Now selecting who is transmitting the disease
        int which = static_cast<int>(
          std::floor(m->size() * m->runif())
        );
        
        /* There is a bug in which runif() returns 1.0. It is rare, but
         * we saw it here. See the Notes section in the C++ manual
         * https://en.cppreference.com/mwiki/index.php?title=cpp/numeric/random/uniform_real_distribution&oldid=133329
         * And the reported bug in GCC:
         * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63176
         * 
         */
        if (which == static_cast<int>(m->size()))
          --which;
        
        // Can't sample itself
        if (which == static_cast<int>(p->get_id()))
          continue;
        
        // If the neighbor is infected, then proceed
        auto & neighbor = m->get_agents()[which];
        if (neighbor.get_state() == ModelSEIRD<TSeq>::INFECTED)
        {
          
          for (const VirusPtr<TSeq> & v : neighbor.get_viruses()) 
          { 
            
          #ifdef EPI_DEBUG
            if (nviruses_tmp >= static_cast<int>(m->array_virus_tmp.size()))
              throw std::logic_error("Trying to add an extra element to a temporal array outside of the range.");
          #endif
            
            /* And it is a function of susceptibility_reduction as well */ 
            m->array_double_tmp[nviruses_tmp] =
            (1.0 - p->get_susceptibility_reduction(v, m)) * 
            v->get_prob_infecting(m) * 
            (1.0 - neighbor.get_transmission_reduction(v, m)) 
              ; 
            
            m->array_virus_tmp[nviruses_tmp++] = &(*v);
            
          } 
          
        }
      }
      
      // No virus to compute
      if (nviruses_tmp == 0u)
        return;
      
      // Running the roulette
      int which = roulette(nviruses_tmp, m);
      
      if (which < 0)
        return;
      
      p->add_virus(
          *m->array_virus_tmp[which],
                             m,
                             ModelSEIRD<TSeq>::EXPOSED
      );
      
      return; 
      
    };
      

      epiworld::UpdateFun<TSeq> update_infected = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
      ) -> void {
        
        auto state = p->get_state();
        
        if (state == ModelSEIRD<TSeq>::INFECTED)
        {
          
          
          // Odd: Die, Even: Recover
          epiworld_fast_uint n_events = 0u;
          for (const auto & v : p->get_viruses())
          {
            
            // Recover
            m->array_double_tmp[n_events++] = 
              1.0 - (1.0 - v->get_prob_recovery(m)) * (1.0 - p->get_recovery_enhancer(v, m)); 
            
          }
          
          #ifdef EPI_DEBUG
          if (n_events == 0u)
          {
            printf_epiworld(
              "[epi-debug] agent %i has 0 possible events!!\n",
              static_cast<int>(p->get_id())
            );
            throw std::logic_error("Zero events in exposed.");
          }
          #else
          if (n_events == 0u)
            return;
          #endif
          
          
          // Running the roulette
          int which = roulette(n_events, m);
          
          if (which < 0)
            return;
          
          // Which roulette happen?
          size_t which_v = std::floor(which / 2);
          p->rm_virus(which_v, m);
          
          return ;
          
        } else
          throw std::logic_error("This function can only be applied to infected individuals. (SEIRD)") ;
        
        return;
        
      };

};


template<typename TSeq>
inline ModelSEIRD<TSeq>::ModelSEIRD(
    ModelSEIRD<TSeq> & model,
    std::string vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate,
    epiworld_double death_rate
    )
{

    // Adding statuses
    model.add_state("Susceptible", epiworld::default_update_susceptible<TSeq>);
    model.add_state("Exposed", model.update_exposed_seir);
    model.add_state("Infected", model.update_infected);
    model.add_state("Removed");
    model.add_state("Deceased");

    // Setting up parameters
    model.add_param(transmission_rate, "Transmission rate");
    model.add_param(avg_incubation_days, "Incubation days");
    model.add_param(recovery_rate, "Recovery rate");
    model.add_param(death_rate, "Death rate");

    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname);
    virus.set_state(ModelSEIRD<TSeq>::EXPOSED, ModelSEIRD<TSeq>::REMOVED, ModelSEIRD<TSeq>::DECEASED);

    virus.set_prob_infecting(&model("Transmission rate"));
    virus.set_incubation(&model("Incubation days"));
    virus.set_prob_death(&model("Death rate"));
    
    // Adding the tool and the virus
    model.add_virus(virus, prevalence);
    
    model.set_name("Susceptible-Exposed-Infected-Removed-Deceased (SEIRD)");

    return;
   
}

template<typename TSeq>
inline ModelSEIRD<TSeq>::ModelSEIRD(
    std::string vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate,
    epiworld_double death_rate
    )
{

    ModelSEIRD<TSeq>(
        *this,
        vname,
        prevalence,
        transmission_rate,
        avg_incubation_days,
        recovery_rate,
        death_rate
        );

    return;

}



#endif
