#ifndef EPIWORLD_LFMCMC_HPP
#define EPIWORLD_LFMCMC_HPP

#ifndef epiworld_double
    #define epiworld_double float
#endif

#define VEC(a) std::vector< a >
#define FUN std::function

template<typename TData>
class LFMCMC;

template<typename TData>
using LFMCMCSimFun = std::function<TData(const VEC(epiworld_double)&,LFMCMC<TData>*)>;

template<typename TData>
using LFMCMCSummaryFun = std::function<void(VEC(epiworld_double)&,const TData&,LFMCMC<TData>*)>;

template<typename TData>
using LFMCMCProposalFun = std::function<void(VEC(epiworld_double)&,const VEC(epiworld_double)&,LFMCMC<TData>*)>;

template<typename TData>
using LFMCMCKernelFun = std::function<epiworld_double(const VEC(epiworld_double)&,const VEC(epiworld_double)&,epiworld_double,LFMCMC<TData>*)>;

/**
 * @brief Proposal function
 * @param params_now Vector where to save the new parameters.
 * @param params_prev Vector of reference parameters.
 * @param m LFMCMC model.
 * @tparam TData 
 */
template<typename TData>
inline void proposal_fun_normal(
    VEC( epiworld_double )& params_now,
    const VEC( epiworld_double )& params_prev,
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
    epiworld_double lb = std::numeric_limits<epiworld_double>::min(),
    epiworld_double ub = std::numeric_limits<epiworld_double>::max()
) {

    LFMCMCProposalFun<TData> fun =
        [scale,lb,ub](
            VEC( epiworld_double )& params_now,
            const VEC( epiworld_double )& params_prev,
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
 * @brief 
 * 
 * @tparam TData 
 * @param m 
 */
template<typename TData>
inline void proposal_fun_unif(
    VEC( epiworld_double )& params_now,
    const VEC( epiworld_double )& params_prev,
    LFMCMC<TData>* m
) {

    for (size_t p = 0u; p < m->get_n_parameters(); ++p)
        params_now[p] = (params_prev[p] + m->runif());

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
    const VEC( epiworld_double )& stats_now,
    const VEC( epiworld_double )& stats_obs,
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
    const VEC( epiworld_double )& stats_now,
    const VEC( epiworld_double )& stats_obs,
    epiworld_double epsilon,
    LFMCMC<TData>* m
) {

    epiworld_double ans = 0.0;
    for (size_t p = 0u; p < m->get_n_parameters(); ++p)
        ans += std::pow(stats_obs[p] - stats_now[p], 2.0);

    return std::exp(-.5 * (ans/std::pow(1 + std::pow(epsilon, 2.0)/3.0, 2.0)))/std::sqrt(2.0 * 3.141593);

}



/**
 * @brief Likelihood-Free Markov Chain Monte Carlo
 * 
 * @tparam TData Type of data that is generated
 */
template<typename TData>
class LFMCMC {
private:

    // Random number sampling
    std::shared_ptr< std::mt19937 > engine =
        std::make_shared< std::mt19937 >();
    
    std::shared_ptr< std::uniform_real_distribution<> > runifd =
        std::make_shared< std::uniform_real_distribution<> >(0.0, 1.0);

    std::shared_ptr< std::normal_distribution<> > rnormd =
        std::make_shared< std::normal_distribution<> >(0.0);

    std::shared_ptr< std::gamma_distribution<> > rgammad = 
        std::make_shared< std::gamma_distribution<> >();

    // Process data
    TData * observed_data;
    
    // Information about the size of the problem
    size_t n_samples;
    size_t n_statistics;
    size_t n_parameters;

    epiworld_double epsilon;

    VEC(epiworld_double) params_now;
    VEC(epiworld_double) params_prev;
    VEC(epiworld_double) params_init;
    VEC(epiworld_double) statistics_obs;
    VEC(epiworld_double) statistics_hist;
    VEC(bool)            statistics_accepted;
    VEC(epiworld_double) posterior_lf_prob;
    VEC(epiworld_double) posterior_dist;
    VEC(epiworld_double) acceptance_prob;
    VEC(epiworld_double) drawn_prob;
    VEC(TData) * sampled_data = nullptr;

    // Functions
    LFMCMCSimFun<TData> simulation_fun;
    LFMCMCSummaryFun<TData> summary_fun;
    LFMCMCProposalFun<TData> proposal_fun = proposal_fun_normal<TData>;
    LFMCMCKernelFun<TData> kernel_fun     = kernel_fun_uniform<TData>;
    
public:

    void run(VEC( epiworld_double ) param_init, size_t n_samples_, epiworld_double epsilon_);

    LFMCMC() {};
    LFMCMC(TData & observed_data_) : observed_data(&observed_data_) {};
    ~LFMCMC() {};

    void set_observed_data(TData & observed_data_) {observed_data = &observed_data_;};
    void set_proposal_fun(LFMCMCProposalFun<TData> fun);
    void set_simulation_fun(LFMCMCSimFun<TData> fun);
    void set_summary_fun(LFMCMCSummaryFun<TData> fun);
    void set_kernel_fun(LFMCMCKernelFun<TData> fun);
    
    /**
     * @name Random number generation
     * 
     * @param eng 
     */
    ///@{
    void set_rand_engine(std::mt19937 & eng);
    std::mt19937 * get_rand_endgine();
    void seed(unsigned int s);
    void set_rand_gamma(epiworld_double alpha, epiworld_double beta);
    epiworld_double runif();
    epiworld_double rnorm();
    epiworld_double rnorm(epiworld_double mean, epiworld_double sd);
    epiworld_double rgamma();
    epiworld_double rgamma(epiworld_double alpha, epiworld_double beta);
    ///@}

    // Accessing parameters of the function
    const size_t get_n_samples() {return n_samples;};
    const size_t get_n_statistics() {return n_statistics;};
    const size_t get_n_parameters() {return n_parameters;};
    const epiworld_double get_epsilon() {return epsilon;};

    const VEC(epiworld_double) & get_params_now() {return params_now;};
    const VEC(epiworld_double) & get_params_prev() {return params_prev;};
    const VEC(epiworld_double) & get_params_init() {return params_init;};
    const VEC(epiworld_double) & get_statistics_obs() {return statistics_obs;};
    const VEC(epiworld_double) & get_statistics_hist() {return statistics_hist;};
    const VEC(bool)            & get_statistics_accepted() {return statistics_accepted;};
    const VEC(epiworld_double) & get_posterior_lf_prob() {return posterior_lf_prob;};
    const VEC(epiworld_double) & get_acceptance_prob() {return acceptance_prob;};
    const VEC(epiworld_double) & get_drawn_prob() {return drawn_prob;};
    VEC(TData) * get_sampled_data() {return sampled_data;};

};


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
    VEC( epiworld_double ) params_init_,
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
    summary_fun(statistics_obs, *observed_data, this);
    n_statistics = statistics_obs.size();

    // Reserving size
    acceptance_prob.resize(n_samples);
    drawn_prob.resize(n_samples);
    statistics_accepted.resize(n_samples, false);
    statistics_hist.resize(n_samples * n_statistics);
    posterior_lf_prob.resize(n_samples);
    posterior_dist.resize(n_samples * n_parameters);

    TData data_i = simulation_fun(params_init, this);

    VEC( epiworld_double ) stats_i;
    summary_fun(stats_i, data_i, this);
    posterior_lf_prob[0u] = kernel_fun(stats_i, statistics_obs, epsilon, this);

    // Recording statistics
    for (size_t i = 0u; i < n_statistics; ++i)
        statistics_hist[i] = stats_i[i];

    for (size_t k = 0u; k < n_statistics; ++k)
        posterior_dist[k] = stats_i[k];
   
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
        summary_fun(stats_i, data_i, this);

        // Step 4: Compute the hastings ratio using the kernel function
        double hr    = kernel_fun(stats_i, statistics_obs, epsilon, this);
        double aprob = std::min(1.0, hr / posterior_lf_prob[i - 1u]);

        // Storing data
        for (size_t k = 0u; k < n_statistics; ++k)
            statistics_hist[i * n_statistics + k] = stats_i[k];
        
        // Running Hastings ratio
        double r = runif();

        acceptance_prob[i] = aprob;
        drawn_prob[i]      = r;

        // Step 5: Update if likely
        if (r < aprob)
        {
            posterior_lf_prob[i]   = hr;
            statistics_accepted[i] = true;
            params_prev = params_now;

        } else
            posterior_lf_prob[i] = posterior_lf_prob[i - 1u];
            

        for (size_t k = 0u; k < n_parameters; ++k)
            posterior_dist[i * n_parameters + k] = params_prev[k];


    }

}


template<typename TData>
inline epiworld_double LFMCMC<TData>::runif()
{
    return runifd->operator()(*engine);
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

#undef VEC
#undef FUN

#endif