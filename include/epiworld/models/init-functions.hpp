#ifndef EPIWORLD_MODELS_INIT_FUNCTIONS_HPP
#define EPIWORLD_MODELS_INIT_FUNCTIONS_HPP

#include "../model-bones.hpp"

/**
 * @brief Creates an initial function for the SIR-like models
 * @ingroup model_utilities
 * 
 * The function is used for the initial states of the model.
*/
template<typename TSeq>
inline std::function<void(Model<TSeq>*)> create_init_function_sir(
    std::vector< double > proportions_
) {

    // Checking widths
    if (proportions_.size() != 1u)
        throw std::invalid_argument(
            "The vector of proportions must have a single element."
            );

    // Proportion should be within [0, 1]
    if ((proportions_[0] < 0.0) || (proportions_[0] > 1.0))
        throw std::invalid_argument(
            "The proportion must be within (0, 1)."
            );

    double prop = proportions_[0u];

    std::function<void(Model<TSeq>*)> fun =
    [prop] (Model<TSeq> * model) -> void {

        // Figuring out information about the viruses
        double tot = 0.0;
        double n   = static_cast<double>(model->size());
        for (const auto & agent: model->get_agents())
        {
            if (agent.get_virus() != nullptr)
                tot += 1.0;
        }
        tot /= n;

        // Putting the total into context
        double tot_left = 1.0 - tot;

        // Since susceptible and infected are "fixed,"
        // we only need to change recovered
        size_t nrecovered = prop * tot_left * n;
        
        AgentsSample<TSeq> sample(
            *model,
            nrecovered,
            {0u},
            true
            );

        // Setting up the initial states
        for (auto & agent : sample)
            agent->change_state(*model, 2, Queue<TSeq>::NoOne);

        // Running the events
        model->events_run();

        return;

    };

    return fun;

}

/**
 * @brief Creates an initial function for the SIR-like models
 * @ingroup model_utilities
 *
 * The function is used for the initial states of the model.
*/
template<typename TSeq>
inline std::function<void(Model<TSeq>*)> create_init_function_sird(
    std::vector< double > prop
) {

    // Check length of prop equals two
    if (prop.size() != 2u)
        throw std::invalid_argument(
            "The vector of proportions must have two elements."
            );

    // Check elements in prop are within [0, 1] and sum up to 1
    double tot = 0.0;
    for (auto & v : prop)
    {
        if ((v < 0.0) || (v > 1.0))
            throw std::invalid_argument(
                "The proportion must be within (0, 1)."
                );
        tot += v;
    }

    if (tot >= 1.0)
        throw std::invalid_argument(
            "The proportions must sum up to 1."
            );

    std::function<void(Model<TSeq>*)> fun =
    [prop] (Model<TSeq> * model) -> void {

        // Figuring out information about the viruses
        double tot = 0.0;
        double n   = static_cast<double>(model->size());
        for (const auto & agent: model->get_agents())
        {
            if (agent.get_virus() != nullptr)
                tot += 1.0;
        }
        tot /= n;

        // Putting the total into context
        double tot_left = 1.0 - tot;

        // Since susceptible and infected are "fixed,"
        // we only need to change recovered
        size_t nrecovered = prop[0u] * tot_left * n;
        size_t ndeceased  = prop[01] * tot_left * n;

        AgentsSample<TSeq> sample_recover(
            *model,
            nrecovered,
            {0u},
            true
            );

        // Setting up the initial states
        for (auto & agent : sample_recover)
            agent->change_state(*model, 2, Queue<TSeq>::NoOne);

        AgentsSample<TSeq> sample_deceased(
            *model,
            ndeceased,
            {0u},
            true
            );

        // Setting up the initial states
        for (auto & agent : sample_deceased)
            agent->change_state(*model, 3, Queue<TSeq>::NoOne);
        
        // Running the events
        model->events_run();

        return;

    };

    return fun;

}


/**
 * @brief Creates an initial function for the SEIR-like models
 * @ingroup model_utilities
 * 
 * The function is used for the initial states of the model.
*/
template<typename TSeq>
inline std::function<void(Model<TSeq>*)> create_init_function_seir(
    std::vector< double > proportions_
) {

    // Checking widths
    if (proportions_.size() != 2u) {
        throw std::invalid_argument("-proportions_- must have two entries.");
    }

    // proportions_ are values between 0 and 1, otherwise error
    for (auto & v : proportions_)
        if ((v < 0.0) || (v > 1.0))
            throw std::invalid_argument(
                "-proportions_- must have values between 0 and 1."
                );


    std::function<void(Model<TSeq>*)> fun = 
        [proportions_] (Model<TSeq> * model) -> void {

        // Figuring out information about the viruses
        double tot = 0.0;
        double n   = static_cast<double>(model->size());
        for (const auto & agent: model->get_agents())
        {
            if (agent.get_virus() != nullptr)
                tot += 1.0;
        }
        tot /= n;

        // Putting the total into context
        double tot_left = 1.0 - tot;

        // Since susceptible and infected are "fixed,"
        // we only need to change recovered
        size_t nexposed   = proportions_[0u] * tot * n;
        size_t nrecovered = proportions_[1u] * tot_left * n;
        
        AgentsSample<TSeq> sample_suscept(
            *model,
            nrecovered,
            {0u},
            true
            );

        // Setting up the initial states
        for (auto & agent : sample_suscept)
            agent->change_state(*model, 3, Queue<TSeq>::NoOne);

        AgentsSample<TSeq> sample_exposed(
            *model,
            nexposed,
            {1u},
            true
            );

        // Setting up the initial states
        for (auto & agent : sample_exposed)
            agent->change_state(*model, 2, Queue<TSeq>::NoOne);

        // Running the events
        model->events_run();

        return;

    };

    return fun;

}

/**
 * @brief Creates an initial function for the SEIR-like models
 * @ingroup model_utilities
 *
 * The function is used for the initial states of the model.
*/
template<typename TSeq>
inline std::function<void(Model<TSeq>*)> create_init_function_seird(
    std::vector< double > proportions_
) {

    // Checking widths
    if (proportions_.size() != 3u) {
        throw std::invalid_argument("-proportions_- must have three entries.");
    }

    // proportions_ are values between 0 and 1, otherwise error
    for (auto & v : proportions_)
        if ((v < 0.0) || (v > 1.0))
            throw std::invalid_argument(
                "-proportions_- must have values between 0 and 1."
                );

    // Last first two terms shouldn't add up to more than 1
    if ((proportions_[1u] + proportions_[2u]) > 1.0)
        throw std::invalid_argument(
            "The last two terms of -proportions_- must add up to less than 1."
            );

    std::function<void(Model<TSeq>*)> fun = 
        [proportions_] (Model<TSeq> * model) -> void {

        // Figuring out information about the viruses
        double tot = 0.0;
        double n   = static_cast<double>(model->size());

        for (const auto & agent: model->get_agents())
        {
            if (agent.get_virus() != nullptr)
                tot += 1.0;
        }
        tot /= n;

        // Putting the total into context
        double tot_left = 1.0 - tot;

        // Since susceptible and infected are "fixed,"
        // we only need to change recovered
        size_t nexposed   = proportions_[0u] * tot * n;
        size_t nrecovered = proportions_[1u] * tot_left * n;
        size_t ndeceased  = proportions_[2u] * tot_left * n;
        
        AgentsSample<TSeq> sample_suscept(
            *model,
            nrecovered,
            {0u},
            true
            );

        // Setting up the initial states
        for (auto & agent : sample_suscept)
            agent->change_state(*model, 3, Queue<TSeq>::NoOne);

        AgentsSample<TSeq> sample_exposed(
            *model,
            nexposed,
            {1u},
            true
            );

        // Setting up the initial states
        for (auto & agent : sample_exposed)
            agent->change_state(*model, 2, Queue<TSeq>::NoOne);

        // Running the events
        model->events_run();

        // Setting the initial states for the deceased
        AgentsSample<TSeq> sample_deceased(
            *model,
            ndeceased,
            {0u},
            true
            );

        // Setting up the initial states
        for (auto & agent : sample_deceased)
            agent->change_state(*model, 4, Queue<TSeq>::NoOne);
        
        // Running the events
        model->events_run();

        return;

    };

    return fun;

}


/**
 * @brief Initial stages for models with a natural history.
 *
 * @details Moves some of the initial cases (agents with a virus, in the
 * infection stage) to `case_stage`, and some of the other agents (in the
 * first stage) to `other_stage`, in natural history 0. Every agent starts
 * in the model's first status.
 *
 * @param proportions_ Two values in [0, 1]: the share of the initial cases
 * moved to `case_stage`, and the share of the other agents moved to
 * `other_stage`.
 * @param case_stage,other_stage The stages to move them to.
 */
template<typename TSeq>
inline std::function<void(Model<TSeq>*)> create_init_function_stages(
    std::vector< double > proportions_,
    int case_stage,
    int other_stage
) {

    if (proportions_.size() != 2u)
        throw std::invalid_argument("-proportions_- must have two entries.");

    for (auto & v : proportions_)
        if ((v < 0.0) || (v > 1.0))
            throw std::invalid_argument(
                "-proportions_- must have values between 0 and 1."
            );

    return [proportions_, case_stage, other_stage](Model<TSeq> * model) -> void {

        const auto & nh = model->get_natural_history(0u);
        double n = static_cast< double >(model->size());
        double tot = 0.0;
        for (const auto & agent : model->get_agents())
            if (agent.get_virus() != nullptr)
                tot += 1.0;
        tot /= n;

        size_t ncases  = static_cast< size_t >(proportions_[0u] * tot * n);
        size_t nothers = static_cast< size_t >(proportions_[1u] * (1.0 - tot) * n);

        // The states of the agents to move: first status, the first stage or
        // the infection stage
        auto in_stage = [model](int s) {
            return static_cast< size_t >(model->state_with(0u, 0, 0u, s));
        };

        AgentsSample< TSeq > others(*model, nothers, {in_stage(0)}, true);
        for (auto & agent : others)
            agent->change_stage(*model, other_stage);

        AgentsSample< TSeq > cases(
            *model, ncases, {in_stage(nh.get_infection_stage())}, true
        );
        for (auto & agent : cases)
            agent->change_stage(*model, case_stage);

        model->events_run();

    };

}

#endif
