#ifndef EPIWORLD_MODELS_SIRCONNECTED_HPP 
#define EPIWORLD_MODELS_SIRCONNECTED_HPP

template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSIRCONN : public epiworld::Model<TSeq>
{
private:
    static const int SUSCEPTIBLE = 0;
    static const int INFECTED    = 1;
    static const int RECOVERED   = 2;

public:

    ModelSIRCONN() {
        
        tracked_agents_infected.reserve(1e4);
        tracked_agents_infected_next.reserve(1e4);

    };

    ModelSIRCONN(
        ModelSIRCONN<TSeq> & model,
        std::string vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double reproductive_number,
        epiworld_double prob_transmission,
        epiworld_double prob_recovery
    );

    ModelSIRCONN(
        std::string vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double reproductive_number,
        epiworld_double prob_transmission,
        epiworld_double prob_recovery
    );

    // Tracking who is infected and who is not
    std::vector< epiworld::Agent<TSeq>* > tracked_agents_infected = {};
    std::vector< epiworld::Agent<TSeq>* > tracked_agents_infected_next = {};

    bool tracked_started = false;
    int tracked_ninfected = 0;
    int tracked_ninfected_next = 0;
    epiworld_double tracked_current_infect_prob = 0.0;

};


/**
 * @brief Template for a Susceptible-Infected-Removed (SIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param prevalence Initial prevalence (proportion)
 * @param reproductive_number Reproductive number (beta)
 * @param prob_transmission Probability of transmission
 * @param prob_recovery Probability of recovery
 */
template<typename TSeq>
inline ModelSIRCONN<TSeq>::ModelSIRCONN(
    ModelSIRCONN<TSeq> & model,
    std::string vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double reproductive_number,
    epiworld_double prob_transmission,
    epiworld_double prob_recovery
    // epiworld_double prob_reinfection
    )
{

    auto * _tracked_started = &model.tracked_started;
    auto * _tracked_ninfected = &model.tracked_ninfected;
    auto * _tracked_ninfected_next = &model.tracked_ninfected_next;
    auto * _tracked_current_infect_prob = &model.tracked_current_infect_prob;
    auto * _tracked_agents_infected = &model.tracked_agents_infected;
    auto * _tracked_agents_infected_next = &model.tracked_agents_infected_next;

    std::function<void(epiworld::Model<TSeq> * m)> tracked_agents_check_init = [
            _tracked_started,
            _tracked_agents_infected,
            _tracked_ninfected,
            _tracked_current_infect_prob
        ](epiworld::Model<TSeq> * m) -> void
        {

            if (*_tracked_started)
                return;

            /* Checking first if it hasn't  */ 
            if (!*_tracked_started) 
            { 
                
                /* Listing who is infected */ 
                for (auto & p : *(m->get_agents()))
                {
                    if (p.get_status() == ModelSIRCONN<TSeq>::INFECTED)
                    {
                    
                        _tracked_agents_infected->push_back(&p);
                        *_tracked_ninfected = *_tracked_ninfected + 1;
                    
                    }
                }

                for (auto & p: *_tracked_agents_infected)
                {
                    if (p->get_n_viruses() == 0)
                        throw std::logic_error("Cannot be infected and have no viruses.");
                }
                
                *_tracked_started = true;

                // Computing infection probability
                *_tracked_current_infect_prob =  1.0 - std::pow(
                    1.0 - (*m->p0) * (*m->p1) / m->size(),
                    *_tracked_ninfected
                );
                
            }

        };

    epiworld::UpdateFun<TSeq> update_susceptible = 
        [
            tracked_agents_check_init,
            _tracked_ninfected,
            _tracked_current_infect_prob,
            _tracked_agents_infected_next,
            _tracked_ninfected_next,
            _tracked_agents_infected
        ](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void
        {

            tracked_agents_check_init(m);

            // No infected individual?
            if (*_tracked_ninfected == 0)
                return;

            if (m->runif() < *_tracked_current_infect_prob)
            {

                // Adding the individual to the queue
                _tracked_agents_infected_next->push_back(p);
                *_tracked_ninfected_next = *_tracked_ninfected_next + 1;

                // Now selecting who is transmitting the disease
                epiworld_fast_uint which = static_cast<epiworld_fast_uint>(
                    std::floor(*_tracked_ninfected * m->runif())
                );

                // Infecting the individual
                p->add_virus(
                    _tracked_agents_infected->operator[](which)->get_virus(0u)
                    ); 

                return;

            }

            return;

        };

    epiworld::UpdateFun<TSeq> update_infected = 
        [
            tracked_agents_check_init,
            _tracked_ninfected_next,
            _tracked_agents_infected_next
        ](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void
        {

            tracked_agents_check_init(m);

            // Is recovering
            if (m->runif() < (*m->p2))
            {

                *_tracked_ninfected_next -= 1;
                epiworld::VirusPtr<int> v = p->get_virus(0u);
                p->rm_virus(0);
                return;

            }

            // Will be present next
            _tracked_agents_infected_next->push_back(p);

            return;

        };

    epiworld::GlobalFun<TSeq> global_accounting = 
        [
            _tracked_started,
            _tracked_agents_infected,
            _tracked_agents_infected_next,
            _tracked_ninfected,
            _tracked_ninfected_next,
            _tracked_current_infect_prob
        ](epiworld::Model<TSeq> * m) -> void
        {

            // On the last day, also reset tracked agents and
            // set the initialized value to false
            if (static_cast<epiworld_fast_uint>(m->today()) == (m->get_ndays() - 1))
            {

                *_tracked_started = false;
                _tracked_agents_infected->clear();
                _tracked_agents_infected_next->clear();
                *_tracked_ninfected = 0;
                *_tracked_ninfected_next = 0;    
                *_tracked_current_infect_prob = 0.0;

                return;
            }

            std::swap(*_tracked_agents_infected, *_tracked_agents_infected_next);
            _tracked_agents_infected_next->clear();

            *_tracked_ninfected += *_tracked_ninfected_next;
            *_tracked_ninfected_next = 0;

            *_tracked_current_infect_prob = 1.0 - std::pow(
                1.0 - (*m->p0) * (*m->p1) / m->size(),
                *_tracked_ninfected
                );

        };

    // Status
    model.add_status("Susceptible", update_susceptible);
    model.add_status("Infected", update_infected);
    model.add_status("Recovered");

    // Setting up parameters
    model.add_param(reproductive_number, "Beta");
    model.add_param(prob_transmission, "Prob. Transmission");
    model.add_param(prob_recovery, "Prob. Recovery");
    // model.add_param(prob_reinfection, "Prob. Reinfection");
    
    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname);
    virus.set_status(1, 2, 2);

    model.add_virus(virus, prevalence);

    // Adding updating function
    model.add_global_action(global_accounting, -1);

    model.queuing_off(); // No queuing need

    model.agents_empty_graph(n);

    model.set_name("Susceptible-Infected-Removed (SIR) (connected)");

    return;

}

template<typename TSeq>
inline ModelSIRCONN<TSeq>::ModelSIRCONN(
    std::string vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double reproductive_number,
    epiworld_double prob_transmission,
    epiworld_double prob_recovery
    )
{

    ModelSIRCONN(
        *this,
        vname,
        n,
        prevalence,
        reproductive_number,
        prob_transmission,
        prob_recovery
    );

    return;

}


#endif
