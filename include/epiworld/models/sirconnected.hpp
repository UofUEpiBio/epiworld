#ifndef EPIWORLD_SIR_H 
#define EPIWORLD_SIR_H

enum SIRCONSTATUS {
    SUSCEPTIBLE,
    INFECTED,
    RECOVERED
};

// Tracking who is infected and who is not
std::vector< epiworld::Person<>* > tracked_agents_infected(0u);
std::vector< epiworld::Person<>* > tracked_agents_infected_next(0u);

bool tracked_started = false;
int tracked_ninfected = 0;
int tracked_ninfected_next = 0;

template<typename TSeq>
inline void tracked_agents_check_init(epiworld::Model<TSeq> * m) 
{

    if (tracked_started)
        return;

    /* Checking first if it hasn't  */ 
    if (!tracked_started) 
    { 
        
        /* Listing who is infected */ 
        for (auto & p : *(m->get_population()))
        {
            if (p.get_status() == SIRCONSTATUS::INFECTED)
            {
            
                tracked_agents_infected.push_back(&p);
                tracked_ninfected++;
            
            }
        }

        for (auto & p: tracked_agents_infected)
        {
            if (p->get_viruses().size() == 0)
                throw std::logic_error("Cannot be infected and have no viruses.");
        }
        
        tracked_started = true;
        
    }

}

EPI_NEW_UPDATEFUN(update_susceptible, bool)
{

    tracked_agents_check_init(m);

    // No infected individual?
    if (tracked_ninfected == 0)
        return p->get_status();

    // Computing probability of contagion
    // P(infected) = 1 - (1 - beta/Pop * ptransmit) ^ ninfected
    epiworld_double prob_infect = 1.0 - std::pow(
        1.0 - (*m->p0) * (*m->p1) / m->size(),
        tracked_ninfected
        );

    if (m->runif() < prob_infect)
    {

        // Adding the individual to the queue
        tracked_agents_infected_next.push_back(p);
        tracked_ninfected_next++;

        // Now selecting who is transmitting the disease
        epiworld_fast_uint which = static_cast<epiworld_fast_uint>(
            std::floor(tracked_ninfected * m->runif())
        );

        // Infecting the individual
        p->add_virus(
            &tracked_agents_infected[which]->get_virus(0u)
            ); 

        return SIRCONSTATUS::INFECTED;

    }

    return p->get_status();

}

EPI_NEW_UPDATEFUN(update_infected, bool)
{

    tracked_agents_check_init(m);

    // Is recovering
    if (m->runif() < (*m->p2))
    {

        tracked_ninfected_next--;
        epiworld::Virus<> * v = &p->get_virus(0u);
        p->rm_virus(v);
        return SIRCONSTATUS::RECOVERED;

    }

    // Will be present next
    tracked_agents_infected_next.push_back(p);

    return p->get_status();

}

EPI_NEW_GLOBALFUN(global_accounting, bool)
{

    // On the last day, also reset tracked agents and
    // set the initialized value to false
    if (static_cast<unsigned int>(m->today()) == (m->get_ndays() - 1))
    {

        tracked_started = false;
        tracked_agents_infected.clear();
        tracked_agents_infected_next.clear();
        tracked_ninfected = 0;
        tracked_ninfected_next = 0;    

        return;
    }

    std::swap(tracked_agents_infected, tracked_agents_infected_next);
    tracked_agents_infected_next.clear();

    tracked_ninfected += tracked_ninfected_next;
    tracked_ninfected_next = 0;

}



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
inline void set_up_sir_connected(
    epiworld::Model<TSeq> & model,
    std::string vname,
    epiworld_double prevalence,
    epiworld_double reproductive_number,
    epiworld_double prob_transmission,
    epiworld_double prob_recovery
    // epiworld_double prob_reinfection
    )
{

    // Setting up parameters
    model.add_param(reproductive_number, "Beta");
    model.add_param(prob_transmission, "Prob. Transmission");
    model.add_param(prob_recovery, "Prob. Recovery");
    // model.add_param(prob_reinfection, "Prob. Reinfection");
    
    // Status
    std::vector< epiworld_fast_uint > new_status =
    {
        SIRCONSTATUS::SUSCEPTIBLE, SIRCONSTATUS::INFECTED,
        SIRCONSTATUS::RECOVERED
    };

    model.reset_status_codes(
        new_status,
        {"susceptible", "infected", "recovered"},
        false
    );

    model.set_update_exposed(update_infected);
    model.set_update_susceptible(update_susceptible);

    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname);
    model.add_virus(virus, prevalence);

    // Adding updating function
    model.add_global_action(global_accounting, -1);

    model.queuing_off(); // No queuing need

    return;

}

#endif
