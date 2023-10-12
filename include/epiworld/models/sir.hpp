#ifndef EPIWORLD_SIR_H 
#define EPIWORLD_SIR_H

/**
 * @brief Template for a Susceptible-Infected-Removed (SIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param initial_prevalence epiworld_double Initial prevalence
 * @param initial_efficacy epiworld_double Initial susceptibility_reduction of the immune system
 * @param initial_recovery epiworld_double Initial recovery_rate rate of the immune system
 */
template<typename TSeq = int>
class ModelSIR : public epiworld::Model<TSeq>
{
public:

    ModelSIR() {};

    ModelSIR(
        ModelSIR<TSeq> & model,
        std::string vname,
        epiworld_double prevalence,
        epiworld_double transmission_rate,
        epiworld_double recovery_rate
    );

    ModelSIR(
        std::string vname,
        epiworld_double prevalence,
        epiworld_double transmission_rate,
        epiworld_double recovery_rate
    );

    void initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_
    );
    
};

template<typename TSeq>
inline ModelSIR<TSeq>::ModelSIR(
    ModelSIR<TSeq> & model,
    std::string vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double recovery_rate
    )
{

    // Adding statuses
    model.add_state("Susceptible", epiworld::default_update_susceptible<TSeq>);
    model.add_state("Infected", epiworld::default_update_exposed<TSeq>);
    model.add_state("Recovered");

    // Setting up parameters
    model.add_param(recovery_rate, "Recovery rate");
    model.add_param(transmission_rate, "Transmission rate");

    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname);
    virus.set_state(1,2,2);
    
    virus.set_prob_recovery(&model("Recovery rate"));
    virus.set_prob_infecting(&model("Transmission rate"));
    
    model.add_virus(virus, prevalence);

    model.set_name("Susceptible-Infected-Recovered (SIR)");

    return;
   
}

template<typename TSeq>
inline ModelSIR<TSeq>::ModelSIR(
    std::string vname,
    epiworld_double prevalence,
    epiworld_double transmission_rate,
    epiworld_double recovery_rate
    )
{

    ModelSIR<TSeq>(
        *this,
        vname,
        prevalence,
        transmission_rate,
        recovery_rate
        );

    return;

}

template<typename TSeq>
inline void ModelSIR<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > queue_
) {

    // Checking widths
    size_t nstates = this->get_states().size();
    if (proportions_.size() != nstates) {
        throw std::invalid_argument("The number of proportions must be equal to the number of states.");
    }

    // Proportions should add to 1 minus the virus, but this needs
    // to be managed on the fly once we know what is the population.
    // We just check for the second entry, which needs to be zero.
    if (proportions_[1] != 0.0) {
        throw std::invalid_argument("The second entry of the proportions must be zero.");
    }

    // Checking queue
    if (queue_.size() == 0u)
    {
       queue_ = {Queue<TSeq>::NoOne, Queue<TSeq>::NoOne, Queue<TSeq>::NoOne};
    } else if (queue_.size() != nstates) {
        throw std::invalid_argument("The number of queue must be equal to the number of states.");
    }

    // Capturing variables
    const auto * vpreval = &Model<TSeq>::prevalence_virus;
    const auto * vprop   = &Model<TSeq>::prevalence_virus_as_proportion;

    // Creating function
    std::function<void(epiworld::Model<TSeq>*)> fun =
    [proportions_, queue_, vpreval, vprop] (epiworld::Model<TSeq> * model) -> void {

        // Figuring out information about the viruses
        double tot = 0.0;
        double n   = static_cast<double>(model->size());
        for (size_t i = 0u; i < model->get_n_viruses(); ++i)
        {
            if ((*vprop)[i])
                tot += (*vpreval)[i];
            else
                tot += (*vpreval)[i] / n;
        }

        // Putting the total into context
        double tot_left = 1.0 - tot;
        double total_prop = std::accumulate(
            proportions_.begin(), proportions_.end(), 0.0
            );

        // Since susceptible and infected are "fixed,"
        // we only need to change recovered
        size_t nrecovered = proportions_[2] / total_prop * tot_left * n;
        
        epiworld::AgentsSample<TSeq> sample(
            *model,
            nrecovered,
            true
            );

        // Setting up the initial states
        for (auto & agent : sample)
            agent->change_state(model, 2, Queue<TSeq>::NoOne);
        
        return;

    };

    Model<TSeq>::initial_states_fun = fun;

    return;

}

#endif
