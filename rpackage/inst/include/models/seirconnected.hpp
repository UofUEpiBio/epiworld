#define EPI_DEBUG
#ifndef EPIWORLD_SEIR_H 
#define EPIWORLD_SEIR_H

enum SEIRCONSTATUS {
    SUSCEPTIBLE,
    EXPOSED,
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
            if (p.get_status() == SEIRCONSTATUS::INFECTED)
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

        // Now selecting who is transmitting the disease
        epiworld_fast_uint which = static_cast<epiworld_fast_uint>(
            std::floor(tracked_ninfected * m->runif())
        );

        // Infecting the individual
        #ifdef EPI_DEBUG
        if (tracked_agents_infected[which]->get_viruses().size() == 0)
        {

            printf_epiworld("[epiworld-debug] date: %i\n", m->today());
            printf_epiworld("[epiworld-debug] sim#: %i\n", m->get_n_replicates());

            throw std::logic_error(
                "[epiworld-debug] The agent " + std::to_string(which) + " has no "+
                "virus to share. The agent's status is: " +
                std::to_string(tracked_agents_infected[which]->get_status())
            );
        }
        #endif
        p->add_virus(
            &tracked_agents_infected[which]->get_virus(0u)
            ); 

        return SEIRCONSTATUS::EXPOSED;

    }

    return p->get_status();

}

EPI_NEW_UPDATEFUN(update_infected, bool)
{

    tracked_agents_check_init(m);
    auto status = p->get_status();

    if (status == SEIRCONSTATUS::EXPOSED)
    {

        // Does the agent become infected?
        if (m->runif() < 1.0/(*m->p3))
        {
            // Adding the individual to the queue
            tracked_agents_infected_next.push_back(p);
            tracked_ninfected_next++;

            return SEIRCONSTATUS::INFECTED;

        }


    } else if (status == SEIRCONSTATUS::INFECTED)
    {

        if (m->runif() < (*m->p2))
        {

            tracked_ninfected_next--;
            p->rm_virus(&p->get_virus(0u));
            return SEIRCONSTATUS::RECOVERED;

        }

        tracked_agents_infected_next.push_back(p);

    } 

    return status;

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
inline void set_up_seir_connected(
    epiworld::Model<TSeq> & model,
    std::string vname,
    epiworld_double prevalence,
    epiworld_double reproductive_number,
    epiworld_double prob_transmission,
    epiworld_double incubation_days,
    epiworld_double prob_recovery
    // epiworld_double prob_reinfection
    )
{

    // Setting up parameters
    model.add_param(reproductive_number, "Beta");
    model.add_param(prob_transmission, "Prob. Transmission");
    model.add_param(prob_recovery, "Prob. Recovery");
    model.add_param(incubation_days, "Avg. Incubation days");
    
    // Status
    std::vector< epiworld_fast_uint > new_status =
    {
        SEIRCONSTATUS::SUSCEPTIBLE, SEIRCONSTATUS::EXPOSED,
        SEIRCONSTATUS::RECOVERED
    };

    model.reset_status_codes(
        new_status,
        {"susceptible", "exposed", "recovered"},
        true
    );

    model.add_status_exposed(SEIRCONSTATUS::INFECTED, "infected");
    model.print_status_codes();
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
