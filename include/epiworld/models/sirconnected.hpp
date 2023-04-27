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
        
        // tracked_agents_infected.reserve(1e4);
        // tracked_agents_infected_next.reserve(1e4);

    };

    ModelSIRCONN(
        ModelSIRCONN<TSeq> & model,
        std::string vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double prob_transmission,
        epiworld_double prob_recovery
    );

    ModelSIRCONN(
        std::string vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double prob_transmission,
        epiworld_double prob_recovery
    );

    // Tracking who is infected and who is not
    // std::vector< epiworld::Agent<TSeq>* > tracked_agents_infected = {};
    // std::vector< epiworld::Agent<TSeq>* > tracked_agents_infected_next = {};
    // std::vector< epiworld_double >        tracked_agents_weight        = {};
    // std::vector< epiworld_double >        tracked_agents_weight_next   = {};

    // int tracked_ninfected = 0;
    // int tracked_ninfected_next = 0;
    // epiworld_double tracked_current_infect_prob = 0.0;

    void run(
        epiworld_fast_uint ndays,
        int seed = -1
    );
    
    void reset();

    Model<TSeq> * clone_ptr();


};

template<typename TSeq>
inline void ModelSIRCONN<TSeq>::run(
    epiworld_fast_uint ndays,
    int seed
)
{

    // tracked_agents_infected.clear();
    // tracked_agents_infected_next.clear();

    // tracked_ninfected = 0;
    // tracked_ninfected_next = 0;
    // tracked_current_infect_prob = 0.0;

    Model<TSeq>::run(ndays, seed);

}

template<typename TSeq>
inline void ModelSIRCONN<TSeq>::reset()
{

    Model<TSeq>::reset();

    // /* Listing who is infected */ 
    // for (auto & p : Model<TSeq>::get_agents())
    // {
    //     if (p.get_state() == ModelSIRCONN<TSeq>::INFECTED)
    //     {
        
    //         tracked_agents_infected.push_back(&p);
    //         tracked_ninfected++;
        
    //     }
    // }

    // for (auto & p: tracked_agents_infected)
    // {
    //     if (p->get_n_viruses() == 0)
    //         throw std::logic_error("Cannot be infected and have no viruses.");
    // }
    
    // // Computing infection probability
    // tracked_current_infect_prob =  1.0 - std::pow(
    //     1.0 - (Model<TSeq>::par("Contact rate")) * (Model<TSeq>::par("Prob. Transmission")) / Model<TSeq>::size(),
    //     tracked_ninfected
    // );

    Model<TSeq>::set_rand_binom(
        Model<TSeq>::size(),
        static_cast<double>(
            Model<TSeq>::par("Contact rate"))/
            static_cast<double>(Model<TSeq>::size())
        );

    return;

}

template<typename TSeq>
inline Model<TSeq> * ModelSIRCONN<TSeq>::clone_ptr()
{
    
    ModelSIRCONN<TSeq> * ptr = new ModelSIRCONN<TSeq>(
        *dynamic_cast<const ModelSIRCONN<TSeq>*>(this)
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
inline ModelSIRCONN<TSeq>::ModelSIRCONN(
    ModelSIRCONN<TSeq> & model,
    std::string vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double contact_rate,
    epiworld_double prob_transmission,
    epiworld_double prob_recovery
    // epiworld_double prob_reinfection
    )
{



    epiworld::UpdateFun<TSeq> update_susceptible = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void
        {

            // Sampling how many individuals
            int ndraw = m->rbinom();

            if (ndraw == 0)
                return;

            // Drawing from the set
            int nvariants_tmp = 0;
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
                auto neighbor = m->get_agents()[which];
                if (neighbor.get_state() == ModelSIRCONN<TSeq>::INFECTED)
                {

                    for (const VirusPtr<TSeq> & v : neighbor.get_viruses()) 
                    { 

                        #ifdef EPI_DEBUG
                        if (nvariants_tmp >= m->array_virus_tmp.size())
                            throw std::logic_error("Trying to add an extra element to a temporal array outside of the range.");
                        #endif
                            
                        /* And it is a function of susceptibility_reduction as well */ 
                        m->array_double_tmp[nvariants_tmp] =
                            (1.0 - p->get_susceptibility_reduction(v, m)) * 
                            v->get_prob_infecting(m) * 
                            (1.0 - neighbor.get_transmission_reduction(v, m)) 
                            ; 
                    
                        m->array_virus_tmp[nvariants_tmp++] = &(*v);
                        
                    } 

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

    epiworld::UpdateFun<TSeq> update_infected = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void
        {

            // // Getting the right type
            // ModelSIRCONN<TSeq> * _m = dynamic_cast<ModelSIRCONN<TSeq>*>(m);

            // Is recovering
            if (m->runif() < (m->par("Prob. Recovery")))
            {

                // --_m->tracked_ninfected_next;
                epiworld::VirusPtr<int> v = p->get_virus(0u);
                p->rm_virus(0, m);
                return;

            }

            // // Will be present next
            // _m->tracked_agents_infected_next.push_back(p);

            return;

        };

    // epiworld::GlobalFun<TSeq> global_accounting = [](epiworld::Model<TSeq> * m) -> void
    //     {

    //         // Getting the right type
    //         ModelSIRCONN<TSeq> * _m = dynamic_cast<ModelSIRCONN<TSeq>*>(m);

    //         // On the last day, also reset tracked agents and
    //         // set the initialized value to false
    //         if (static_cast<epiworld_fast_uint>(m->today()) == (m->get_ndays() - 1))
    //         {
    //             return;
    //         }

    //         std::swap(_m->tracked_agents_infected, _m->tracked_agents_infected_next);
    //         _m->tracked_agents_infected_next.clear();

    //         _m->tracked_ninfected += _m->tracked_ninfected_next;
    //         _m->tracked_ninfected_next = 0;

    //         _m->tracked_current_infect_prob = 1.0 - std::pow(
    //             1.0 - (m->par("Contact rate")) * (m->par("Prob. Transmission")) / m->size(),
    //             _m->tracked_ninfected
    //             );

    //     };

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

    // // Adding updating function
    // model.add_global_action(global_accounting, -1);

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
    epiworld_double contact_rate,
    epiworld_double prob_transmission,
    epiworld_double prob_recovery
    )
{

    ModelSIRCONN(
        *this,
        vname,
        n,
        prevalence,
        contact_rate,
        prob_transmission,
        prob_recovery
    );

    return;

}


#endif
