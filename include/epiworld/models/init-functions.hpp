#ifndef EPIWORLD_MODELS_INIT_FUNCTIONS_HPP
#define EPIWORLD_MODELS_INIT_FUNCTIONS_HPP

#include <functional>
#include <vector>
#include <stdexcept>

#include <epiworld/model-bones.hpp>

/**
 * @brief Creates an initial function for the SIR-like models
 * @ingroup model_utilities
 * 
 * The function is used for the initial states of the model.
*/
template<typename TSeq>
inline std::function<void(Model<TSeq>*)> create_init_function_sir(
    std::vector< double > const& proportions
) {

    // Checking widths
    if (proportions.size() != 1) {
        throw std::invalid_argument(
            "The vector of proportions must have a single element."
        );
    }

    // Proportion should be within [0, 1]
    if ((proportions[0] < 0.0) || (proportions[0] > 1.0)) {
        throw std::invalid_argument(
            "The proportion must be within (0, 1)."
        );
    }

    double prop = proportions[0];

    std::function<void(Model<TSeq>*)> fun =
    [prop] (Model<TSeq> * model) -> void {
        // Figuring out information about the viruses
        double tot = 0.0;
        auto virus_count   = static_cast<double>(model->size());
        for (const auto & agent: model->get_agents())
        {
            if (agent.get_virus() != nullptr) {
                tot += 1.0;
            }
        }

        tot /= virus_count;

        // Putting the total into context
        double tot_left = 1.0 - tot;

        // Since susceptible and infected are "fixed,"
        // we only need to change recovered
        auto nrecovered = static_cast<size_t>(prop * tot_left * virus_count);
        
        AgentsSample<TSeq> sample(
            *model,
            nrecovered,
            {0},
            true
        );

        // Setting up the initial states
        for (auto & agent : sample) {
            agent->change_state(model, 2, Queue<TSeq>::NoOne);
        }
        
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
    std::vector< double > const& props
) {

    // Check length of prop equals two
    if (props.size() != 2) {
        throw std::invalid_argument(
            "The vector of proportions must have two elements."
        );
    }

    // Check elements in prop are within [0, 1] and sum up to 1
    double tot = 0.0;
    for (const auto & prop : props)
    {
        if ((prop < 0.0) || (prop > 1.0)) {
            throw std::invalid_argument(
                "The proportion must be within (0, 1)."
            );
        }

        tot += prop;
    }

    if (tot >= 1.0) {
        throw std::invalid_argument(
            "The proportions must sum up to 1."
        );
    }

    std::function<void(Model<TSeq>*)> fun =
    [props] (Model<TSeq> * model) -> void {
        // Figuring out information about the viruses
        double tot = 0.0;
        auto virus_count   = static_cast<double>(model->size());
        for (const auto & agent: model->get_agents())
        {
            if (agent.get_virus() != nullptr) {
                tot += 1.0;
            }
        }

        tot /= virus_count;

        // Putting the total into context
        double tot_left = 1.0 - tot;

        // Since susceptible and infected are "fixed,"
        // we only need to change recovered
        auto nrecovered = static_cast<size_t>(props[0] * tot_left * virus_count);
        auto ndeceased  = static_cast<size_t>(props[1] * tot_left * virus_count);
        
        AgentsSample<TSeq> sample_recover(
            *model,
            nrecovered,
            {0},
            true
        );

        // Setting up the initial states
        for (auto & agent : sample_recover) {
            agent->change_state(model, 2, Queue<TSeq>::NoOne);
        }

        AgentsSample<TSeq> sample_deceased(
            *model,
            ndeceased,
            {0},
            true
        );

        // Setting up the initial states
        for (auto & agent : sample_deceased) {
            agent->change_state(model, 3, Queue<TSeq>::NoOne);
        }
        
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
    std::vector< double > const & proportions
) {

    // Checking widths
    if (proportions.size() != 2) {
        throw std::invalid_argument("-proportions_- must have two entries.");
    }

    // proportions_ are values between 0 and 1, otherwise error
    for (const auto & prop : proportions) {
        if ((prop < 0.0) || (prop > 1.0)) {
            throw std::invalid_argument(
                "-proportions_- must have values between 0 and 1."
            );
        }
    }


    std::function<void(Model<TSeq>*)> fun = 
        [proportions] (Model<TSeq> * model) -> void {

        // Figuring out information about the viruses
        double tot = 0.0;
        auto virus_count   = static_cast<double>(model->size());
        for (const auto & agent: model->get_agents())
        {
            if (agent.get_virus() != nullptr) {
                tot += 1.0;
            }
        }

        tot /= virus_count;

        // Putting the total into context
        double tot_left = 1.0 - tot;

        // Since susceptible and infected are "fixed,"
        // we only need to change recovered
        auto nexposed   = proportions[0] * tot * virus_count;
        auto nrecovered = proportions[1] * tot_left * virus_count;
        
        AgentsSample<TSeq> sample_suscept(
            *model,
            nrecovered,
            {0},
            true
        );

        // Setting up the initial states
        for (auto & agent : sample_suscept) {
            agent->change_state(model, 3, Queue<TSeq>::NoOne);
        }

        AgentsSample<TSeq> sample_exposed(
            *model,
            nexposed,
            {1},
            true
        );

        // Setting up the initial states
        for (auto & agent : sample_exposed) {
            agent->change_state(model, 2, Queue<TSeq>::NoOne);
        }
        
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
    std::vector< double > proportions
) {

    // Checking widths
    if (proportions.size() != 3) {
        throw std::invalid_argument("-proportions_- must have three entries.");
    }

    // proportions_ are values between 0 and 1, otherwise error
    for (auto & prop : proportions) { {
        if ((prop < 0.0) || (prop > 1.0)) {
            throw std::invalid_argument(
                "-proportions_- must have values between 0 and 1."
            );
        }
    }

    // Last first two terms shouldn't add up to more than 1
    if ((proportions[1] + proportions[2]) > 1.0) {
        throw std::invalid_argument(
            "The last two terms of -proportions_- must add up to less than 1."
        );
    }

    std::function<void(Model<TSeq>*)> fun = 
        [proportions] (Model<TSeq> * model) -> void {

        // Figuring out information about the viruses
        double tot = 0.0;
        auto n_viruses   = static_cast<double>(model->size());

        for (const auto & agent: model->get_agents())
        {
            if (agent.get_virus() != nullptr) {
                tot += 1.0;
            }
        }

        tot /= n_viruses;

        // Putting the total into context
        double tot_left = 1.0 - tot;

        // Since susceptible and infected are "fixed,"
        // we only need to change recovered
        auto nexposed   = static_cast<size_t>(proportions[0] * tot * n_viruses);
        auto nrecovered = static_cast<size_t>(proportions[1] * tot_left * n_viruses);
        auto ndeceased  = static_cast<size_t>(proportions[2] * tot_left * n_viruses);
        
        AgentsSample<TSeq> sample_suscept(
            *model,
            nrecovered,
            {0},
            true
        );

        // Setting up the initial states
        for (auto & agent : sample_suscept) {
            agent->change_state(model, 3, Queue<TSeq>::NoOne);
        }

        AgentsSample<TSeq> sample_exposed(
            *model,
            nexposed,
            {1},
            true
        );

        // Setting up the initial states
        for (auto & agent : sample_exposed) {
            agent->change_state(model, 2, Queue<TSeq>::NoOne);
        }

        // Running the events
        model->events_run();

        // Setting the initial states for the deceased
        AgentsSample<TSeq> sample_deceased(
            *model,
            ndeceased,
            {0},
            true
        );
        
        // Setting up the initial states
        for (auto & agent : sample_deceased) {
            agent->change_state(model, 4, Queue<TSeq>::NoOne);
        }

        // Running the events
        model->events_run();

        return;

    };

    return fun;

}


#endif