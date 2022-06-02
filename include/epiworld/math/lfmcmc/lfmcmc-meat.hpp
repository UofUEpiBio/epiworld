#ifndef EPIWORLD_LFMCMC_MEAT_HPP
#define EPIWORLD_LFMCMC_MEAT_HPP

#include "lfmcmc-bones.hpp"

#ifndef epiworld_double
    #define epiworld_double float
#endif

/**
 * @brief Proposal function
 * @param params_now Vector where to save the new parameters.
 * @param params_prev Vector of reference parameters.
 * @param m LFMCMC model.
 * @tparam TData 
 */
template<typename TData>
inline void proposal_fun_normal(
    std::vector< epiworld_double >& params_now,
    const std::vector< epiworld_double >& params_prev,
    LFMCMC<TData>* m
) {

    for (size_t p = 0u; p < m->get_n_parameters(); ++p)
        params_now[p] = params_prev[p] + m->rnorm();

    return;
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
    epiworld_double lb,
    epiworld_double ub
) {

    LFMCMCProposalFun<TData> fun =
        [scale,lb,ub](
            std::vector< epiworld_double >& params_now,
            const std::vector< epiworld_double >& params_prev,
            LFMCMC<TData>* m
        ) {

        // Making the proposal
        for (size_t p = 0u; p < m->get_n_parameters(); ++p)
            params_now[p] = params_prev[p] + m->rnorm() * scale;

        // Checking boundaries
        epiworld_double d = ub - lb;
        int odd;
        epiworld_double d_above, d_below;
        for (auto & p : params_now)
        {

            // Correcting if parameter goes above the upper bound
            if (p > ub)
            {
                d_above = p - ub;
                odd     = static_cast<int>(std::floor(d_above / d)) % 2;
                d_above = d_above - std::floor(d_above / d) * d;

                p = (lb + d_above) * odd +
                    (ub - d_above) * (1 - odd);

            // Correcting if parameter goes below upper bound
            } else if (p < lb)
            {
                d_below = lb - p;
                int odd = static_cast<int>(std::floor(d_below / d)) % 2;
                d_below = d_below - std::floor(d_below / d) * d;

                p = (ub - d_below) * odd +
                    (lb + d_below) * (1 - odd);
            }

        }

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
 * @param params_now Where to write the new parameters
 * @param params_prev Reference parameters
 * @tparam TData 
 * @param m LFMCMC model.
 */
template<typename TData>
inline void proposal_fun_unif(
    std::vector< epiworld_double >& params_now,
    const std::vector< epiworld_double >& params_prev,
    LFMCMC<TData>* m
) {

    for (size_t p = 0u; p < m->get_n_parameters(); ++p)
        params_now[p] = (params_prev[p] + m->runif(-1.0, 1.0));

    return;
}

/**
 * @brief Uses the uniform kernel with euclidean distance
 * 
 * @param stats_now Vector of current statistics based on 
 * simulated data.
 * @param stats_obs Vector of observed statistics
 * @param epsilon Epsilon parameter
 * @param m LFMCMC model.
 * @return epiworld_double 
 */
template<typename TData>
inline epiworld_double kernel_fun_uniform(
    const std::vector< epiworld_double >& stats_now,
    const std::vector< epiworld_double >& stats_obs,
    epiworld_double epsilon,
    LFMCMC<TData>* m
) {

    epiworld_double ans = 0.0;
    for (size_t p = 0u; p < m->get_n_parameters(); ++p)
        ans += std::pow(stats_obs[p] - stats_now[p], 2.0);

    return std::sqrt(ans) < epsilon ? 1.0 : 0.0;

}

/**
 * @brief Gaussian kernel
 * 
 * @tparam TData 
 * @param epsilon 
 * @param m 
 * @return epiworld_double 
 */
template<typename TData>
inline epiworld_double kernel_fun_gaussian(
    const std::vector< epiworld_double >& stats_now,
    const std::vector< epiworld_double >& stats_obs,
    epiworld_double epsilon,
    LFMCMC<TData>* m
) {

    epiworld_double ans = 0.0;
    for (size_t p = 0u; p < m->get_n_parameters(); ++p)
        ans += std::pow(stats_obs[p] - stats_now[p], 2.0);

    return std::exp(-.5 * (ans/std::pow(1 + std::pow(epsilon, 2.0)/3.0, 2.0)))/std::sqrt(2.0 * 3.141593);

}


template<typename TData>
inline void LFMCMC<TData>::set_proposal_fun(LFMCMCProposalFun<TData> fun)
{
    proposal_fun = fun;
}

template<typename TData>
inline void LFMCMC<TData>::set_simulation_fun(LFMCMCSimFun<TData> fun)
{
    simulation_fun = fun;
}

template<typename TData>
inline void LFMCMC<TData>::set_summary_fun(LFMCMCSummaryFun<TData> fun)
{
    summary_fun = fun;
}

template<typename TData>
inline void LFMCMC<TData>::set_kernel_fun(LFMCMCKernelFun<TData> fun)
{
    kernel_fun = fun;
}


template<typename TData>
inline void LFMCMC<TData>::run(
    std::vector< epiworld_double > params_init_,
    size_t n_samples_,
    epiworld_double epsilon_
    )
{

    // Setting the baseline parameters of the model
    n_samples    = n_samples_;
    epsilon      = epsilon_;
    params_init  = params_init_;
    n_parameters = params_init_.size();

    params_now.resize(n_parameters);
    params_prev.resize(n_parameters);

    if (sampled_data != nullptr)
        sampled_data->resize(n_samples);

    params_prev = params_init;
    params_now  = params_init;

    // Computing the baseline sufficient statistics
    summary_fun(observed_stats, *observed_data, this);
    n_statistics = observed_stats.size();

    // Reserving size
    drawn_prob.resize(n_samples);
    sampled_accepted.resize(n_samples, false);
    sampled_stats.resize(n_samples * n_statistics);
    sampled_stats_prob.resize(n_samples);

    accepted_params.resize(n_samples * n_parameters);
    accepted_stats.resize(n_samples * n_statistics);
    accepted_params_prob.resize(n_samples);

    TData data_i = simulation_fun(params_init, this);

    std::vector< epiworld_double > proposed_stats_i;
    summary_fun(proposed_stats_i, data_i, this);
    accepted_params_prob[0u] = kernel_fun(proposed_stats_i, observed_stats, epsilon, this);

    // Recording statistics
    for (size_t i = 0u; i < n_statistics; ++i)
        sampled_stats[i] = proposed_stats_i[i];

    for (size_t k = 0u; k < n_statistics; ++k)
        accepted_params[k] = proposed_stats_i[k];
   
    for (size_t i = 1u; i < n_samples; ++i)
    {
        // Step 1: Generate a proposal and store it in params_now
        proposal_fun(params_now, params_prev, this);

        // Step 2: Using params_now, simulate data
        TData data_i = simulation_fun(params_now, this);

        // Are we storing the data?
        if (sampled_data != nullptr)
            sampled_data->operator[](i) = data_i;

        // Step 3: Generate the summary statistics of the data
        summary_fun(proposed_stats_i, data_i, this);

        // Step 4: Compute the hastings ratio using the kernel function
        double hr = kernel_fun(proposed_stats_i, observed_stats, epsilon, this);
        sampled_stats_prob[i] = hr;

        // Storing data
        for (size_t k = 0u; k < n_statistics; ++k)
            sampled_stats[i * n_statistics + k] = proposed_stats_i[k];
        
        // Running Hastings ratio
        double r      = runif();
        drawn_prob[i] = r;

        // Step 5: Update if likely
        if (r < std::min(1.0, hr / accepted_params_prob[i - 1u]))
        {
            accepted_params_prob[i] = hr;
            sampled_accepted[i]     = true;
            
            for (size_t k = 0u; k < n_statistics; ++k)
                accepted_stats[i * n_statistics + k] =
                    proposed_stats_i[k];

            params_prev = params_now;

        } else
        {

            for (size_t k = 0u; k < n_statistics; ++k)
                accepted_stats[i * n_statistics + k] =
                    accepted_stats[(i - 1) * n_statistics + k];

            accepted_params_prob[i] = accepted_params_prob[i - 1u];
        }
            

        for (size_t k = 0u; k < n_parameters; ++k)
            accepted_params[i * n_parameters + k] = params_prev[k];

    }

}


template<typename TData>
inline epiworld_double LFMCMC<TData>::runif()
{
    return runifd->operator()(*engine);
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::runif(
    epiworld_double lb,
    epiworld_double ub
)
{
    return runifd->operator()(*engine) * (ub - lb) + lb;
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::rnorm()
{
    return rnormd->operator()(*engine);
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::rnorm(
    epiworld_double mean,
    epiworld_double sd
    )
{
    return (rnormd->operator()(*engine)) * sd + mean;
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::rgamma()
{
    return rgammad->operator()(*engine);
}

template<typename TData>
inline epiworld_double LFMCMC<TData>::rgamma(
    epiworld_double alpha,
    epiworld_double beta
    )
{

    auto old_param = rgammad->param();

    rgammad->param(std::gamma_distribution<>::param_type(alpha, beta));

    epiworld_double ans = rgammad->operator()(*engine);

    rgammad->param(old_param);

    return ans;

}

template<typename TData>
inline void LFMCMC<TData>::seed(unsigned int s) {

    this->engine->seed(s);

}

template<typename TData>
inline void LFMCMC<TData>::set_rand_engine(std::mt19937 & eng)
{
    engine = std::make_shared< std::mt19937 >(eng);
}

template<typename TData>
inline void LFMCMC<TData>::set_rand_gamma(epiworld_double alpha, epiworld_double beta)
{
    rgammad = std::make_shared<std::gamma_distribution<>>(alpha,beta);
}

template<typename TData>
inline std::mt19937 * LFMCMC<TData>::get_rand_endgine()
{
    return engine.get();
}

// Step 1: Simulate data

// Step 2: Compute the sufficient statistics

// Step 3: Compute the hastings-ratio

// Step 4: Accept/reject, and go back to step 1

template<typename TData>
inline void LFMCMC<TData>::summary() const
{
    std::vector< epiworld_double > summ_params(n_parameters * 3, 0.0);
    std::vector< epiworld_double > summ_stats(n_statistics * 3, 0.0);

    for (size_t k = 0u; k < n_parameters; ++k)
    {

        // Retrieving the relevant parameter
        std::vector< epiworld_double > par_i(n_samples);
        for (size_t i = 0u; i < n_samples; ++i)
        {
            par_i[i] = accepted_params[i * n_parameters + k];
            summ_params[k * 3] += par_i[i]/n_samples;
        }

        // Computing the 95% Credible interval
        std::sort(par_i.begin(), par_i.end());

        summ_params[k * 3 + 1u] = 
            par_i[std::floor(.025 * static_cast<epiworld_double>(n_samples))];
        summ_params[k * 3 + 2u] = 
            par_i[std::floor(.975 * static_cast<epiworld_double>(n_samples))];

    }

    for (size_t k = 0u; k < n_statistics; ++k)
    {

        // Retrieving the relevant parameter
        std::vector< epiworld_double > stat_k(n_samples);
        for (size_t i = 0u; i < n_samples; ++i)
        {
            stat_k[i] = accepted_stats[i * n_statistics + k];
            summ_stats[k * 3] += stat_k[i]/n_samples;
        }

        // Computing the 95% Credible interval
        std::sort(stat_k.begin(), stat_k.end());

        summ_stats[k * 3 + 1u] = 
            stat_k[std::floor(.025 * static_cast<epiworld_double>(n_samples))];
        summ_stats[k * 3 + 2u] = 
            stat_k[std::floor(.975 * static_cast<epiworld_double>(n_samples))];

    }

    printf_epiworld("___________________________________________\n\n");
    printf_epiworld("LIKELIHOOD-FREE MARKOV CHAIN MONTE CARLO\n\n");

    printf_epiworld("Parameters:\n");
    for (size_t k = 0u; k < n_parameters; ++k)
    {
        printf_epiworld(
            "  [%-2ld]: % 4.2f [% 4.2f, % 4.2f] (initial : % 4.2f)\n",
            k,
            summ_params[k * 3],
            summ_params[k * 3 + 1u],
            summ_params[k * 3 + 2u],
            params_init[k]
            );
    }

    printf_epiworld("\nStatistics:\n");
    for (size_t k = 0u; k < n_statistics; ++k)
    {
        printf_epiworld(
            "  [%-2ld]: % 4.2f [% 4.2f, % 4.2f] (Observed: % 4.2f)\n",
            k,
            summ_stats[k * 3],
            summ_stats[k * 3 + 1u],
            summ_stats[k * 3 + 2u],
            observed_stats[k]
            );
    }

    printf_epiworld("___________________________________________\n\n");
}

template<typename TData>
inline void LFMCMC<TData>::set_par_names(std::vector< std::string > names)
{

}
template<typename TData>
inline void LFMCMC<TData>::set_stats_names(std::vector< std::string > names)
{
    
}

#endif