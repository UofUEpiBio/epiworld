#ifndef EPIWORLD_MATH_LFMCMC_MEAT_HPP
#define EPIWORLD_MATH_LFMCMC_MEAT_HPP

#include <chrono>
#include <stdexcept>

#include "epiworld/math/lfmcmc/lfmcmc-bones.hpp"

namespace epiworld::math::lfmcmc {
/**
 * @brief Proposal function
 * @param new_params Vector where to save the new parameters.
 * @param old_params Vector of reference parameters.
 * @param m LFMCMC model.
 * @tparam TData 
 */
template<typename TData>
inline void proposal_fun_normal(
    std::vector< epiworld_double >& new_params,
    const std::vector< epiworld_double >& old_params,
    LFMCMC<TData>* model
) {

    for (size_t param = 0; param < model->get_n_params(); ++param) {
        new_params[param] = old_params[param] + model->rnorm();
    }
}

/**
 * @brief Factory for a reflective normal kernel
 * 
 * @details Reflective kernel corrects proposals by forcing them to be
 * within prespecified boundaries. 
 * 
 * @tparam TData 
 * @param scale Scale of the normal kernel
 * @param lb Lower bound (applies the same to all parameters)
 * @param ub Upper bound (applies the same to all parameters)
 * @return LFMCMCProposalFun<TData> 
 */
template<typename TData>
inline LFMCMCProposalFun<TData> make_proposal_norm_reflective(
    epiworld_double scale,
    epiworld_double lower_bound,
    epiworld_double upper_bound
) {

    LFMCMCProposalFun<TData> fun =
        [scale,lower_bound,upper_bound](
            std::vector< epiworld_double >& new_params,
            const std::vector< epiworld_double >& old_params,
            LFMCMC<TData>* model
        ) {

        // Making the proposal
        for (size_t param = 0; param < model->get_n_params(); ++param) {
            new_params[param] = old_params[param] + model->rnorm() * scale;
        }

        // Checking boundaries
        int odd = 0;
        epiworld_double delta = upper_bound - lower_bound;
        epiworld_double delta_above = NAN;
        epiworld_double delta_below = NAN;
        
        for (auto & param : new_params)
        {

            // Correcting if parameter goes above the upper bound
            if (param > upper_bound)
            {
                delta_above = param - upper_bound;
                odd     = static_cast<int>(std::floor(delta_above / delta)) % 2;
                delta_above = delta_above - std::floor(delta_above / delta) * delta;

                param = (lower_bound + delta_above) * static_cast<epiworld_double>(odd) +
                    (upper_bound - delta_above) * static_cast<epiworld_double>(1 - odd);

            // Correcting if parameter goes below upper bound
            } else if (param < lower_bound)
            {
                delta_below = lower_bound - param;
                int odd = static_cast<int>(std::floor(delta_below / delta)) % 2;
                delta_below = delta_below - std::floor(delta_below / delta) * delta;

                param = (upper_bound - delta_below) * static_cast<epiworld_double>(odd) +
                    (lower_bound + delta_below) * static_cast<epiworld_double>(1 - odd);
            }

        }

        #ifdef EPI_DEBUG
        for (auto & param : new_params) {
            if (param < lower_bound || param > upper_bound) {
                throw std::range_error("The parameter is out of bounds.");
            }
        }
        #endif

        return;
    };

    return fun;
}

/**
 * @brief Uniform proposal kernel
 * 
 * Proposals are made within a radious 1 of the current
 * state of the parameters.
 * 
 * @param new_params Where to write the new parameters
 * @param old_params Reference parameters
 * @tparam TData 
 * @param m LFMCMC model.
 */
template<typename TData>
inline void proposal_fun_unif(
    std::vector< epiworld_double >& new_params,
    const std::vector< epiworld_double >& old_params,
    LFMCMC<TData>* model
) {
    for (size_t param = 0; param < model->get_n_params(); ++param) {
        new_params[param] = (old_params[param] + model->runif(-1.0, 1.0));
    }
}

/**
 * @brief Uses the uniform kernel with euclidean distance
 * 
 * @param simulated_stats Vector of statistics based on 
 * simulated data
 * @param observed_stats Vector of observed statistics
 * @param epsilon Epsilon parameter
 * @param m LFMCMC model
 * @return epiworld_double 
 */
template<typename TData>
inline epiworld_double kernel_fun_uniform(
    const std::vector< epiworld_double >& simulated_stats,
    const std::vector< epiworld_double >& observed_stats,
    epiworld_double epsilon,
    LFMCMC<TData>* model
) {

    epiworld_double ans = 0.0;
    for (size_t param = 0; param < model->get_n_params(); ++param) {
        ans += static_cast<epiworld_double>(std::pow(observed_stats[param] - simulated_stats[param], 2));
    }

    return std::sqrt(ans) < epsilon ? 1.0 : 0.0;

}

constexpr epiworld_double sqrt2pi = 2.5066282746310002416;

/**
 * @brief Gaussian kernel
 * 
 * @tparam TData 
 * @param simulated_stats Vector of statistics based on 
 * simulated data
 * @param observed_stats Vector of observed statistics
 * @param epsilon Epsilon parameter
 * @param m LFMCMC model
 * @return epiworld_double 
 */
template<typename TData>
inline epiworld_double kernel_fun_gaussian(
    const std::vector< epiworld_double >& simulated_stats,
    const std::vector< epiworld_double >& observed_stats,
    epiworld_double epsilon,
    LFMCMC<TData>* model
) {

    epiworld_double ans = 0.0;
    for (size_t param = 0; param < model->get_n_params(); ++param) {
        ans += static_cast<epiworld_double>(std::pow(observed_stats[param] - simulated_stats[param], 2));
    }

    return static_cast<epiworld_double>(std::exp(
        -0.5 * (ans/std::pow(1 + std::pow(epsilon, 2)/3, 2)) // NOLINT
    ) / sqrt2pi);
}


template<typename TData>
inline void LFMCMC<TData>::set_proposal_fun(LFMCMCProposalFun<TData> fun)
{
    m_proposal_fun = fun;
}

template<typename TData>
inline void LFMCMC<TData>::set_simulation_fun(LFMCMCSimFun<TData> fun)
{
    m_simulation_fun = fun;
}

template<typename TData>
inline void LFMCMC<TData>::set_summary_fun(LFMCMCSummaryFun<TData> fun)
{
    m_summary_fun = fun;
}

template<typename TData>
inline void LFMCMC<TData>::set_kernel_fun(LFMCMCKernelFun<TData> fun)
{
    m_kernel_fun = fun;
}


template<typename TData>
inline void LFMCMC<TData>::run(
    const std::vector< epiworld_double > &params_init_,
    size_t n_samples_,
    epiworld_double epsilon_,
    int seed
    )
{

    // Starting timing
    chrono_start();

    // Setting the baseline parameters of the model
    m_n_samples    = n_samples_;
    m_epsilon      = epsilon_;
    m_initial_params  = params_init_;
    m_n_params = params_init_.size();

    if (seed >= 0) {
        this->seed = seed;
    }

    m_current_proposed_params.resize(m_n_params);
    m_current_accepted_params.resize(m_n_params);

    if (m_simulated_data != nullptr) {
        m_simulated_data->resize(m_n_samples);
    }

    m_current_accepted_params = m_initial_params;
    m_current_proposed_params  = m_initial_params;

    // Computing the baseline sufficient statistics
    m_summary_fun(m_observed_stats, m_observed_data, this);
    m_n_stats = m_observed_stats.size();

    // Reserving size
    m_current_proposed_stats.resize(m_n_stats);
    m_current_accepted_stats.resize(m_n_stats);
    m_all_sample_drawn_prob.resize(m_n_samples);
    m_all_sample_acceptance.resize(m_n_samples, false);
    m_all_sample_params.resize(m_n_samples * m_n_params);
    m_all_sample_stats.resize(m_n_samples * m_n_stats);
    m_all_sample_kernel_scores.resize(m_n_samples);

    m_all_accepted_params.resize(m_n_samples * m_n_params);
    m_all_accepted_stats.resize(m_n_samples * m_n_stats);
    m_all_accepted_kernel_scores.resize(m_n_samples);

    TData data_i = m_simulation_fun(m_initial_params, this);

    m_summary_fun(m_current_proposed_stats, data_i, this);
    m_all_accepted_kernel_scores[0] = m_kernel_fun(
        m_current_proposed_stats, m_observed_stats, m_epsilon, this
        );

    // Recording statistics
    for (size_t i = 0; i < m_n_stats; ++i) {
        m_all_sample_stats[i] = m_current_proposed_stats[i];
    }
    
    m_current_accepted_stats = m_current_proposed_stats;

    for (size_t k = 0; k < m_n_params; ++k) {
        m_all_accepted_params[k] = m_initial_params[k];
    }
    
    for (size_t k = 0; k < m_n_params; ++k) {
        m_all_sample_params[k] = m_initial_params[k];
    }
   
    // Init progress bar
    progress_bar = Progress(m_n_samples, EPIWORLD_PROGRESS_BAR_WIDTH);

    // Run LFMCMC
    for (size_t i = 1; i < m_n_samples; ++i)
    {
        // Step 1: Generate a proposal and store it in m_current_proposed_params
        m_proposal_fun(m_current_proposed_params, m_current_accepted_params, this);

        // Step 2: Using m_current_proposed_params, simulate data
        TData data_i = m_simulation_fun(m_current_proposed_params, this);

        // Are we storing the data?
        if (m_simulated_data != nullptr) {
            m_simulated_data->operator[](i) = data_i;
        }

        // Step 3: Generate the summary statistics of the data
        m_summary_fun(m_current_proposed_stats, data_i, this);

        // Step 4: Compute the hastings ratio using the kernel function
        epiworld_double hastings_ratio = m_kernel_fun(
            m_current_proposed_stats, m_observed_stats, m_epsilon, this
            );

        m_all_sample_kernel_scores[i] = hastings_ratio;

        // Storing data
        for (size_t k = 0; k < m_n_params; ++k) {
            m_all_sample_params[i * m_n_params + k] = m_current_proposed_params[k];
        }

        for (size_t k = 0; k < m_n_stats; ++k) {
            m_all_sample_stats[i * m_n_stats + k] = m_current_proposed_stats[k];
        }
        
        // Running Hastings ratio
        epiworld_double rand_accept = runif();
        m_all_sample_drawn_prob[i] = rand_accept;

        // Step 5: Update if likely
        if (rand_accept < std::min(static_cast<epiworld_double>(1.0), hastings_ratio / m_all_accepted_kernel_scores[i - 1]))
        {
            m_all_accepted_kernel_scores[i] = hastings_ratio;
            m_all_sample_acceptance[i]     = true;
            
            for (size_t k = 0; k < m_n_stats; ++k) {
                m_all_accepted_stats[i * m_n_stats + k] =
                    m_current_proposed_stats[k];
            }

            m_current_accepted_params = m_current_proposed_params;
            m_current_accepted_stats = m_current_proposed_stats;
        } else
        {

            for (size_t k = 0; k < m_n_stats; ++k) {
                m_all_accepted_stats[i * m_n_stats + k] =
                    m_all_accepted_stats[(i - 1) * m_n_stats + k];
            }

            m_all_accepted_kernel_scores[i] = m_all_accepted_kernel_scores[i - 1];
        }
            

        for (size_t k = 0; k < m_n_params; ++k) {
            m_all_accepted_params[i * m_n_params + k] = m_current_accepted_params[k];
        }

        if (verbose) {
           progress_bar.next();
        }
    }

    // End timing
    chrono_end();
}


template<typename TData>
inline epiworld_double LFMCMC<TData>::runif()
{
    return runifd->operator()(*m_engine);
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::runif(
    epiworld_double lower_bound,
    epiworld_double upper_bound
)
{
    return runifd->operator()(*m_engine) * (upper_bound - lower_bound) + lower_bound;
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::rnorm()
{
    return rnormd->operator()(*m_engine);
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::rnorm(
    epiworld_double mean,
    epiworld_double std
    )
{
    return (rnormd->operator()(*m_engine)) * std + mean;
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::rgamma()
{
    return rgammad->operator()(*m_engine);
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::rgamma(
    epiworld_double alpha,
    epiworld_double beta
    )
{

    auto old_param = rgammad->param();

    rgammad->param(std::gamma_distribution<>::param_type(alpha, beta));

    epiworld_double ans = rgammad->operator()(*m_engine);

    rgammad->param(old_param);

    return ans;

}

template<typename TData>
inline void LFMCMC<TData>::seed(epiworld_fast_uint seed) {

    this->m_engine->seed = seed;

}

template<typename TData>
inline void LFMCMC<TData>::set_rand_engine(std::shared_ptr< std::mt19937 > & eng)
{
    m_engine = eng;
}

template<typename TData>
inline void LFMCMC<TData>::set_rand_gamma(epiworld_double alpha, epiworld_double beta)
{
    rgammad = std::make_shared<std::gamma_distribution<>>(alpha,beta);
}

template<typename TData>
inline std::shared_ptr< std::mt19937 > & LFMCMC<TData>::get_rand_endgine()
{
    return m_engine;
}

// Step 1: Simulate data

// Step 2: Compute the sufficient statistics

// Step 3: Compute the hastings-ratio

// Step 4: Accept/reject, and go back to step 1

template <typename Duration>
inline epiworld_double durcast_fn(
    const std::chrono::steady_clock::time_point& start,
    const std::chrono::steady_clock::time_point& end
) {
    epiworld_double elapsed = std::chrono::duration_cast<Duration>(end - start).count();
    
    return elapsed;
}

template<typename TData>
inline void LFMCMC<TData>::get_elapsed_time(
    std::string const& unit,
    epiworld_double* last_elapsed,
    std::string* unit_abbr,
    bool print
) const {

    // Determine unit
    std::string chosen_unit = unit;
    if (unit == "auto") {
        const auto nanosecond_boundary = 1e3L;
        const auto microsecond_boundary = 1e6L;
        const auto millisecond_boundary = 1e9L;
        const auto second_boundary = 60 * 1e9L;
        const auto minute_boundary = 3600 * 1e9L;

        auto ns_count = std::chrono::duration_cast<std::chrono::nanoseconds>(m_end_time - m_start_time).count();

        if (ns_count < nanosecond_boundary) { chosen_unit = "nanoseconds"; }
        else if (ns_count < microsecond_boundary) { chosen_unit = "microseconds"; }
        else if (ns_count < millisecond_boundary) { chosen_unit = "milliseconds"; }
        else if (ns_count < second_boundary) { chosen_unit = "seconds"; }
        else if (ns_count < minute_boundary) { chosen_unit = "minutes"; }
        else { chosen_unit = "hours"; }
    }

    struct UnitInfo {
        epiworld_double (*cast_fn)(const std::chrono::steady_clock::time_point&, const std::chrono::steady_clock::time_point&);
        const char* abbr;
    };

    static const std::unordered_map<std::string, UnitInfo> unit_map = {
        {"nanoseconds", {[](auto start, auto end) { return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count(); }, "ns"}},
        {"microseconds", {[](auto start, auto end) { return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count(); }, "\xC2\xB5s"}},
        {"milliseconds", {[](auto start, auto end) { return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(); }, "ms"}},
        {"seconds",      {[](auto start, auto end) { return std::chrono::duration_cast<std::chrono::seconds>(end - start).count(); }, "s"}},
        {"minutes",      {[](auto start, auto end) { return std::chrono::duration_cast<std::chrono::minutes>(end - start).count(); }, "m"}},
        {"hours",        {[](auto start, auto end) { return std::chrono::duration_cast<std::chrono::hours>(end - start).count(); }, "h"}}
    };

    auto unit_it = unit_map.find(chosen_unit);
    if (unit_it == unit_map.end()) {
        throw std::range_error("The time unit " + chosen_unit + " is not supported.");
    }

    epiworld_double elapsed = unit_it->second.cast_fn(m_start_time, m_end_time);
    std::string abbr = unit_it->second.abbr;

    if (last_elapsed != nullptr) { *last_elapsed = elapsed; }
    if (unit_abbr != nullptr) { *unit_abbr = abbr; }

    if (print) {
        printf_epiworld("Elapsed time : %.2f%s.\n", elapsed, abbr.c_str());
    }
}

template<typename TData>
inline void LFMCMC<TData>::chrono_start() {
    m_start_time = std::chrono::steady_clock::now();
}

template<typename TData>
inline void LFMCMC<TData>::chrono_end() {
    m_end_time = std::chrono::steady_clock::now();
    m_elapsed_time += (m_end_time - m_start_time);
}

template<typename TData>
inline void LFMCMC<TData>::set_params_names(const std::vector< std::string > &names)
{

    if (names.size() != m_n_params) {
        throw std::length_error("The number of names to add differs from the number of parameters in the model.");
    }

    m_param_names = names;

}
template<typename TData>
inline void LFMCMC<TData>::set_stats_names(const std::vector< std::string > &names)
{

    if (names.size() != m_n_stats) {
        throw std::length_error("The number of names to add differs from the number of statistics in the model.");
    }

    m_stat_names = names;

}

template<typename TData>
inline std::vector< epiworld_double > LFMCMC<TData>::get_mean_params()
{
    std::vector< epiworld_double > res(this->m_n_params, 0.0);
    
    for (size_t k = 0; k < m_n_params; ++k)
    {
        for (size_t i = 0; i < m_n_samples; ++i) {
            res[k] += (this->m_all_accepted_params[k + m_n_params * i])/
                static_cast< epiworld_double >(m_n_samples);
        }
    }

    return res;

}

template<typename TData>
inline std::vector< epiworld_double > LFMCMC<TData>::get_mean_stats()
{
    std::vector< epiworld_double > res(this->m_n_stats, 0.0);
    
    for (size_t k = 0; k < m_n_stats; ++k)
    {
        for (size_t i = 0; i < m_n_samples; ++i) {
            res[k] += (this->m_all_accepted_stats[k + m_n_stats * i])/
                static_cast< epiworld_double >(m_n_samples);
        }
    }

    return res;

}

template<typename TData>
inline LFMCMC<TData> & LFMCMC<TData>::verbose_off()
{
    verbose = false;
    return *this;
}

template<typename TData>
inline LFMCMC<TData> & LFMCMC<TData>::verbose_on()
{
    verbose = true;
    return *this;
}
}

#endif /* !EPIWORLD_MATH_LFMCMC_MEAT_HPP */