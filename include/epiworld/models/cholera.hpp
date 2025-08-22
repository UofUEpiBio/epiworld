#ifndef EPIWORLD_CHOLERA_HPP
#define EPIWORLD_CHOLERA_HPP

// Function to sample from n elements and assign to ans
#define SAMPLE_FROM_PROBS(n, ans) \
    size_t ans; \
    epiworld_double p_total = m->runif(); \
    for (ans = 0u; ans < n; ++ans) \
    { \
        if (p_total < m->array_double_tmp[ans]) \
            break; \
        m->array_double_tmp[ans + 1] += m->array_double_tmp[ans]; \
    }


#include <epiworld/epiworld.hpp>
using namespace epiworld;

template<typename TSeq = EPI_DEFAULT_TSEQ>
class ModelCholera : public epiworld::Model<TSeq>
{
public:

    // Possible states
    static const int SUSCEPTIBLE = 0;
    static const int INFECTED    = 1;
    static const int RECOVERED   = 2;

    ModelCholera() {};

    /**
     * @param n Population size
     * @param prevalence Number of initial cases.
     * @param env_load Initial load in the water (environment).
     * @param pp_t_rate Person-person transmission rate.
     * @param e_t_rate Environmental transmission rate.
     * @param hs_rate Half saturation rate in the environment (water).
     * @param loss_rate_imm Loss of human immunity
     * @param rec_rate Recovery rate of humans.
     * @param shedd_rate Shedding rate of humans into the environment.
     * @param b_death_rate_env Bacterial death rate in the environment.
     * @param i_growth_rate Bacterial growth rate.
     * @param b_carry_capacity Bacterial carrying capacity.
     * @param b_death_rate Bacterial death rate.
     * @param pb_contact_rate Phage-bacterial contact rate.
     * @param p_i_rate Prophage induction rate
     * @param cd_size Cell division size.
     * @param b_size Burst size
     * @param p_death_rate Phage death rate.
     * @param b_death_rate_lysis Bacterial death rate due to lysis.
     * @param nu Frac. of ingested vibrios that successfully est in host i.
     */
    ModelCholera(
        size_t n,
        size_t prevalence,
        double env_load,
        double pp_t_rate,
        double e_t_rate,
        double hs_rate,
        // double bd_rate_h /* Birth-death rate humans */
        double loss_rate_imm,
        double rec_rate,
        double shedd_rate,
        double b_death_rate_env,
        double i_growth_rate,
        double b_carry_capacity,
        // double ratio of time scale
        double b_death_rate,
        double pb_contact_rate,
        double p_i_rate,
        double cd_size,
        double b_size,
        double p_death_rate,
        double b_death_rate_lysis,
        double nu
    );

    ModelCholera(
        ModelCholera<TSeq> & model,
        size_t n,
        size_t prevalence,
        double env_load,
        double pp_t_rate,
        double e_t_rate,
        double hs_rate,
        // double bd_rate_h /* Birth-death rate humans */
        double loss_rate_imm,
        double rec_rate,
        double shedd_rate,
        double b_death_rate_env,
        double i_growth_rate,
        double b_carry_capacity,
        // double ratio of time scale
        double b_death_rate,
        double pb_contact_rate,
        double p_i_rate,
        double cd_size,
        double b_size,
        double p_death_rate,
        double b_death_rate_lysis,
        double nu
    );

    ModelCholera<TSeq> & initial_states(
        std::vector< double > proportions_,
        std::vector< int > queue_ = {}
    );

    /**
     * @brief Reset the model to initial state
     */
    void reset();

    /**
     * @brief Create a clone of this model
     * @return Pointer to a new model instance with the same configuration
     */
    Model<TSeq> * clone_ptr();

private:

    // Logic for sampling infected agents
    std::vector< size_t > sampled_agents;
    size_t m_sample_infected_agents();

    // Human updates
    static void m_update_susceptible(Agent<TSeq> * p, Model<TSeq> * m);
    static void m_update_infected(Agent<TSeq> * p, Model<TSeq> * m);
    static void m_update_recovered(Agent<TSeq> * p, Model<TSeq> * m);

    // Environmental model
    double env_vibrios_in_water;
    void m_update_envir();
    
    // Within host model
    void m_update_hosts();
    std::vector< double > env_vibrios_in_hosts;
    std::vector< double > lysogenic_bacteria;
    std::vector< double > free_phages;

    // Set of agents who are infected
    std::vector< size_t > infected_agents;
    size_t n_infected_agents;
    void m_update_infected_agents();

};

template<typename TSeq>
inline void ModelCholera<TSeq>::m_update_infected_agents()
{

    infected_agents.clear();
    auto & agents = Model<TSeq>::get_agents();

    for (const auto & a : agents)
    {
        if (a.get_state() == ModelCholera<TSeq>::INFECTED)
        {
            infected_agents.push_back(a.get_id());
        }
    }

    // Updating the size
    n_infected_agents = infected_agents.size();

    return;

}

template<typename TSeq>
inline size_t ModelCholera<TSeq>::m_sample_infected_agents()
{

    // Sampling infected individuals
    size_t n_sampled = epiworld::Model<TSeq>::rbinom(
        n_infected_agents,
        Model<TSeq>::par("Person-person transmission rate")
    );

    // If no agents are sampled, then return
    if (n_sampled == 0u)
        return;

    for (size_t i = 0u; i < n_sampled; ++i)
    {

        // Randomly selecting an agent
        int which = epiworld::Model<TSeq>::runif() * n_infected_agents;

        // Correcting overflow error
        if (which >= static_cast<int>(n_infected_agents))
            which = static_cast<int>(n_infected_agents) - 1;

        sampled_agents[i] = which;
        
    }

    return n_sampled;

}

template<typename TSeq>
inline void ModelCholera<TSeq>::m_update_susceptible(
    Agent<TSeq> * p, Model<TSeq> * m
)
{

    // Upcasting!
    auto & cholera_model = *dynamic_cast<ModelCholera<TSeq> *>(m);

    // If no agents, then figure out if transmitted from the water
    if (n_infected_agents == 0u)
    {

        // Does the person become infected from the water?
        double p = Model<TSeq>::runif();

        if (p < cholera_model.par("Environment transmission rate"))
        {
            // The second virus is Cholera in water
            p->set_virus(&m->get_virus(1));
        }

        return;
    }

    // Sampling agents
    auto n_sampled = cholera_model.m_sample_infected_agents();

    // If no cases, then return
    if (n_sampled == 0u)
        return;

    // Which agent?
    SAMPLE_FROM_PROBS(n_sampled, selected_agent);

    p->set_virus(
        *cholera_model.get_agent(
            cholera_model.sampled_agents[selected_agent]
        ).get_virus(),
        m
    );

    return;

}

template<typename TSeq>
inline void ModelCholera<TSeq>::reset()
{

    Model<TSeq>::reset();

    // Making room (cleaning)
    env_vibrios_in_water = 0.0;

    // Filling everything with zeros
    env_vibrios_in_hosts.assign(Model<TSeq>::size(), 0.0);
    lysogenic_bacteria.assign(Model<TSeq>::size(), 0.0);
    free_phages.assign(Model<TSeq>::size(), 0.0);

};

template<typename TSeq>
inline Model<TSeq> * ModelCholera<TSeq>::clone_ptr()
{
    
    ModelCholera<TSeq> * ptr = new ModelCholera<TSeq>(
        *dynamic_cast<const ModelCholera<TSeq>*>(this)
        );

    #if __cplusplus >= 202302L
        // C++23 or later
        [[assume(ptr != nullptr)]]
    #else
        // C++17 or C++20
        assert(ptr != nullptr); // Use assert for runtime checks
    #endif

    return dynamic_cast< Model<TSeq> *>(ptr);

};

template<typename TSeq>
inline ModelCholera<TSeq>::ModelCholera(
    size_t n,
    size_t prevalence,
    double env_load,
    double pp_t_rate,
    double e_t_rate,
    double hs_rate,
    // double bd_rate_h /* Birth-death rate humans */
    double loss_rate_imm,
    double rec_rate,
    double shedd_rate,
    double b_death_rate_env,
    double i_growth_rate,
    double b_carry_capacity,
    // double ratio of time scale
    double b_death_rate,
    double pb_contact_rate,
    double p_i_rate,
    double cd_size,
    double b_size,
    double p_death_rate,
    double b_death_rate_lysis,
    double nu
) {

    ModelCholera<TSeq>(
        *this,
        n,
        prevalence,
        env_load,
        pp_t_rate,
        e_t_rate,
        hs_rate,
        loss_rate_imm,
        rec_rate,
        shedd_rate,
        b_death_rate_env,
        i_growth_rate,
        b_carry_capacity,
        b_death_rate,
        pb_contact_rate,
        p_i_rate,
        cd_size,
        b_size,
        p_death_rate,
        b_death_rate_lysis,
        nu
    );

    return;

};

template<typename TSeq>
inline ModelCholera<TSeq>::ModelCholera(
    ModelCholera<TSeq> & model,
    size_t n,
    size_t prevalence,
    double env_load,
    double pp_t_rate,
    double e_t_rate,
    double hs_rate,
    // double bd_rate_h /* Birth-death rate humans */
    double loss_rate_imm,
    double rec_rate,
    double shedd_rate,
    double b_death_rate_env,
    double i_growth_rate,
    double b_carry_capacity,
    // double ratio of time scale
    double b_death_rate,
    double pb_contact_rate,
    double p_i_rate,
    double cd_size,
    double b_size,
    double p_death_rate,
    double b_death_rate_lysis,
    double nu
) {

    // Adding statuses
    model.add_state("Susceptible");
    model.add_state("Infected");
    model.add_state("Recovered");

    // Setting up the parameters
    model.add_param(pp_t_rate, "Person-person transmission rate");
    model.add_param(e_t_rate, "Environment transmission rate");
    model.add_param(hs_rate, "Human-to-sheep transmission rate");
    model.add_param(loss_rate_imm, "Loss of immunity rate");
    model.add_param(rec_rate, "Recovery rate");
    model.add_param(shedd_rate, "Shedding rate");
    model.add_param(b_death_rate_env, "Bacterial death rate in environment");
    model.add_param(i_growth_rate, "Infection growth rate");
    model.add_param(b_carry_capacity, "Bacterial carrying capacity");
    model.add_param(b_death_rate, "Bacterial death rate");
    model.add_param(pb_contact_rate, "Person-bacteria contact rate");
    model.add_param(p_i_rate, "Person-infection rate");
    model.add_param(cd_size, "Cholera dynamics size");
    model.add_param(b_size, "Bacterial size");
    model.add_param(p_death_rate, "Person death rate");
    model.add_param(b_death_rate_lysis, "Bacterial death rate due to lysis");
    model.add_param(nu, "Fraction of ingested vibrios");

    // Creating viruses
    Virus<TSeq> host_virus("Cholera");
    host_virus.set_distribution(
        distribute_virus_randomly<TSeq>(
            prevalence,
            false
        )
    );

    Virus<TSeq> water_virus("Cholera in water");
    water_virus.set_distribution(
        distribute_virus_randomly<TSeq>(
            0,
            false
        )
    );

    // Adding the viruses to the model
    model.add_virus(host_virus);
    model.add_virus(water_virus);    

    // Initializing the population
    model.agents_empty_graph(n);

    model.set_name("Within and between cholera");

    return;

};

template<typename TSeq>
inline ModelCholera<TSeq> & ModelCholera<TSeq>::initial_states(
    std::vector< double > proportions_,
    std::vector< int > /* queue_ */
) {

    Model<TSeq>::initial_states_fun =
        create_init_function_sir<TSeq>(proportions_)
        ;

    return *this;

};

#undef SAMPLE_FROM_PROBS

 #endif