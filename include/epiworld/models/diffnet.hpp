#include "../epiworld.hpp"
#ifndef EPIWORLD_DIFFNET_H 
#define EPIWORLD_DIFFNET_H

/**
 * @brief Template for a Network Diffusion Model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param initial_prevalence epiworld_double Initial prevalence
 * @param initial_efficacy epiworld_double Initial susceptibility_reduction of the immune system
 * @param initial_recovery epiworld_double Initial recovery rate of the immune system
 */
template<typename TSeq = int>
class ModelDiffNet : public epiworld::Model<TSeq>
{
private:
public:

    ModelDiffNet() {};

    ModelDiffNet(
        ModelDiffNet<TSeq> & model,
        std::string innovation_name,
        epiworld_double prevalence,
        epiworld_double prob_adopt,
        bool normalize_exposure,
        std::vector< double > params
    );

    ModelDiffNet(
        std::string innovation_name,
        epiworld_double prevalence,
        epiworld_double prob_adopt,
        bool normalize_exposure,
        std::vector< double > params
    );
    
    static const int NONADOPTER = 0;
    static const int ADOPTER    = 1;

    bool normalize_exposure = true;
    std::vector< double > params;
};

template<typename TSeq>
inline ModelDiffNet<TSeq>::ModelDiffNet(
    ModelDiffNet<TSeq> & model,
    std::string innovation_name,
    epiworld_double prevalence,
    epiworld_double prob_adopt,
    bool normalize_exposure,
    std::vector< double > params
    )
{

    epiworld::UpdateFun<TSeq> update_non_adopters[](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
    ) -> void {

        // Measuring exposure
        // If the neighbor is infected, then proceed
        size_t nvariants = m->get_n_variants();
        std::vector< std::vector< Virus<TSeq>* > > innovations(nvariants, {});
        std::vector< std::vector< double > > innovations_prob(nvariants, {});
        std::vector< double > exposure(nvariants, 0.0);

        ModelDiffNet<TSeq> * diffmodel = dynamic_cast<ModelDiffNet<TSeq>*>(m);

        Agent<TSeq> & agent = *p;

        // For each one of the possible innovations, we have to compute
        // the adoption probability, which is a function of exposure
        for (auto & neighbor: agent.get_neighbors())
        {

            if (neighbor->get_state() == ModelDiffNet<TSeq>::ADOPTER)
            {

                for (const VirusPtr<TSeq> & v : neighbor->get_viruses()) 
                { 
                        
                    /* And it is a function of susceptibility_reduction as well */ 
                    double p_i =
                        (1.0 - agent.get_susceptibility_reduction(v, m)) * 
                        (1.0 - agent.get_transmission_reduction(v, m)) 
                        ; 
                
                    size_t vid = v->get_id();
                    innovations[vid].push_back(&(*v));
                    innovations_prob[vid].push_back(p_i);
                    exposure[vid] += p_i;
                    
                } 

            }

        }

        // Computing probability of adoption
        size_t nparams = diffmodel->params.size();
        for (size_t i = 0u; i < nvariants; ++i)
        {

            if (diffmodel->normalize_exposure)
                exposure.at(i) /= agent.get_n_neighbors();

            for (size_t j = 0u; j < nparams; ++j)
                exposure.at(i) += agent(j) * diffmodel->params.at(j);

            // Baseline probability of adoption
            double p = m->viruses[i]->get_prob_infecting(m);
            exposure.at(i) += std::log(p) - std::log(1.0 - p);

            // Computing as log
            exposure.at(i) = 1.0/(1.0 + std::exp(-exposure.at(i)));

        }

        // Running the roulette to see is an innovation is adopted
        int which = roulette(exposure, m);

        // No innovation was adopted
        if (which < 0)
            return;

        // Innovation -which- will be adopted. Need to figure out who
        // transmitted it
        for (auto & inn: innovations_prob.at(which))
            inn = 1.0/(1.0 + std::exp(inn));

        int who = roulette(innovations_prob, m);

        // If no one did, then it means that the agent will adopt it
        // directly from the environment...
        if (who < 0)
        {
            agent.add_virus(
                *m->viruses[which], 
                m,
                ModelDiffNet::ADOPTER
            );

            return;
        } 
        else // Otherwise, it will be adopted from another agent
        {

            agent.add_virus(
                *innovations.at(which).at(who),
                m,
                ModelDiffNet::ADOPTER
            )

        }

        return;

        };

    // Adding statuses
    model.add_state("Non adopters", update_non_adopters);
    model.add_state("Adopters");

    // Adding parameters
    std::string parname = std::string("Prob. Adopting ") + innovation_name;
    model.add_param(prob_adopt, parname);

    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> innovation(innovation_name);
    innovation.set_state(1,2,2);
    
    innovation.set_prob_infecting(&model(parname));
    
    model.add_virus(innovation, prevalence);

    model.set_name("Diffusion of Innovations");

    return;
   
}

template<typename TSeq>
inline ModelDiffNet<TSeq>::ModelDiffNet(
    std::string vname,
    epiworld_double prevalence,
    epiworld_double infectiousness,
    epiworld_double recovery
    )
{

    ModelDiffNet<TSeq>(
        *this,
        vname,
        prevalence,
        infectiousness,
        recovery
        );

    return;

}

#endif
