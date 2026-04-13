#ifndef EPIWORLD_INTERVENTIONMEASLESPEP_MEAT_HPP
#define EPIWORLD_INTERVENTIONMEASLESPEP_MEAT_HPP

#include "interventionmeaslespep-bones.hpp"
#include "../tools/vaccine.hpp"
#include "../tools/immunoglobulin.hpp"

template<typename TSeq>
inline InterventionMeaslesPEP<TSeq>::InterventionMeaslesPEP(
    std::string name,
    epiworld_double mmr_efficacy,
    epiworld_double ig_efficacy,
    epiworld_double ig_half_life_mean,
    epiworld_double ig_half_life_sd,
    epiworld_double mmr_willingness,
    epiworld_double ig_willingness,
    epiworld_double mmr_window,
    epiworld_double ig_window,
    std::vector< int > target_states,
    std::vector< int > states_if_pep_effective,
    std::vector< int > states_if_pep_ineffective
) {

    this->set_name(name);

    // Must match the length
    if (target_states.size() != states_if_pep_effective.size() || target_states.size() != states_if_pep_ineffective.size())
        throw std::logic_error(
            "The length of the target states and the destination states for "
            "PEP must be the same. These are currently: " +
            std::to_string(target_states.size()) + " and " +
            std::to_string(states_if_pep_effective.size()) +
            ", respectively."
        );

    this->_target_states = target_states;
    this->_states_if_pep_effective = states_if_pep_effective;
    this->_states_if_pep_ineffective = states_if_pep_ineffective;

    // Parameters
    this->_mmr_efficacy = mmr_efficacy;
    this->_ig_efficacy = ig_efficacy;
    this->_ig_half_life_mean = ig_half_life_mean;
    this->_ig_half_life_sd = ig_half_life_sd;
    this->_mmr_willingness = mmr_willingness;
    this->_ig_willingness = ig_willingness;
    this->_pep_mmr_window = mmr_window;
    this->_pep_ig_window = ig_window;
}

template<typename TSeq>
inline void InterventionMeaslesPEP<TSeq>::_setup(
    Model<TSeq> * model
) {

    // Randomizing willingness (separate for MMR and IG)
    this->_willing_to_receive_mmr.assign(model->size(), false);
    this->_willing_to_receive_ig.assign(model->size(), false);

    // Setting the parameters
    model->add_param(this->_mmr_willingness, this->_par_mmr_willingness, true);
    model->add_param(this->_ig_willingness, this->_par_ig_willingness, true);
    model->add_param(this->_mmr_efficacy, this->_par_mmr_efficacy, true);
    model->add_param(this->_ig_efficacy, this->_par_ig_efficacy, true);
    model->add_param(this->_pep_mmr_window, this->_par_pep_mmr_window, true);
    model->add_param(this->_pep_ig_window, this->_par_pep_ig_window, true);
    model->add_param(this->_ig_half_life_mean, this->_par_half_life_mean, true);
    model->add_param(this->_ig_half_life_sd, this->_par_half_life_sd, true);

    auto mmr_willingness = model->par(this->_par_mmr_willingness);
    auto ig_willingness = model->par(this->_par_ig_willingness);
    for (size_t i = 0u; i < model->size(); ++i)
    {
        if (model->runif() < mmr_willingness)
        {
            this->_willing_to_receive_mmr[i] = true;
        }
        if (model->runif() < ig_willingness)
        {
            this->_willing_to_receive_ig[i] = true;
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
        ToolImmunoglobulin<TSeq> ig(
            "PEP IG",
            this->_par_ig_efficacy,
            this->_par_half_life_mean,
            this->_par_half_life_sd
        );
        
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
    auto pep_ig_window = static_cast<int>(model->par(this->_par_pep_ig_window));

    // Precapturing the tools
    auto & tool_mmr = model->get_tool("PEP MMR");
    auto & tool_ig  = model->get_tool("PEP IG");
    
    // Iterating over the agents
    _to_receive_pep.clear();
    _next_if_effective.clear();
    _next_if_ineffective.clear();
    for (auto & agent: model->get_agents())
    {

        // Checking if the agent is in a target
        // state
        int agent_state = static_cast<int>(agent.get_state());
        if (!IN(agent_state, this->_target_states))
            continue;

        // Finding the corresponding state for PEP
        auto it = std::find(
            this->_target_states.begin(),
            this->_target_states.end(),
            agent.get_state()
        );

        // No need to check it, we know it is there
        auto pos = std::distance(this->_target_states.begin(), it);

        // Checking if the agent has a virus
        if (agent.get_virus() != nullptr)
        {

            int day_infected = agent.get_virus()->get_date();
            bool within_mmr_window =
                (today - day_infected) < pep_mmr_window;
            bool within_ig_window =
                (today - day_infected) < pep_ig_window;

            // (a) Check if within the MMR window and willing
            if (within_mmr_window &&
                this->_willing_to_receive_mmr[agent.get_id()])
            {

                // Agents with virus may recover, so we track them
                _to_receive_pep.push_back(agent.get_id());
                _next_if_effective.push_back(
                    _states_if_pep_effective[pos]
                );
                _next_if_ineffective.push_back(
                    _states_if_pep_ineffective[pos]
                );

                agent.add_tool(*model, tool_mmr);

            }
            // (b) Otherwise, check if within the IG window and willing
            else if (within_ig_window &&
                     this->_willing_to_receive_ig[agent.get_id()])
            {

                _to_receive_pep.push_back(agent.get_id());
                _next_if_effective.push_back(
                    _states_if_pep_effective[pos]
                );
                _next_if_ineffective.push_back(
                    _states_if_pep_ineffective[pos]
                );

                agent.add_tool(*model, tool_ig);

            }

            // Go to the next agent (agents with virus are handled above)
            continue;

        }

        // If we got this far, the agent has no virus (susceptible in
        // quarantine). Check willingness for MMR first, then IG.
        if (this->_willing_to_receive_mmr[agent.get_id()])
        {
            agent.add_tool(
                *model,
                tool_mmr,
                this->_states_if_pep_effective[pos]
            );
        }
        else if (this->_willing_to_receive_ig[agent.get_id()])
        {
            agent.add_tool(
                *model,
                tool_ig,
                this->_states_if_pep_effective[pos]
            );
        }

    }

    // Second set of iterations (figuring out if the agents
    // will recover or not)
    model->events_run();
    for (size_t i = 0u; i < _to_receive_pep.size(); ++i)
    {
        auto & agent = model->get_agent(_to_receive_pep[i]);
        int next_state_success = _next_if_effective[i];
        int next_state_failure = _next_if_ineffective[i];

        auto recovers = agent.get_susceptibility_reduction(
            agent.get_virus(), *model
        ); 

        if (recovers > model->runif())
        {
            agent.rm_virus(*model, next_state_success);
        } 
        else
        {
            agent.change_state(*model, next_state_failure);
        }

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
        if (tool->get_susceptibility_reduction(p.get_virus(), m) > 0.0)
        {
            p.rm_virus(m, next_state);
            return true;
        }
    }

    if (p.has_tool("PEP IG"))
    {

        // Adding the tool
        auto & tool = p.get_tool("PEP IG");
        if (tool->get_susceptibility_reduction(p.get_virus(), m) > 0.0)
        {
            p.rm_virus(m, next_state);
            return true;
        }
    }

    return false;

}

#endif