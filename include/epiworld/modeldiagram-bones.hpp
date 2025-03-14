#ifndef EPIWORLD_MODELDIAGRAM_HPP
#define EPIWORLD_MODELDIAGRAM_HPP

class ModelDiagram {
private:
    #ifdef EPI_DEBUG
    std::vector< int > thread;
    #endif

    std::vector< std::string > from;
    std::vector< std::string > to;
    std::vector< int > counts;

    std::vector< std::string > states;
    std::vector< epiworld_double > tprob;

    void draw_mermaid(
        std::string fn_output = "",
        bool self = false
    );


    int n_runs = 0; ///< The number of runs included in the diagram.

public:
    ModelDiagram() {};

    /**
     * @brief Reads the transitions from a file.
     * @details
     * The file is assumed to come from the output of [Model::write_data()],
     * and should be in the following format:
     * ```
     * 0 "S" "I" 10
     * 1 "I" "R" 5
     * 2 "R" "S" 3
     * ...
     * ```
     * The first column is the step, the second column is the state from which
     * the transition is made, the third column is the state to which the
     * transition is made, and the fourth column is the number of transitions.
     * 
     * @param fn_transition The name of the file to read.
     * @return void
     * @throws std::runtime_error if the file cannot be opened.
     */
    void read_transitions(
        const std::string & fn_transition
    );

    /**
     * @brief In the case of multiple files (like from run_multiple)
     * @param fns_transition The names of the files to read.
     * @details
     * It will read the transitions from multiple files and concatenate them.
     * into the same object.
     */
    void read_transitions(
        const std::vector< std::string > & fns_transition
    );
    
    /**
     * @brief Computes the transition probability matrix.
     * @param normalize Whether to compute only the counts,
     * otherwise the probabilities will be computed (row-stochastic).
     * @return void
     */
    void transition_probability(bool normalize = true);

    void draw(
        const std::string & fn_transition,
        const std::string & fn_output = "",
        bool self = false
    );

    void set_states_and_tprob(
        const std::vector< std::string > & states,
        const std::vector< epiworld_double > & tprob
    );

};

#endif