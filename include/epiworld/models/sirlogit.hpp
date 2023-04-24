// #include "../../epiworld.hpp"

#ifndef EPIWORLD_MODELS_SIRLOGIT_HPP 
#define EPIWORLD_MODELS_SIRLOGIT_HPP

template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSIRLogit : public epiworld::Model<TSeq>
{
private:
    static const int SUSCEPTIBLE = 0;
    static const int INFECTED    = 1;
    static const int RECOVERED   = 2;

public:

    ModelSIRLogit() {};

    /**
      * @param vname Name of the virus.
      * @param coefs_infect Double ptr. Infection coefficients.
      * @param coefs_recover Double ptr. Recovery coefficients.
      * @param ncoef_infect Unsigned int. Number of infection coefficients.
      * @param ncoef_recover Unsigned int. Number of recovery coefficients.
      * @param coef_infect_cols Vector<unsigned int>. Ids of infection vars.
      * @param coef_recover_cols Vector<unsigned int>. Ids of recover vars.
    */
    ModelSIRLogit(
        ModelSIRLogit<TSeq> & model,
        std::string vname,
        double * data,
        size_t ncols,
        double * coefs_infect,
        double * coefs_recover,
        size_t ncoef_infect,
        size_t ncoef_recover,
        std::vector< size_t > coef_infect_cols,
        std::vector< size_t > coef_recover_cols,
        epiworld_double prevalence
    );

    ModelSIRLogit(
        std::string vname,
        double * data,
        size_t ncols,
        double * coefs_infect,
        double * coefs_recover,
        size_t ncoef_infect,
        size_t ncoef_recover,
        std::vector< size_t > coef_infect_cols,
        std::vector< size_t > coef_recover_cols,
        epiworld_double prevalence
    );

    void run(
        epiworld_fast_uint ndays,
        int seed = -1
    );

    Model<TSeq> * clone_ptr();

    bool tracked_started = false;
    
    double * coefs_infect = nullptr;
    double * coefs_recover = nullptr;
    size_t ncoef_infect = 0u;
    size_t ncoef_recover = 0u;
    std::vector< size_t > coef_infect_cols;
    std::vector< size_t > coef_recover_cols;

    double prod(
        double * x,
        double * coefs_infect,
        size_t k
    );

};

template<typename TSeq>
inline double ModelSIRLogit<TSeq>::prod(
    double * x, 
    double * coefs_infect,
    size_t k
) {

    double res = 0.0;

#pragma omp simd reduction(+:res)
    for (size_t i = 0u; i < k ++i)
        res += (*x)[i] * (*coefs_infect)[i];

    
    return res;

}

template<typename TSeq>
inline void ModelSIRLogit<TSeq>::run(
    epiworld_fast_uint ndays,
    int seed
)
{

    Model<TSeq>::run(ndays, seed);

}

template<typename TSeq>
inline Model<TSeq> * ModelSIRLogit<TSeq>::clone_ptr()
{
    
    ModelSIRLogit<TSeq> * ptr = new ModelSIRLogit<TSeq>(
        *dynamic_cast<const ModelSIRLogit<TSeq>*>(this)
        );

    return dynamic_cast< Model<TSeq> *>(ptr);

}

/**
 * @brief Template for a Susceptible-Infected-Removed (SIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param prevalence Initial prevalence (proportion)
 * @param contact_rate Average number of contacts (interactions) per step.
 * @param prob_transmission Probability of transmission
 * @param prob_recovery Probability of recovery
 */
template<typename TSeq>
inline ModelSIRLogit<TSeq>::ModelSIRLogit(
    ModelSIRLogit<TSeq> & model,
    std::string vname,
    double * data,
    size_t ncols,
    double * coefs_infect,
    double * coefs_recover,
    size_t ncoef_infect,
    size_t ncoef_recover,
    std::vector< size_t > coef_infect_cols,
    std::vector< size_t > coef_recover_cols,
    epiworld_double prevalence
    )
{

    // Saving the variables
    model.set_agents_data(
        data, ncols
    );

    std::function<void(ModelSIRLogit<TSeq> * m)> check_init = [](
        ModelSIRLogit<TSeq> * m
        ) -> void
        {

            /* Checking first if it hasn't  */ 
            if (m->tracked_started)
                return;
    
            /* Checking attributes */ 
            if (m->ncoef_infect != (m->coef_infect_cols.size() + 1u))
                throw std::logic_error(
                    "The number of coefficients (infection) doesn't match the number of features. It must be as many features of the agents plus 1 (exposure.)"
                    );

            if (m->ncoef_recover != m->coef_recover_cols.size())
                throw std::logic_error(
                    "The number of coefficients (recovery) doesn't match the number of features. It must be as many features of the agents."
                    );
            
            m->tracked_started = true;            

        };

    epiworld::UpdateFun<TSeq> update_susceptible = [
        check_init
    ](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void
        {

            // Getting the right type
            ModelSIRLogit<TSeq> * _m = dynamic_cast<ModelSIRLogit<TSeq>*>(m);

            check_init(_m);

            // Exposure coefficient
            const double coef_exposure = *_m->coefs_infect;

            // This computes the prob of getting any neighbor variant
            size_t nvariants_tmp = 0u;
            for (auto & neighbor: p->get_neighbors()) 
            {

                // Computing baseline probabilities
                double baseline = prod(
                    x,
                    _m->coefs_infect + 1u, // We skip the exposure
                    _m->ncoef_infect - 1u  // The first is for exposure
                );
                
                for (const VirusPtr<TSeq> & v : neighbor->get_viruses()) 
                { 

                    #ifdef EPI_DEBUG
                    if (nvariants_tmp >= m->array_virus_tmp.size())
                        throw std::logic_error("Trying to add an extra element to a temporal array outside of the range.");
                    #endif
                        
                    /* And it is a function of susceptibility_reduction as well */ 
                    m->array_double_tmp[nvariants_tmp] =
                        baseline +
                        (1.0 - p->get_susceptibility_reduction(v, m)) * 
                        v->get_prob_infecting(m) * 
                        (1.0 - neighbor->get_transmission_reduction(v, m))  *
                        coef_exposure
                        ; 

                    // Applying the plogis function
                    m->array_double_tmp[nvariants_tmp] = 1.0/
                        (1.0 + std::exp(-m->array_double_tmp[nvariants_tmp]))
                
                    m->array_virus_tmp[nvariants_tmp++] = &(*v);

                }

            }

            // No virus to compute
            if (nvariants_tmp == 0u)
                return;

            // Running the roulette
            int which = roulette(nvariants_tmp, m);

            if (which < 0)
                return;

            p->add_virus(*m->array_virus_tmp[which], m);

            return;

        };

    epiworld::UpdateFun<TSeq> update_infected = [
        check_init
    ](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void
        {

            // Getting the right type
            ModelSIRLogit<TSeq> * _m = dynamic_cast<ModelSIRLogit<TSeq>*>(m);

            check_init(_m);

            // Computing recovery probability once
            double prob    = 0.0;
            size_t id      = p->get_id();
            size_t nagents = m->size();
            #pragma omp simd reduction(+:prob)
            for (size_t i = 0u; i < _m->ncoef_recover; ++i)
            {
                prob +=
                    (*(
                        m->agents_data +
                        /* data is stored column-major */
                        (_m->coef_infect_cols[i] * nagents + id)
                    )) * _m->coefs_infect[i];

            }

            // Computing logis
            prob = 1.0/(1.0 + std::exp(-prob));

            if (prob > m->runif())
                p->rm_virus(0, m);
            
            return;

        };

    // Status
    model.add_state("Susceptible", update_susceptible);
    model.add_state("Infected", update_infected);
    model.add_state("Recovered");

    // Setting up parameters
    model.add_param(contact_rate, "Contact rate");
    model.add_param(prob_transmission, "Prob. Transmission");
    model.add_param(prob_recovery, "Prob. Recovery");
    // model.add_param(prob_reinfection, "Prob. Reinfection");
    
    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname);
    virus.set_state(1, 2, 2);

    model.add_virus(virus, prevalence);

    model.set_name("Susceptible-Infected-Removed (SIR) (logit)");

    return;

}

template<typename TSeq>
inline ModelSIRLogit<TSeq>::ModelSIRLogit(
    std::string vname,
    double * data,
    double * coefs_infect,
    double * coefs_recover,
    size_t ncoef_infect,
    size_t ncoef_recover,
    std::vector< size_t > coef_infect_cols,
    std::vector< size_t > coef_recover_cols,
    epiworld_double prevalence
    )
{

    ModelSIRLogit(
        *this,
        vname,
        data,
        coefs_infect,
        coefs_recover,
        ncoef_infect,
        ncoef_recover,
        coef_infect_cols,
        coef_recover_cols,
        prevalence
    );

    return;

}


#endif
