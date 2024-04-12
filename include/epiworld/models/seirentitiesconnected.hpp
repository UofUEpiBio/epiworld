#ifndef EPIWORLD_MODELS_SEIRENTITIESCONNECTED_HPP
#define EPIWORLD_MODELS_SEIRENTITIESCONNECTED_HPP

template<typename TSeq>
class GroupSampler;

/**
 * @file seirentitiesconnected.hpp
 * @brief Template for a Susceptible-Exposed-Infected-Removed (SEIR) model with entities
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelSEIREntitiesConn : public epiworld::Model<TSeq> 
{
public:

    static const int SUSCEPTIBLE = 0;
    static const int EXPOSED     = 1;
    static const int INFECTED    = 2;
    static const int RECOVERED   = 3;

    GroupSampler<TSeq> group_sampler;


    ModelSEIREntitiesConn() {};

    
    /**
     * @brief Constructs a ModelSEIREntitiesConn object.
     *
     * @param model A reference to an existing ModelSEIREntitiesConn object.
     * @param vname The name of the ModelSEIREntitiesConn object.
     * @param n The number of entities in the model.
     * @param prevalence The initial prevalence of the disease in the model.
     * @param contact_rate The contact rate between entities in the model.
     * @param transmission_rate The transmission rate of the disease in the model.
     * @param avg_incubation_days The average incubation period of the disease in the model.
     * @param recovery_rate The recovery rate of the disease in the model.
     * @param entities A vector of entity values.
     * @param entities_names A vector of entity names.
     */
    ModelSEIREntitiesConn(
        ModelSEIREntitiesConn<TSeq> & model,
        std::string vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double avg_incubation_days,
        epiworld_double recovery_rate,
        std::vector< epiworld_double > entities,
        std::vector< std::string > entities_names,
        std::vector< double > contact_matrix
    );
    
    /**
     * @brief Constructs a ModelSEIREntitiesConn object.
     *
     * @param vname The name of the ModelSEIREntitiesConn object.
     * @param n The number of entities in the model.
     * @param prevalence The initial prevalence of the disease in the model.
     * @param contact_rate The contact rate between entities in the model.
     * @param transmission_rate The transmission rate of the disease in the model.
     * @param avg_incubation_days The average incubation period of the disease in the model.
     * @param recovery_rate The recovery rate of the disease in the model.
     * @param entities A vector of entity values.
     */
    ModelSEIREntitiesConn(
        std::string vname,
        epiworld_fast_uint n,
        epiworld_double prevalence,
        epiworld_double contact_rate,
        epiworld_double transmission_rate,
        epiworld_double avg_incubation_days,
        epiworld_double recovery_rate,
        std::vector< epiworld_double > entities,
        std::vector< std::string > entities_names,
        std::vector< double > contact_matrix
    );

    ModelSEIREntitiesConn<TSeq> & run(
        epiworld_fast_uint ndays,
        int seed = -1
    );

    void reset();

    Model<TSeq> * clone_ptr();

    /**
     * @brief Set the initial states of the model
     * @param proportions_ Double vector with a single element:
     * - The proportion of non-infected individuals who have recovered.
    */
    ModelSEIREntitiesConn<TSeq> & initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_ = {}
    );

};

/**
 * @brief Weighted sampling of groups
 */
template<typename TSeq> 
class GroupSampler {

private:
    
    epiworld::Model<TSeq> & model;
    const std::vector< double > contact_matrix; ///< Contact matrix between groups
    const std::vector< size_t > group_sizes;    ///< Sizes of the groups
    std::vector< double > cumulate;               ///< Cumulative sum of the contact matrix (row-major for faster access)

    /**
     * @brief Get the index of the contact matrix
     * 
     * The matrix is a vector stored in column-major order.
     * 
     * @param i Index of the row
     * @param j Index of the column
     * @return Index of the contact matrix
     */
    inline int idx(const int i, const int j, bool rowmajor = false) const
    {
        
        if (rowmajor)
            return i * group_sizes.size() + j;
        
        return j * group_sizes.size() + i; 

    }

public:

    GroupSampler(
        epiworld::Model<TSeq> & model,
        const std::vector< double > & contact_matrix,
        const std::vector< size_t > & group_sizes,
        bool normalize = true
    ): model(model), contact_matrix(contact_matrix), group_sizes(group_sizes) {

        this->cumulate.resize(contact_matrix.size());
        std::fill(cumulate.begin(), cumulate.end(), 0.0);

        // Cumulative sum
        for (size_t j = 1; j < group_sizes.size(); ++j)
        {
            for (size_t i = 0; i < group_sizes.size(); ++i)
                cumulate[idx(i, j, true)] += 
                    cumulate[idx(i, j - 1, true)] +
                    contact_matrix[idx(i, j)];
        }

        if (normalize)
        {
            for (size_t i = 0; i < group_sizes.size(); ++i)
            {
                double sum = 0.0;
                for (size_t j = 0; j < group_sizes.size(); ++j)
                    sum += contact_matrix[idx(i, j, true)];
                for (size_t j = 0; j < group_sizes.size(); ++j)
                    contact_matrix[idx(i, j, true)] /= sum;
            }
        }

    };

    int sample_1(const int origin_group);

    void sample_n(
        std::vector< size_t > & sample,
        const int origin_group,
        const int nsamples
    );

};

template<typename TSeq>
int GroupSampler<TSeq>::sample_1(const int origin_group)
{

    // Random number
    double r = model.runif();

    // Finding the group
    size_t j = 0;
    while (r > cumulate[idx(origin_group, j, true)])
        ++j;

    // Adjusting the prob
    r = r - (j == 0 ? 0.0 : cumulate[idx(origin_group, j - 1, true)]);

    int res = static_cast<int>(
        std::floor(r * group_sizes[j])
    );

    // Making sure we are not picling outside of the group
    if (res >= static_cast<int>(group_sizes[j]))
        res = static_cast<int>(group_sizes[j]) - 1;

    return res;

}

template<typename TSeq>
void GroupSampler<TSeq>::sample_n(
    std::vector< size_t > & sample,
    const int origin_group,
    const int nsamples
)
{

    for (int i = 0; i < nsamples; ++i)
        sample[i] = sample_1(origin_group);

    return;

}




template<typename TSeq>
inline ModelSEIREntitiesConn<TSeq> & ModelSEIREntitiesConn<TSeq>::run(
    epiworld_fast_uint ndays,
    int seed
)
{
    
    Model<TSeq>::run(ndays, seed);

    return *this;

}

template<typename TSeq>
inline void ModelSEIREntitiesConn<TSeq>::reset()
{

    Model<TSeq>::reset();

    Model<TSeq>::set_rand_binom(
        Model<TSeq>::size(),
        static_cast<double>(
            Model<TSeq>::par("Contact rate"))/
            static_cast<double>(Model<TSeq>::size())
        );

    return;

}

template<typename TSeq>
inline Model<TSeq> * ModelSEIREntitiesConn<TSeq>::clone_ptr()
{
    
    ModelSEIREntitiesConn<TSeq> * ptr = new ModelSEIREntitiesConn<TSeq>(
        *dynamic_cast<const ModelSEIREntitiesConn<TSeq>*>(this)
        );

    return dynamic_cast< Model<TSeq> *>(ptr);

}


/**
 * @brief Template for a Susceptible-Exposed-Infected-Removed (SEIR) model
 * 
 * @param model A Model<TSeq> object where to set up the SIR.
 * @param vname std::string Name of the virus
 * @param prevalence Initial prevalence (proportion)
 * @param contact_rate Average number of contacts (interactions) per step.
 * @param transmission_rate Probability of transmission
 * @param recovery_rate Probability of recovery
 */
template<typename TSeq>
inline ModelSEIREntitiesConn<TSeq>::ModelSEIREntitiesConn(
    ModelSEIREntitiesConn<TSeq> & model,
    std::string vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate,
    std::vector< epiworld_double > entities,
    std::vector< std::string > entities_names,
    std::vector< double > contact_matrix
    )
{

    // Instantiating entities
    int entity_id = 0;
    for (auto & e : entities)
        model.add_entity_n(
            epiworld::Entity<TSeq>(entities_names[entity_id++]),
            e * n
            );

    std::vector< size_t > group_sizes(entities.size());
    for (size_t i = 0; i < entities.size(); ++i)
        group_sizes[i] = static_cast<size_t>(entities[i] * n);

    // Setting up the group sampler
    GroupSampler<TSeq> group_sampler(
        model,
        contact_matrix,
        group_sizes
        );

    epiworld::UpdateFun<TSeq> update_susceptible = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void
        {

            // Sampling how many individuals
            int ndraw = m->rbinom();
                        
            if (ndraw == 0)
                return;

            // Sampling from the agent's entities
            epiworld::AgentsSample<TSeq> sample(m, *p, ndraw, {}, true);

            // Drawing from the set
            int nviruses_tmp = 0;
            for (const auto & neighbor: sample)
            {

                // Can't sample itself
                if (neighbor->get_id() == static_cast<int>(p->get_id()))
                    continue;

                // If the neighbor is infected, then proceed
                if (neighbor->get_state() == ModelSEIREntitiesConn<TSeq>::INFECTED)
                {

                    auto & v = neighbor->get_virus();

                    #ifdef EPI_DEBUG
                    if (nviruses_tmp >= static_cast<int>(m->array_virus_tmp.size()))
                        throw std::logic_error("Trying to add an extra element to a temporal array outside of the range.");
                    #endif
                        
                    /* And it is a function of susceptibility_reduction as well */ 
                    m->array_double_tmp[nviruses_tmp] =
                        (1.0 - p->get_susceptibility_reduction(v, m)) * 
                        v->get_prob_infecting(m) * 
                        (1.0 - neighbor->get_transmission_reduction(v, m)) 
                        ; 
                
                    m->array_virus_tmp[nviruses_tmp++] = &(*v);

                }
            }

            // No virus to compute
            if (nviruses_tmp == 0u)
                return;

            // Running the roulette
            int which = roulette(nviruses_tmp, m);

            if (which < 0)
                return;

            p->set_virus(
                *m->array_virus_tmp[which],
                m,
                ModelSEIREntitiesConn<TSeq>::EXPOSED
                );

            return; 

        };

    epiworld::UpdateFun<TSeq> update_infected = [](
        epiworld::Agent<TSeq> * p, epiworld::Model<TSeq> * m
        ) -> void {

            auto state = p->get_state();

            if (state == ModelSEIREntitiesConn<TSeq>::EXPOSED)
            {

                // Getting the virus
                auto & v = p->get_virus();

                // Does the agent become infected?
                if (m->runif() < 1.0/(v->get_incubation(m)))
                {

                    p->change_state(m, ModelSEIREntitiesConn<TSeq>::INFECTED);
                    return;

                }


            } else if (state == ModelSEIREntitiesConn<TSeq>::INFECTED)
            {


                // Odd: Die, Even: Recover
                epiworld_fast_uint n_events = 0u;
                const auto & v = p->get_virus();

                // Recover
                m->array_double_tmp[n_events++] = 
                    1.0 - (1.0 - v->get_prob_recovery(m)) * (1.0 - p->get_recovery_enhancer(v, m)); 

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
                p->rm_virus(m);

                return ;

            } else
                throw std::logic_error("This function can only be applied to exposed or infected individuals. (SEIR)") ;

            return;

        };

    // Setting up parameters
    model.add_param(contact_rate, "Contact rate");
    model.add_param(transmission_rate, "Prob. Transmission");
    model.add_param(recovery_rate, "Prob. Recovery");
    model.add_param(avg_incubation_days, "Avg. Incubation days");
    
    // state
    model.add_state("Susceptible", update_susceptible);
    model.add_state("Exposed", update_infected);
    model.add_state("Infected", update_infected);
    model.add_state("Recovered");


    // Preparing the virus -------------------------------------------
    epiworld::Virus<TSeq> virus(vname);
    virus.set_state(
        ModelSEIREntitiesConn<TSeq>::EXPOSED,
        ModelSEIREntitiesConn<TSeq>::RECOVERED,
        ModelSEIREntitiesConn<TSeq>::RECOVERED
        );

    virus.set_prob_infecting(&model("Prob. Transmission"));
    virus.set_prob_recovery(&model("Prob. Recovery"));
    virus.set_incubation(&model("Avg. Incubation days"));

    model.add_virus(virus, prevalence);

    model.queuing_off(); // No queuing need

    // Adding the empty population
    model.agents_empty_graph(n);

    model.set_name("Susceptible-Exposed-Infected-Removed (SEIR) (connected)");

    return;

}

template<typename TSeq>
inline ModelSEIREntitiesConn<TSeq>::ModelSEIREntitiesConn(
    std::string vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double contact_rate,
    epiworld_double transmission_rate,
    epiworld_double avg_incubation_days,
    epiworld_double recovery_rate,
    std::vector< epiworld_double > entities,
    std::vector< std::string > entity_names,
    std::vector< double > contact_matrix
    )
{

    ModelSEIREntitiesConn(
        *this,
        vname,
        n,
        prevalence,
        contact_rate,
        transmission_rate,
        avg_incubation_days,
        recovery_rate,
        entities,
        entity_names,
        contact_matrix
    );

    return;

}

template<typename TSeq>
inline ModelSEIREntitiesConn<TSeq> & ModelSEIREntitiesConn<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /* queue_ */
)
{

    Model<TSeq>::initial_states_fun =
        create_init_function_seir<TSeq>(proportions_)
        ;

    return *this;

}

#endif
