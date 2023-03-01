#ifndef EPIWORLD_MODELS_SEIRCONNECTED_HPP
#define EPIWORLD_MODELS_SEIRCONNECTED_HPP

template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSEIRCONN : public epiworld::Model<TSeq> 
{
public:

    static const int SUSCEPTIBLE = 0;
    static const int EXPOSED     = 1;
    static const int INFECTED    = 2;
    static const int RECOVERED   = 3;


    ModelSEIRCONN() {

        tracked_agents_infected.reserve(1e4);
        tracked_agents_infected_next.reserve(1e4);
        
    };

    ModelSEIRCONN(
        ModelSEIRCONN<TSeq> & model,
        std::string vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double reproductive_number,
        epiworld_double prob_transmission,
        epiworld_double incubation_days,
        epiworld_double prob_recovery
    );
    
    ModelSEIRCONN(
        std::string vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double reproductive_number,
        epiworld_double prob_transmission,
        epiworld_double incubation_days,
        epiworld_double prob_recovery
    );

    // Tracking who is infected and who is not
    std::vector< epiworld::Agent<>* > tracked_agents_infected = {};
    std::vector< epiworld::Agent<>* > tracked_agents_infected_next = {};

    bool tracked_started = false;
    int tracked_ninfected = 0;
    int tracked_ninfected_next = 0;

    void run(
        epiworld_fast_uint ndays,
        int seed = -1
    );

    Model<TSeq> * clone_ptr();

};

template<typename TSeq>
inline void ModelSEIRCONN<TSeq>::run(
    epiworld_fast_uint ndays,
    int seed
)
{

    tracked_agents_infected.clear();
    tracked_agents_infected_next.clear();

    tracked_started = false;
    tracked_ninfected = 0;
    tracked_ninfected_next = 0;

    Model<TSeq>::run(ndays, seed);

}

template<typename TSeq>
inline Model<TSeq> * ModelSEIRCONN<TSeq>::clone_ptr()
{
    
    ModelSEIRCONN<TSeq> * ptr = new ModelSEIRCONN<TSeq>(
        *dynamic_cast<const ModelSEIRCONN<TSeq>*>(this)
        );

    return dynamic_cast< Model<TSeq> *>(ptr);

}

/**
 * @brief Template for a Susceptible-Exposed-Infected-Removed (SEIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param prevalence Initial prevalence (proportion)
 * @param reproductive_number Reproductive number (beta)
 * @param prob_transmission Probability of transmission
 * @param prob_recovery Probability of recovery
 */
template<typename TSeq>
inline ModelSEIRCONN<TSeq>::ModelSEIRCONN(
    ModelSEIRCONN<TSeq> & model,
    std::string vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double reproductive_number,
    epiworld_double prob_transmission,
    epiworld_double incubation_days,
    epiworld_double prob_recovery
    // epiworld_double prob_reinfection
    )
{

    std::function<void(ModelSEIRCONN<TSeq> *)> tracked_agents_check_init = 
    [](ModelSEIRCONN<TSeq> * m) 
        {

            /* Checking first if it hasn't  */ 
            if (m->tracked_started)
                return;

            /* Listing who is infected */ 
            for (auto & p : m->get_agents())
            {
                if (p.get_status() == ModelSEIRCONN<TSeq>::INFECTED)
                {
                
                    m->tracked_agents_infected.push_back(&p);
                    m->tracked_ninfected++;
                
                }
            }

            for (auto & p: m->tracked_agents_infected)
            {
                if (p->get_n_viruses() == 0)
                    throw std::logic_error("Cannot be infected and have no viruses.");
            }
            
            m->tracked_started = true;
                
        };

    epiworld::UpdateFun<TSeq> update_susceptible = 
    [tracked_agents_check_init](epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m) -> void
        {

            // Getting the right type
            ModelSEIRCONN<TSeq> * _m = dynamic_cast<ModelSEIRCONN<TSeq>*>(m);

            tracked_agents_check_init(_m);

            // No infected individual?
            if (_m->tracked_ninfected == 0)
                return;

            // Computing probability of contagion
            // P(infected) = 1 - (1 - beta/Pop * ptransmit) ^ ninfected
            epiworld_double prob_infect = 1.0 - std::pow(
                1.0 - (m->par("Beta")) * (m->par("Prob. Transmission")) / m->size(),
                _m->tracked_ninfected
                );

            if (m->runif() < prob_infect)
            {

                // Now selecting who is transmitting the disease
                epiworld_fast_uint which = static_cast<epiworld_fast_uint>(
                    std::floor(_m->tracked_ninfected * m->runif())
                );

                // Infecting the individual
                #ifdef EPI_DEBUG
                if (_m->tracked_agents_infected[which]->get_n_viruses() == 0)
                {

                    printf_epiworld("[epi-debug] date: %i\n", m->today());
                    printf_epiworld("[epi-debug] sim#: %i\n", m->get_n_replicates());

                    throw std::logic_error(
                        "[epi-debug] The agent " + std::to_string(which) + " has no "+
                        "virus to share. The agent's status is: " +
                        std::to_string(_m->tracked_agents_infected[which]->get_status())
                    );
                }
                #endif
                p->add_virus(
                    _m->tracked_agents_infected[which]->get_virus(0u),
                    m,
                    ModelSEIRCONN<TSeq>::EXPOSED
                    ); 

                return;

            }

            return;

        };

    epiworld::UpdateFun<TSeq> update_infected = 
    [tracked_agents_check_init](epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m) -> void
        {

            // Getting the right type
            ModelSEIRCONN<TSeq> * _m = dynamic_cast<ModelSEIRCONN<TSeq>*>(m);

            tracked_agents_check_init(_m);

            auto status = p->get_status();

            if (status == ModelSEIRCONN<TSeq>::EXPOSED)
            {

                // Does the agent become infected?
                if (m->runif() < 1.0/(m->par("Avg. Incubation days")))
                {
                    // Adding the individual to the queue
                    _m->tracked_agents_infected_next.push_back(p);
                    _m->tracked_ninfected_next++;

                    p->change_status(m, ModelSEIRCONN<TSeq>::INFECTED);

                    return;

                }


            } else if (status == ModelSEIRCONN<TSeq>::INFECTED)
            {

                if (m->runif() < (m->par("Prob. Recovery")))
                {

                    _m->tracked_ninfected_next--;
                    p->rm_virus(0, m);
                    return;

                }

                _m->tracked_agents_infected_next.push_back(p);

            } 

            return;

        };

    epiworld::GlobalFun<TSeq> global_accounting = 
    [](epiworld::Model<TSeq>* m) -> void
        {

            // Getting the right type
            ModelSEIRCONN<TSeq> * _m = dynamic_cast<ModelSEIRCONN<TSeq>*>(m);

            // On the last day, also reset tracked agents and
            // set the initialized value to false
            if (static_cast<epiworld_fast_uint>(m->today()) == (m->get_ndays() - 1))
            {

                _m->tracked_started = false;
                _m->tracked_agents_infected.clear();
                _m->tracked_agents_infected_next.clear();
                _m->tracked_ninfected = 0;
                _m->tracked_ninfected_next = 0;    

                return;
            }

            std::swap(_m->tracked_agents_infected, _m->tracked_agents_infected_next);
            _m->tracked_agents_infected_next.clear();

            _m->tracked_ninfected += _m->tracked_ninfected_next;
            _m->tracked_ninfected_next = 0;

        };

    // Setting up parameters
    model.add_param(reproductive_number, "Beta");
    model.add_param(prob_transmission, "Prob. Transmission");
    model.add_param(prob_recovery, "Prob. Recovery");
    model.add_param(incubation_days, "Avg. Incubation days");
    
    // Status
    model.add_status("Susceptible", update_susceptible);
    model.add_status("Exposed", update_infected);
    model.add_status("Infected", update_infected);
    model.add_status("Recovered");


    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname);
    virus.set_status(ModelSEIRCONN<TSeq>::EXPOSED, ModelSEIRCONN<TSeq>::RECOVERED, ModelSEIRCONN<TSeq>::RECOVERED);
    model.add_virus(virus, prevalence);

    // Adding updating function
    model.add_global_action(global_accounting, -1);

    model.queuing_off(); // No queuing need

    // Adding the empty population
    model.agents_empty_graph(n);

    model.set_name("Susceptible-Exposed-Infected-Removed (SEIR) (connected)");

    return;

}

template<typename TSeq>
inline ModelSEIRCONN<TSeq>::ModelSEIRCONN(
    std::string vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double reproductive_number,
    epiworld_double prob_transmission,
    epiworld_double incubation_days,
    epiworld_double prob_recovery
    )
{

    ModelSEIRCONN(
        *this,
        vname,
        n,
        prevalence,
        reproductive_number,
        prob_transmission,
        incubation_days,
        prob_recovery
    );

    return;

}

#endif
