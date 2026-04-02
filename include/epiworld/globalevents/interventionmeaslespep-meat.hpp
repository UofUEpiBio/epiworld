#ifndef EPIWORLD_INTERVENTIONMEASLESPEP_MEAT_HPP
#define EPIWORLD_INTERVENTIONMEASLESPEP_MEAT_HPP

#include "interventionmeaslespep-bones.hpp"

template<typename TSeq>
inline InterventionMeaslesPEP<TSeq>::InterventionMeaslesPEP(
    epiworld_double mmr_efficacy,
    epiworld_double ig_efficacy,
    epiworld_double pep_willingness,
    epiworld_double mmr_window,
    std::vector< int > quarantine_states,
    std::vector< int > quarantine_states_for_pep
) {

    // Must match the length
    if (quarantine_states.size() != quarantine_states_for_pep.size())
        throw std::logic_error(
            "The length of the quarantine states and the quarantine states for "
            "PEP must be the same. These are currently: " +
            std::to_string(quarantine_states.size()) + " and " +
            std::to_string(quarantine_states_for_pep.size()) +
            ", respectively."
        );

    this->_quarantine_states = quarantine_states;
    this->_quarantine_states_for_pep = quarantine_states_for_pep;
    
    // Paramters
    this->_mmr_efficacy = mmr_efficacy;
    this->_ig_efficacy = ig_efficacy;
    this->_willingness = pep_willingness;
    this->_pep_mmr_window = mmr_window;
}

template<typename TSeq>
inline void InterventionMeaslesPEP<TSeq>::_setup(
    Model<TSeq> * model
) {

    // Randomizing willingness
    this->_willing_to_receive_pep.assign(model->size(), false);

    // Setting the parameters
    model->add_param(this->_willingness, this->_par_willingness, true);
    model->add_param(this->_mmr_efficacy, this->_par_mmr_efficacy, true);
    model->add_param(this->_ig_efficacy, this->_par_ig_efficacy, true);
    model->add_param(this->_pep_mmr_window, this->_par_pep_mmr_window, true);

    auto willingness = model->par(this->_par_willingness);
    for (size_t i = 0u; i < model->size(); ++i)
    {
        if (model->runif() < willingness)
        {
            this->_willing_to_receive_pep[i] = true;
        }
    }

    // Adding the PEP vaccine as a tool to the model
    // (if not already added by the user)
    if (!model->has_tool("PEP MMR"))
    {
        // Creating the PEP vaccine tool
        ToolVaccine<TSeq> pep("PEP MMR");
        pep.set_susceptibility_reduction(model->par(this->_par_mmr_efficacy));
        model->add_tool(pep);

    }

    if (!model->has_tool("PEP IG"))
    {
        // Creating the PEP vaccine tool
        ToolVaccine<TSeq> ig("PEP IG");
        ig.set_susceptibility_reduction(model->par(this->_par_ig_efficacy));
        model->add_tool(ig);
    }

};

template<typename TSeq>
inline void InterventionMeaslesPEP<TSeq>::operator()(Model<TSeq> * model, int) {

    // Verifying if this needs to be setup
    if (static_cast<int>(model->get_sim_id()) != this->model_id)
    {
        this->model_id = static_cast<int>(model->get_sim_id());
        this->_setup(model);
    }

    // Capturing some basic information
    auto today = model->today();
    auto pep_mmr_window = static_cast<int>(model->par(this->_par_pep_mmr_window));

    // Precapturing the tools
    auto & tool_mmr = model->get_tool("PEP MMR");
    auto & tool_ig  = model->get_tool("PEP IG");
    
    // Iterating over the agents
    for (auto & agent: model->get_agents())
    {

        // Checking if the agent is in a quarantine
        // state
        int agent_state = static_cast<int>(agent.get_state());
        if (!IN(agent_state, this->_quarantine_states))
            continue;

        // Checking willigness
        if (!this->_willing_to_receive_pep[agent.get_id()])
            continue;

        // Finding the corresponding state for PEP
        auto it = std::find(
            this->_quarantine_states.begin(),
            this->_quarantine_states.end(),
            agent.get_state()
        );

        // No need to check it, we know it is there
        auto pos = std::distance(this->_quarantine_states.begin(), it);

        // Checking if the agent has a virus
        if (agent.get_virus() != nullptr)
        {

            // Checking when did the agent get infected
            int day_infected = agent.get_virus()->get_date();

            // If the date is within the window for PEP,
            // we administer MMR PEP to the agent
            if ((today - day_infected) > pep_mmr_window)
            {
                // We will administer MMR PEP to the agent
                agent.add_tool(
                    *model,
                    tool_ig,
                    this->_quarantine_states_for_pep[pos]
                );

                // Go to the next agent
                continue;
            }

        }

        // If we got this far, it is because the agent is either 
        agent.add_tool(
            *model,
            tool_mmr,
            this->_quarantine_states_for_pep[pos]
        );

    }

};

template<typename TSeq>
inline std::unique_ptr< GlobalEvent<TSeq> > InterventionMeaslesPEP<TSeq>::clone_ptr() const
{
    return std::make_unique< InterventionMeaslesPEP<TSeq>>(*this);
}

template<typename TSeq>
inline bool InterventionMeaslesPEP<TSeq>::agent_recovers(
    Agent<TSeq> & p,
    Model<TSeq> & m,
    int next_state
) {

    // If the agent has PEP, then we have to figure out if it works or not
    if (p.has_tool("PEP MMR"))
    {

        // Adding the tool
        auto & tool = p.get_tool("PEP MMR");
        if (tool.get_susceptibility_reduction(p.get_virus(), m) > 0.0)
        {
            p.rm_virus(m, next_state);
            return true;
        }
    }

    return false;

}

#endif