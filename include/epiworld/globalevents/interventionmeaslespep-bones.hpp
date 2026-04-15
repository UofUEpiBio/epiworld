#ifndef EPIWORLD_INTERVENTIONMEASLESPEP_BONES_HPP
#define EPIWORLD_INTERVENTIONMEASLESPEP_BONES_HPP

#include "../config.hpp"
#include "../model-bones.hpp"
#include "../agent-bones.hpp"

template<typename TSeq = EPI_DEFAULT_TSEQ>
class InterventionMeaslesPEP final : public GlobalEvent<TSeq> {

private:

    static inline const std::string _par_mmr_willingness{"PEP MMR willingness"};
    static inline const std::string _par_ig_willingness{"PEP IG willingness"};
    static inline const std::string _par_mmr_efficacy{"PEP MMR efficacy"};
    static inline const std::string _par_ig_efficacy{"PEP IG efficacy"};
    static inline const std::string _par_pep_mmr_window{"PEP MMR window"};
    static inline const std::string _par_pep_ig_window{"PEP IG window"};
    static inline const std::string _par_half_life_mean{"PEP IG half-life (mean)"};
    static inline const std::string _par_half_life_sd{"PEP IG half-life (sd)"};


    // Willingness and efficacy of the PEP
    epiworld_double _mmr_willingness;
    epiworld_double _ig_willingness;
    epiworld_double _mmr_efficacy;
    epiworld_double _ig_efficacy;
    epiworld_double _ig_half_life_mean;
    epiworld_double _ig_half_life_sd;
    epiworld_double _pep_mmr_window;
    epiworld_double _pep_ig_window;

    // Willingness of the agents to receive MMR PEP
    std::vector< bool > _willing_to_receive_mmr;
    // Willingness of the agents to receive IG PEP
    std::vector< bool > _willing_to_receive_ig;

    // Target states to which the intervention applies
    int _triggering_state;
    std::vector< int > _target_states;
    std::vector< int > _states_if_pep_effective;
    std::vector< int > _states_if_pep_ineffective;

    // Id of the model
    int model_id = -1;

    /**
     * @brief Set up the intervention.
     * 
     * @details
     * This function is called at the beginning of the simulation. It sets up
     * the willingness of the agents to receive PEP and the efficacy of the
     * PEP.
     * @param model A pointer to the model.
     */
    void _setup(Model<TSeq> * model);

    // List of agents that will receive PEP
    // This is used to avoid iterating over the agents twice.
    std::vector< size_t > _to_receive_pep;
    std::vector< int > _next_if_effective;
    std::vector< int > _next_if_ineffective;

    std::vector< bool > _focal_agent;
    std::vector< int > _focal_agent_contact_day;

public:

    /**
     * @brief Construct a new Intervention PEP object.
     * @param name The name of the intervention.
     * @param mmr_efficacy The efficacy of the PEP MMR. Must be between 0
     * and 1.
     * @param ig_efficacy The efficacy of the PEP IG. Must be between 0
     * and 1.
     * @param ig_half_life_mean Mean half-life of the IG in days.
     * @param ig_half_life_sd Standard deviation of the IG half-life in days.
     * @param mmr_willingness The willingness of the agents to receive
     * MMR PEP. Must be between 0 and 1. Set to 0 to deactivate MMR PEP.
     * @param ig_willingness The willingness of the agents to receive
     * IG PEP. Must be between 0 and 1. Set to 0 to deactivate IG PEP.
     * @param mmr_window Number of days after exposure within which
     * MMR PEP can be administered.
     * @param ig_window Number of days after exposure within which
     * IG PEP can be administered.
     * @param triggering_state State that triggers the intervention.
     * For example, if detected agents were moved to isolation, it
     * should be the state corresponding to isolation.
     * @param target_states The states to which the intervention applies. For
     * example, if the intervention applies to agents in quarantine, then this
     * should include the states that correspond to quarantine.
     * @param states_if_pep_effective The states to which the agents will be
     * moved if they receive PEP and it is effective. Must be the same length
     * as `target_states`.
     * @param states_if_pep_ineffective The states to which the agents will be
     * moved if they receive PEP and it is ineffective. Must be the same length
     * as `target_states`.
     */
    InterventionMeaslesPEP(
        std::string name,
        epiworld_double mmr_efficacy,
        epiworld_double ig_efficacy,
        epiworld_double ig_half_life_mean,
        epiworld_double ig_half_life_sd,
        epiworld_double mmr_willingness,
        epiworld_double ig_willingness,
        epiworld_double mmr_window,
        epiworld_double ig_window,
        int triggering_state,
        std::vector< int > target_states,
        std::vector< int > states_if_pep_effective,
        std::vector< int > states_if_pep_ineffective
    );

    /**
     * @brief Apply the intervention to the model.
     * @details
     * This function is called at the end of the day as a global event. It
     * iterates through the agents and gives PEP to those who are willing and
     * applicable.
     * 
     * Agents who receive PEP may then be moved to a different state if
     * the PEP is effective.
     */
    void operator()(Model<TSeq> * model, int day) override;

    std::unique_ptr< GlobalEvent<TSeq> > clone_ptr() const override;

    static bool agent_recovers(Agent<TSeq> & p, Model<TSeq> & m, int next_state);

};

#endif