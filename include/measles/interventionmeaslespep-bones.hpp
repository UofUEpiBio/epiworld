#ifndef EPIWORLD_INTERVENTIONMEASLESPEP_BONES_HPP
#define EPIWORLD_INTERVENTIONMEASLESPEP_BONES_HPP

using namespace epiworld;

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


    // Willigness and efficacy of the PEP
    epiworld_double _mmr_willingness;
    epiworld_double _ig_willingness;
    epiworld_double _mmr_efficacy;
    epiworld_double _ig_efficacy;
    epiworld_double _ig_half_life_mean;
    epiworld_double _ig_half_life_sd;
    epiworld_double _pep_mmr_window;
    epiworld_double _pep_ig_window;

    // Willingness of the agents to receive MMR and IG as PEP
    std::vector< bool > _willing_to_receive_mmr;
    std::vector< bool > _willing_to_receive_ig;

    // The stages and statuses to which the intervention applies
    std::vector< int > _target_stages;
    std::vector< int > _target_statuses;

    // The model's transitions "PEP cleared" and "PEP release" (if any)
    int _tr_cleared = 0;
    int _tr_release = 0;
    bool _has_cleared = false;
    bool _has_release = false;

    // Group (e.g. classroom) each agent belongs to. Either empty, meaning
    // the whole population is treated as a single exposed group, or one
    // entry per agent, indexed by agent id.
    std::vector< int > _agent_groups;

    /**
     * @brief Group an agent belongs to.
     * @details
     * When no groups were specified the whole population is treated as a
     * single group, so that the grouped and ungrouped cases follow the
     * same code path.
     */
    int _group_of(size_t agent_id) const {
        return this->_agent_groups.empty() ?
            0 : this->_agent_groups[agent_id];
    }

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

public:

    /**
     * @brief Construct a new Intervention PEP object.
     * @param par_mmr_efficacy The efficacy of the PEP. Must be between 0
     * and 1.
     * @param par_pep_willingness The willingness of the agents to receive
     * PEP. Must be between 0 and 1.
     * @param target_stages The stages of the agents offered PEP (e.g.,
     * susceptible, latent and prodromal).
     * @param target_statuses The statuses of the agents offered PEP (e.g.,
     * quarantined); empty for any status.
     *
     * An agent dosed with PEP while carrying the virus clears it if the PEP
     * works (its susceptibility reduction, drawn once), through the model's
     * transition "PEP cleared"; then it is released through the model's
     * transition "PEP release" (e.g., from quarantine to free), if the model
     * has them and the agent is in their source.
     * @param agent_groups Optional group (e.g. classroom) membership, one
     * entry per agent, indexed by agent id. When a case is identified, PEP
     * is offered only within the group(s) the identified case(s) belong to.
     * Group labels are arbitrary integers; agents sharing a label are in
     * the same group. If empty (the default), the whole population is
     * treated as a single exposed group and PEP is offered to everyone.
     * Must be either empty or of length `Model::size()`.
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
        std::vector< int > target_stages,
        std::vector< int > target_statuses,
        std::vector< int > agent_groups = {}
    );

    /**
     * @brief Apply the intervention to the model.
     * @details
     * This function is called at the end of the day as a global event. It
     * iterates through the agents and gives PEP to those who are willing and
     * applicable.
     *
     * Agents who receive PEP may then clear the virus, and be released.
     *
     * If groups were specified, only agents in the same group as an
     * identified case are considered, and each group is timed from its own
     * exposure.
     */
    void operator()(Model<TSeq> * model, int day) override;

    std::unique_ptr< GlobalEvent<TSeq> > clone_ptr() const override;

};

#endif