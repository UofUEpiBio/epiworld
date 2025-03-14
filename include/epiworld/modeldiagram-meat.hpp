#ifndef EPIWORLD_MODELDIAGRAM_MEAT_HPP
#define EPIWORLD_MODELDIAGRAM_MEAT_HPP

inline void ModelDiagram::read_transitions(
    const std::string & fn_transition
) {

    // Checking if the file exists
    std::ifstream file(fn_transition);

    if (!file.is_open())
        throw std::runtime_error(
            "Could not open the file " + 
            fn_transition + " for reading."
        );

    // Reading the data
    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        #ifdef EPI_DEBUG
        int t;
        iss >> t;
        thread.push_back(t);
        #endif

        int i_;
        std::string from_, to_;
        int counts_;

        iss >> i_;

        // Read the quoted strings
        iss >> std::quoted(from_) >> std::quoted(to_);

        // Read the integer
        iss >> counts_;

        from.push_back(from_);
        to.push_back(to_);
        counts.push_back(counts_);
    }

    // Incrementing the number of runs
    this->n_runs++;

}

inline void ModelDiagram::read_transitions(
    const std::vector< std::string > & fns_transition
)
{

    for (const auto & fn: fns_transition)
        this->read_transitions(fn);

    return;

}

inline void ModelDiagram::transition_probability(
    bool normalize
)
{

    // Generating the map of states
    std::map< std::pair< std::string, std::string >, int > map;
    std::set< std::string > states_set;

    for (size_t i = 0u; i < counts.size(); ++i)
    {
        states_set.insert(from[i]);
        states_set.insert(to[i]);

        auto key = std::make_pair(from[i], to[i]);
        if (map.find(key) == map.end())
            map[key] = counts[i];
        else
            map[key] += counts[i];
    }

    // Generating the transition matrix
    size_t n_states = states.size();
    std::vector< epiworld_double > tprob(n_states * n_states, 0.0);

    std::vector< epiworld_double > rowsum(n_states, 0.0);
    for (size_t i = 0; i < n_states; ++i)
    {

        for (size_t j = 0; j < n_states; ++j)
        {

            auto key = std::make_pair(states[i], states[j]);
            if (map.find(key) != map.end())
                tprob[i + j * n_states] = static_cast<epiworld_double>(
                    map[key]
                );

            if (normalize)
                rowsum[i] += tprob[i + j * n_states];

        }

    }
    

}

inline void ModelDiagram::draw_mermaid(
    std::string fn_output,
    bool self
)
{

    std::vector< std::string > states_ids;
    for (size_t i = 0u; i < states.size(); ++i)
        states_ids.push_back("s" + std::to_string(i));

    std::string graph = "flowchart LR\n";

    // Declaring the states
    for (size_t i = 0u; i < states.size(); ++i)
    {
        graph += "\t" + states_ids[i] + "[" + states[i] + "]\n";
    }

    // Adding the transitions
    size_t n_states = states.size();
    for (size_t i = 0u; i < states.size(); ++i)
    {
        for (size_t j = 0u; j < states.size(); ++j)
        {
            if (!self && i == j)
                continue;

            if (tprob[i + j * n_states] > 0.0)
            {
                graph += "\t" + states_ids[i] + " -->|" + 
                    std::to_string(tprob[i + j * n_states]) + "| " + states_ids[j] + "\n";
            }
        }
    }

    if (fn_output != "")
    {
        std::ofstream file(fn_output);

        if (!file.is_open())
            throw std::runtime_error(
                "Could not open the file " +
                fn_output + 
                " for writing."
            );

        file << graph;
        file.close();
        
    } else {
        printf_epiworld("%s\n", graph.c_str());
    }

    return;

}

inline void ModelDiagram::draw(
    const std::string & fn_transition,
    const std::string & fn_output,
    bool self
) {

    if (fn_transition != "")
    {
        // Loading the transition file
        this->read_transitions(fn_transition);

        // Computing the transition probability
        this->transition_probability();
    }

    // Actually drawing the diagram
    this->draw_mermaid(fn_output, self);

    return;

    
}

inline void ModelDiagram::set_states_and_tprob(
    const std::vector< std::string > & states,
    const std::vector< epiworld_double > & tprob
) {
    
    this->states = states;
    this->tprob = tprob;

    return;
}

#endif