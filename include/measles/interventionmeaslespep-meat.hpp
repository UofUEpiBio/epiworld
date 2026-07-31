#ifndef EPIWORLD_INTERVENTIONMEASLESPEP_MEAT_HPP
#define EPIWORLD_INTERVENTIONMEASLESPEP_MEAT_HPP

using namespace epiworld;

#include "interventionmeaslespep-bones.hpp"

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
    std::vector< int > states_if_pep_ineffective,
    std::vector< int > agent_groups
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

    // An empty vector means "no groups": the whole population is treated
    // as a single exposed group. Its length can only be checked against
    // the model once we have one, so that happens in _setup().
    this->_agent_groups = agent_groups;

    // Paramters
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

    // Groups are optional, but if given there must be one entry per agent
    // (they are indexed by agent id).
    if (
        !this->_agent_groups.empty() &&
        (this->_agent_groups.size() != model->size())
    )
        throw std::length_error(
            "The vector of agent groups passed to InterventionMeaslesPEP "
            "must be either empty (no groups: PEP is offered to the whole "
            "population) or have one entry per agent. It currently has " +
            std::to_string(this->_agent_groups.size()) +
            " entries, but the model has " +
            std::to_string(model->size()) + " agents."
        );

    // Randomizing willingness
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
    }

    for (size_t i = 0u; i < model->size(); ++i)
    {
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

    // Checking dynamically
    auto quarantine_trigger_ptr = dynamic_cast<QuarantineTrigger<TSeq> *>(model);
    if (quarantine_trigger_ptr == nullptr)
    {
        throw std::logic_error(
            "The InterventionMeaslesPEP global event can only be used with "
            "models that inherit from QuarantineTrigger. This is because the "
            "intervention relies on the quarantine triggering mechanism to "
            "learn which cases were identified today, and the day public "
            "health considers each of them to have become infectious, which "
            "is what dates the exposure."
        );
    }

    // Verifying if this needs to be setup
    if (static_cast<int>(model->get_sim_id()) != this->model_id)
    {
        this->model_id = static_cast<int>(model->get_sim_id());
        this->_setup(model);
    }


    // Common variables
    int pep_mmr_window = static_cast<int>(model->par(this->_par_pep_mmr_window));
    int pep_ig_window = static_cast<int>(model->par(this->_par_pep_ig_window)); 

    auto & contact_trace = model->get_contact_tracing();

    // Getting the list of agents that triggered the quarantine, together
    // with the day public health considers each of them to have become
    // infectious.
    //
    // This global event runs every day, but the triggering set is only
    // refreshed when a case is actually identified. We therefore treat it
    // as a queue of detections to respond to: today's detections are taken
    // and the queue emptied, so that a detection is not answered with a
    // second round of PEP on every subsequent day.
    auto & triggering_agents = quarantine_trigger_ptr->get_triggering_agents();
    auto & date_infectious   = quarantine_trigger_ptr->get_date_infectious();

    if (triggering_agents.empty())
        return;

    std::vector< size_t > cases(triggering_agents);
    std::vector< int > cases_infectious_since(date_infectious);

    triggering_agents.clear();
    date_infectious.clear();

    // Making room (we will iterate this vectors
    // later to figure out the state changes.)
    _to_receive_pep.clear();
    _next_if_effective.clear();
    _next_if_ineffective.clear();

    // -------------------------------------------------------------------
    // Step 1: date the exposure.
    //
    // Public health does not have the time to trace individual contacts.
    // When a case is identified they look at the group that was exposed
    // (here, the whole school) and ask how long ago that exposure started,
    // since MMR/IG must be given within a few days *of the exposure*.
    //
    // Given an index case with rash onset on day `d`, public health
    // considers it infectious from `d - prodromal_period` onwards (that is
    // `date_infectious`, computed by the model). The reference date is
    // then the FIRST day, on or after that, on which the class actually
    // encountered the index:
    //
    //                first_seen
    //                v
    //   |------------|=====================|.............| today
    //   ^            (index in school and infectious)     (case detected)
    //   infectious_since
    //                |<-------- days_since = today - first_seen -------->|
    //
    // Contact tracing is used only to *date* that first encounter, not to
    // decide who was exposed. This matters when the class is not in
    // session every day: if the contact rate is set to zero on weekends
    // (e.g. via a global event) and the infectious window opens on a
    // Saturday, then the first actual encounter is the following Monday,
    // and Monday -- not Saturday -- anchors the MMR window.
    //
    // Each exposed group is dated separately: a case in one classroom
    // says nothing about how long ago another classroom was exposed.
    // When no groups were given every case falls into the same bucket,
    // which is the whole-population behaviour.
    // -------------------------------------------------------------------
    std::map< int, int > group_first_seen;
    for (size_t t_i = 0u; t_i < cases.size(); ++t_i)
    {

        size_t agent_id = cases[t_i];

        auto n_recorded = contact_trace.get_n_contacts(agent_id);
        if (n_recorded == 0)
            continue;

        // Start of the infectious window as considered by public health:
        // rash onset counted backwards by the prodromal period. Nobody
        // could have been exposed before the simulation began.
        int infectious_since = cases_infectious_since[t_i];
        if (infectious_since < 0)
            infectious_since = 0;

        // First day the class encountered this index while infectious.
        //
        // Contact tracing keeps only the most recent `max_contacts`
        // encounters per agent, in a circular buffer. Once it has wrapped
        // around, the earliest encounters have been overwritten, and the
        // oldest one still on record is *later* than the true first
        // encounter. Trusting it would shorten `days_since` and hand out
        // PEP after the window had in fact closed. When we detect that
        // loss we fall back to the date public health would use with no
        // contact data at all: the infectious-onset date. That is the
        // earliest the class could possibly have been exposed, so the
        // fallback can only withhold PEP, never grant it too late.
        int index_first_seen = -1;
        if (n_recorded > contact_trace.get_max_contacts())
        {
            index_first_seen = infectious_since;
        }
        else
        {
            for (size_t i = 0u; i < n_recorded; ++i)
            {
                int contact_day = contact_trace.get_contact(agent_id, i).second;

                // Encounters before the index was considered infectious do
                // not expose anyone.
                if (contact_day < infectious_since)
                    continue;

                if ((index_first_seen < 0) || (contact_day < index_first_seen))
                    index_first_seen = contact_day;
            }
        }

        // This index never met the class while infectious (e.g. it was
        // never in school during its infectious window), so it exposed
        // nobody.
        if (index_first_seen < 0)
            continue;

        // When several cases are identified together in the same group,
        // public health works from the earliest exposure: a co-detected
        // case whose rash started earlier dictates how much time is left
        // to intervene.
        int group = this->_group_of(agent_id);
        auto group_i = group_first_seen.find(group);
        if (
            (group_i == group_first_seen.end()) ||
            (index_first_seen < group_i->second)
        )
            group_first_seen[group] = index_first_seen;

    }

    // Is there still time to intervene? Drop the groups whose window has
    // already closed; what remains are the groups to offer PEP to.
    for (auto group_i = group_first_seen.begin();
         group_i != group_first_seen.end(); )
    {
        int days_since = model->today() - group_i->second;

        if (
            (days_since < 0) ||
            ((days_since > pep_mmr_window) && (days_since > pep_ig_window))
        )
            group_i = group_first_seen.erase(group_i);
        else
            ++group_i;
    }

    // Either no identified case actually exposed anyone, or it is too late
    // for both MMR and IG everywhere: nobody is offered PEP.
    if (group_first_seen.empty())
        return;

    // -------------------------------------------------------------------
    // Step 2: offer PEP to the exposed group(s).
    //
    // We are not doing individual contact tracing: everyone in an exposed
    // group is assumed to have been exposed. Every agent of that group
    // still in a PEP-target state is therefore offered PEP, whether or not
    // they were recorded as having met the index case. Agents who are
    // already immune or otherwise ineligible are excluded by not being in
    // a target state.
    //
    // Groups let this be narrowed to, say, the classroom the identified
    // case belongs to. Without them the exposed group is the entire
    // population.
    // -------------------------------------------------------------------
    auto & tool_mmr = model->get_tool("PEP MMR");
    auto & tool_ig  = model->get_tool("PEP IG");

    for (size_t agent_i = 0u; agent_i < model->size(); ++agent_i)
    {

        // Was this agent's group exposed by a case identified today, and
        // is that exposure still recent enough to act on?
        auto group_i = group_first_seen.find(this->_group_of(agent_i));
        if (group_i == group_first_seen.end())
            continue;

        // MMR is preferred while we are within its (shorter) window;
        // otherwise IG is offered if we are within its window.
        int days_since = model->today() - group_i->second;
        bool within_mmr_window = (days_since <= pep_mmr_window);
        bool within_ig_window  = (days_since <= pep_ig_window);

        auto & agent = model->get_agent(agent_i);

        // Is the agent eligible for PEP?
        int agent_state = static_cast<int>(agent.get_state());
        if (!IN(agent_state, this->_target_states))
            continue;

        // Already under prophylaxis: there is nothing to add by dosing
        // again, and willingness is fixed, so re-offering would not change
        // the agent's decision. Note that IG wanes and is removed once its
        // duration is over, at which point the agent becomes eligible again.
        if (agent.has_tool("PEP MMR") || agent.has_tool("PEP IG"))
            continue;

        // Willingness to receive each of the two prophylaxes.
        if (within_mmr_window && this->_willing_to_receive_mmr[agent_i])
        {
            // We will administer MMR PEP to the agent
            agent.add_tool(
                *model,
                tool_mmr
            );
        }
        else if (within_ig_window && this->_willing_to_receive_ig[agent_i])
        {
            // We will administer IG PEP to the agent
            agent.add_tool(
                *model,
                tool_ig
            );
        }
        // Nothing happens
        else
            continue;

        // Finding the corresponding state for PEP
        auto it = std::find(
            this->_target_states.begin(),
            this->_target_states.end(),
            agent_state
        );

        // No need to check it, we know it is there
        auto pos = std::distance(this->_target_states.begin(), it);

        // Recording the information of the agent
        // so we can decide to what state to move
        _to_receive_pep.push_back(agent.get_id());
        _next_if_effective.push_back(_states_if_pep_effective[pos]);
        _next_if_ineffective.push_back(_states_if_pep_ineffective[pos]);

    }

    // Second set of iterations (figuring out if the agents
    // will recover or not)
    model->events_run();
    for (size_t i = 0u; i < _to_receive_pep.size(); ++i)
    {
        auto & agent = model->get_agent(_to_receive_pep[i]);

        int next_state_success = _next_if_effective[i];
        int next_state_failure = _next_if_ineffective[i];

        // Recovery is only possible if the agent has a virus
        // and the tool reduces susceptibility.
        auto & agent_v = agent.get_virus();
        if (agent_v != nullptr)
        {
            auto recovers = agent.get_susceptibility_reduction(
                agent.get_virus(), *model
            );

            if (recovers > model->runif())
            {
                agent.rm_virus(*model, next_state_success);
                continue;
            }

        } 
        
        agent.change_state(*model, next_state_failure);

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