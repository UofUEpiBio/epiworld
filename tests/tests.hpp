#ifndef EPIWORLD_TESTS_HPP
#define EPIWORLD_TESTS_HPP


#include <vector>
#include <iostream>
#include <type_traits>
#include <exception>
#if defined(_OPENMP)
    #include <omp.h>
#endif
#include "../include/epiworld/epiworld.hpp"



/**
 * Returns true if the absolute difference between a and b is greater than eps.
 */
template<typename T=epiworld_double>
inline bool moreless(T a, T b, T eps = static_cast<T>(1))
{
    return(std::abs(a-b) > eps);
}

std::string file_reader(std::string fname)
{
    // Create a text string, which is used to output the text file
    std::string myText, res;

    // Read from the text file
    std::ifstream MyReadFile(fname);

    // Check if fname exists and is readable. If not, throw an exception
    if (!MyReadFile.good())
    {
        throw std::runtime_error("File " + fname + " does not exist or is not readable");
    }

    // Use a while loop together with the getline() function to read the file line by line
    while (getline (MyReadFile, myText)) {
        // Output the text from the file
        res += myText;
    }

    // Close the file
    MyReadFile.close();

    return res;
}

template<typename TSeq = EPI_DEFAULT_TSEQ>
inline epiworld::EntityToAgentFun<TSeq> dist_factory(int from, int to) {
    return [from, to](epiworld::Entity<TSeq> & e, epiworld::Model<TSeq> * m) -> void {

            auto & agents = m->get_agents();
            for (int i = from; i < to; ++i)
            {
                e.add_agent(&agents[i], m);
            }
            
            return;

        };
}

template<typename TSeq = EPI_DEFAULT_TSEQ>
inline epiworld::VirusToAgentFun<TSeq> dist_virus(int i)
{
    return [i](epiworld::Virus<TSeq> & v, epiworld::Model<TSeq> * m) -> void {

            m->get_agents()[i].set_virus(v, m);
            return;

        };

}

/**
 * @brief Create a saver function for the model that saves transition probabilities
 * and R0 values.
 * @tparam ModelType The type of the model.
 * @param transitions A vector of vectors to store transition probabilities.
 * @param R0s A vector to store R0 values.
 * @param n_seeds The number of initial cases.
 * @return A function that takes an index and a model pointer, saving the
 * transition probabilities and R0 values for that model instance.
 */
inline std::function<void(size_t, epiworld::Model<>*)> tests_create_saver(
    std::vector<std::vector<epiworld_double>>& transitions,
    std::vector<epiworld_double>& R0s,
    int n_seeds,
    std::vector<std::vector<int>>* final_distribution = nullptr,
    std::vector< double > * outbreak_sizes = nullptr
) {
    return [&transitions, &R0s, n_seeds, final_distribution, outbreak_sizes](size_t n, epiworld::Model<>* m) -> void {
        // Saving the transition probabilities
        transitions[n] = m->get_db().get_transition_probability(false, false);

        // Recording the R0 from the index case
        auto rts = m->get_db().get_reproductive_number();
        int i = 0;
        for (auto & rt: rts) {
            if (
                // Cannot be the model (the source)
                (rt.first[1] >= 0) &&
                // The exposure date is zero
                (rt.first[2] == 0)
            )
                R0s[n_seeds * n + i++] = static_cast<epiworld_double>(rt.second);
        }

        if (i != n_seeds) {
            throw std::runtime_error(
                "The number of seeds (" + std::to_string(n_seeds) +
                ") does not match the number of index cases (" +
                std::to_string(i) + ")."
            );
        }
        
        if (outbreak_sizes != nullptr)
        {
            std::vector< int > date,virus,outbreak;
            m->get_db().get_outbreak_size(date, virus, outbreak);

            outbreak_sizes->at(n) = outbreak.back();
        }

        // Should we get the final distribution?
        if (final_distribution == nullptr)
            return;
        
        m->get_db().get_today_total(
            nullptr,
            &(final_distribution->at(n))
        );
    };
};

/**
 * Computes the average and range of final outbreak sizes
 * 
 * This can be used to check the final outbreak sizes
 * after running a model with multiple simulations.
 * @param final_distribution The final distribution of the model. As generated
 * by the `tests_create_saver` function.
 * @param not_infected_states The states that are considered "not infected".
 * @param nsims The number of simulations.
 * @return A vector containing the final outbreak sizes for each simulation.
 */
inline std::vector< double > test_compute_final_sizes(
    const std::vector<std::vector<int>>& final_distribution,
    std::vector<size_t> not_infected_states,
    size_t nsims,
    bool print = true
) {

    // Looking at the final outbreak size
    std::vector< double > outbreak_sizes(nsims, 0.0);
    size_t n_states = final_distribution[0].size();
    for (size_t i = 0; i < nsims; ++i)
    {
        for (size_t j = 0; j < n_states; ++j)
        {

            // We only count the states that are not considered "not infected"
            if (
                std::find(
                    not_infected_states.begin(),
                    not_infected_states.end(),
                    j) ==
                not_infected_states.end()
            )
            {
                outbreak_sizes[i] += final_distribution[i][j];
            }
        }
    }

    // Sorting the outbreak sizes
    std::sort(outbreak_sizes.begin(), outbreak_sizes.end());

    // Identifying the 95% CI
    size_t lower_bound = static_cast<size_t>(0.025 * nsims);
    size_t upper_bound = static_cast<size_t>(0.975 * nsims);
    size_t mid_point   = static_cast<size_t>(0.5 * nsims);

    // Filling the 95% CI
    std::vector< double > res(3, 0.0);
    res[0] = std::accumulate(
        outbreak_sizes.begin(), outbreak_sizes.end(), 0.0
    ) / static_cast<double>(nsims);
    res[1] = outbreak_sizes[lower_bound];
    res[2] = outbreak_sizes[upper_bound];
    
    if (print)
    {
        printf_epiworld(
            "Final size (%i): %.2f [%.2f, %.2f] (median: %.2f)\n",
            static_cast<int>(nsims), res[0], res[1], res[2], outbreak_sizes[mid_point]
        );

    }

    return res;
}

/**
 * @brief Calculate the average transition probabilities from a vector of transitions.
 * @param transitions A vector of vectors containing transition counts
 * extracted from multiple simulations.
 * @param model The model used to calculate the average transitions.
 * @return A vector containing the average transition probabilities.
 */
template<typename ModelType>
inline std::vector<epiworld_double> tests_calculate_avg_transitions(
    const std::vector<std::vector<epiworld_double>>& transitions,
    const ModelType & model,
    std::vector< double > * outbreak_sizes = nullptr
) {
    if (transitions.empty())
        return {};
        
    // Creating an average across the transitions vectors
    std::vector<epiworld_double> avg_transitions(transitions[0].size(), 0.0);
    for (size_t i = 0; i < transitions.size(); ++i)
    {
        for (size_t j = 0; j < transitions[i].size(); ++j)
        {
            avg_transitions[j] += transitions[i][j];
        }
    }
    
    // Normalizing the average
    auto n_states = model.get_n_states();
    for (size_t i = 0; i < n_states; ++i)
    {
        double rowsums = 0.0;
        for (size_t j = 0; j < n_states; ++j)
        {
            rowsums += avg_transitions[j * n_states + i];
        }

        // Normalizing the rows
        // If the row is empty, we skip it
        if (rowsums == 0.0)
            continue;

        for (size_t j = 0; j < n_states; ++j)
        {
            avg_transitions[j * n_states + i] /= rowsums;
        }
    }
    
    return avg_transitions;
};

template<typename ModelType>
void inline tests_print_avg_transitions(
    const std::vector<epiworld_double> & avg_transitions,
    const ModelType & model
) {
    auto states = model.get_states();
    size_t n_states = states.size();

    // Printing the entry as a matrix
    std::cout << "Average transitions: " << std::endl;
    for (size_t i = 0; i < n_states; ++i)
    {

        // First prints the column numbers
        if (i == 0u)
        {
            printf_epiworld("%32s", " ");
            for (size_t j = 0; j < n_states; ++j)
            {
                printf_epiworld(" [,%2li]", j);
            }
            printf_epiworld("\n");
        }

        printf_epiworld(" %-25s[%-2li,] ", states[i].c_str(), i);
        for (size_t j = 0; j < n_states; ++j)
        {
            if (avg_transitions[j*n_states + i] == 0.0)
            {
                printf_epiworld("  -   ");
                continue;
            }

            printf_epiworld("%5.2f ", avg_transitions[j*n_states + i]);
        }
        printf_epiworld("\n");
    }
}

#ifndef CATCH_CONFIG_MAIN

    
    #define EPIWORLD_TEST_CASE(desc, tag) \
        int main() 

    
#else

    #define EPIWORLD_TEST_CASE(desc, tag) \
        TEST_CASE(desc, tag) 

#endif

#endif