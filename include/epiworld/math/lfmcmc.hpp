#ifndef EPIWORLD_LFMCMC_HPP
#define EPIWORLD_LFMCMC_HPP

#define VEC(a) std::vector< a >
#define FUN std::function

template<typename TData>
class LFMCMC;

template<typename TData>
inline void default_proposal_fun(
    VEC( epiworld_double )& params_now,
    LFMCMC<TData>* m
) {

    const std::vector< epiworld_double > & params_prev = m->get_params_prev();
    for (size_t p = 0u; p < m->get_n_parameters(); ++p)
        params_now[p] = params_prev[p] + m->rnorm();

    return;
}

template<typename TData>
inline void default_proposal_fun_unif(
    VEC( epiworld_double )& params_now,
    LFMCMC<TData>* m
) {

    const std::vector< epiworld_double > & params_prev = m->get_params_prev();
    for (size_t p = 0u; p < m->get_n_parameters(); ++p)
        params_now[p] = m->runif();

    return;
}


/**
 * @brief Uses the uniform kernel with euclidean distance
 * 
 * @param epsilon 
 * @param m 
 * @return epiworld_double 
 */
template<typename TData>
inline epiworld_double default_kernel_fun(
    VEC( epiworld_double )& stats_now,
    epiworld_double epsilon,
    LFMCMC<TData>* m
) {

    epiworld_double ans = 0.0;
    const VEC(epiworld_double) & stats_obs = m->get_statistics_obs();
    for (size_t p = 0u; p < m->get_n_parameters(); ++p)
        ans += std::pow(stats_obs[p] - stats_now[p], 2.0);

    return std::sqrt(ans) < epsilon ? 1.0 : 0.0;

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
    FUN<TData(VEC( epiworld_double )&, LFMCMC<TData>*)> simulation_fun;
    FUN<VEC( epiworld_double )(TData&, LFMCMC<TData>*)> summary_fun;
    FUN<void(VEC( epiworld_double )&, LFMCMC<TData>*)> proposal_fun =
        default_proposal_fun<TData>;
    FUN< epiworld_double (VEC( epiworld_double )&, epiworld_double, LFMCMC<TData>*)> kernel_fun =
        default_kernel_fun<TData>;
    
public:

    void run(VEC( epiworld_double ) param_init, size_t n_samples_, epiworld_double epsilon_);

    LFMCMC() {};
    LFMCMC(TData & observed_data_) : observed_data(&observed_data_) {};
    ~LFMCMC() {};

    void set_observed_data(TData & observed_data_) {observed_data = &observed_data_;};
    void set_proposal_fun(FUN<void(VEC( epiworld_double )&, LFMCMC<TData>*)> fun);
    void set_simulation_fun(FUN<TData(VEC( epiworld_double )&, LFMCMC<TData>*)> fun);
    void set_summary_fun(FUN<VEC( epiworld_double )(TData&, LFMCMC<TData>*)> fun);
    void set_kernel_fun(FUN< epiworld_double (VEC( epiworld_double )&, epiworld_double, LFMCMC<TData>*)> fun);
    
    /**
     * @brief Random number generation
     * 
     * @param eng 
     */
    ///@[
    void set_rand_engine(std::mt19937 & eng);
    std::mt19937 * get_rand_endgine();
    void seed(unsigned int s);
    void set_rand_gamma(epiworld_double alpha, epiworld_double beta);
    epiworld_double runif();
    epiworld_double rnorm();
    epiworld_double rnorm(epiworld_double mean, epiworld_double sd);
    epiworld_double rgamma();
    epiworld_double rgamma(epiworld_double alpha, epiworld_double beta);
    ///@]

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
inline void LFMCMC<TData>::set_proposal_fun(FUN<void(VEC( epiworld_double )&, LFMCMC<TData>*)> fun)
{
    proposal_fun = fun;
}

template<typename TData>
inline void LFMCMC<TData>::set_simulation_fun(FUN<TData(VEC( epiworld_double )&, LFMCMC<TData>*)> fun)
{
    simulation_fun = fun;
}

template<typename TData>
inline void LFMCMC<TData>::set_summary_fun(FUN<VEC( epiworld_double )(TData&, LFMCMC<TData>*)> fun)
{
    summary_fun = fun;
}

template<typename TData>
inline void LFMCMC<TData>::set_kernel_fun(FUN< epiworld_double (VEC( epiworld_double )&, epiworld_double, LFMCMC<TData>*)> fun)
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

    // Computing the baseline sufficient statistics
    statistics_obs = summary_fun(*observed_data, this);
    n_statistics = statistics_obs.size();

    // Reserving size
    acceptance_prob.resize(n_samples);
    drawn_prob.resize(n_samples);
    statistics_accepted.resize(n_samples, false);
    statistics_hist.resize(n_samples * n_statistics);
    posterior_lf_prob.resize(n_samples);
    posterior_dist.resize(n_samples * n_parameters);

    params_now.resize(n_parameters);
    params_prev.resize(n_parameters);

    if (sampled_data != nullptr)
        sampled_data->resize(n_samples);

    std::swap(params_now, params_init);
    TData data_i = simulation_fun(params_now, this);

    VEC( epiworld_double ) stats_i = summary_fun(data_i, this);
    posterior_lf_prob[0u] = kernel_fun(stats_i, epsilon, this);

    for (size_t k = 0u; k < n_statistics; ++k)
        posterior_dist[k] = stats_i[k];

    std::swap(params_prev, params_now);
   
    for (size_t i = 1u; i < n_samples; ++i)
    {
        // Step 1: Generate a proposal and store it in params_now
        proposal_fun(params_now, this);

        // Step 2: Using params_now, simulate data
        TData data_i = simulation_fun(params_now, this);

        // Are we storing the data?
        if (sampled_data != nullptr)
            sampled_data->operator[](i) = data_i;

        // Step 3: Generate the summary statistics of the data
        VEC( epiworld_double ) stats_i = summary_fun(data_i, this);

        // Step 4: Compute the hastings ratio using the kernel function
        double hr = kernel_fun(stats_i, epsilon, this);
        posterior_lf_prob[i] = hr;     

        hr = std::min(1.0, hr / posterior_lf_prob[i - 1u]);

        // Storing data
        for (size_t k = 0u; k < n_statistics; ++k)
            statistics_hist[i * n_statistics + k] = stats_i[k];
        
        // Running Hastings ratio
        double r = runif();

        acceptance_prob[i] = hr;
        drawn_prob[i]      = r;

        // Step 5: Update if likely
        if (r < hr)
        {
            statistics_accepted[i] = true;
            std::swap(params_prev, params_now);

        }

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