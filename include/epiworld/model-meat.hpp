#ifndef EPIWORLD_MODEL_MEAT_HPP
#define EPIWORLD_MODEL_MEAT_HPP

#define CHECK_INIT() if (!initialized) \
        throw std::logic_error("Model not initialized.");

#define NEXT_STATUS() \
    /* Making the change effective */ \
    for (auto & p: population) \
        if (!IN(p.status, status_removed)) \
            p.status = p.status_next;

template<typename TSeq>
inline Model<TSeq>::Model(const Model<TSeq> & model) :
    db(model.db),
    viruses(model.viruses),
    prevalence_virus(model.prevalence_virus),
    tools(model.tools),
    prevalence_tool(model.prevalence_tool),
    engine(model.engine),
    runifd(model.runifd),
    parameters(model.parameters),
    ndays(model.ndays),
    pb(model.pb),
    verbose(model.verbose),
    initialized(model.initialized),
    current_date(model.current_date)
{

    // Pointing to the right place
    db.set_model(*this);

    // Removing old neighbors
    model.clone_population(
        population,
        population_ids,
        directed,
        this
        );

    // Finally, seeds are resetted automatically based on the original
    // engine
    seed(floor(runif() * UINT_MAX));

}

template<typename TSeq>
inline void Model<TSeq>::clone_population(
    std::vector< Person<TSeq> > & p,
    std::map<int,int> & p_ids,
    bool & d,
    Model<TSeq> * model
) const {

    // Copy and clean
    p     = population;
    p_ids = population_ids;
    d     = directed;

    for (auto & p: p)
        p.neighbors.clear();
    
    // Relinking individuals
    for (unsigned int i = 0u; i < size(); ++i)
    {
        // Making room
        const Person<TSeq> & person_this = population[i];
        Person<TSeq> & person_res        = p[i];

        // Person pointing to the right model and person
        if (model != nullptr)
            person_res.model        = model;
        person_res.viruses.host = &person_res;
        person_res.tools.person = &person_res;

        // Readding
        std::vector< Person<TSeq> * > neigh = person_this.neighbors;
        for (unsigned int n = 0u; n < neigh.size(); ++n)
        {
            // Point to the right neighbors
            int loc = p_ids[neigh[n]->get_id()];
            person_res.add_neighbor(&p[loc], true, true);

        }

    }
}

template<typename TSeq>
inline DataBase<TSeq> & Model<TSeq>::get_db()
{
    return db;
}

template<typename TSeq>
inline std::vector<Person<TSeq>> * Model<TSeq>::get_population()
{
    return &population;
}

template<typename TSeq>
inline void Model<TSeq>::set_rand_engine(std::mt19937 & eng)
{
    engine = std::make_shared< std::mt19937 >(eng);
}

template<typename TSeq>
inline void Model<TSeq>::set_rand_gamma(double alpha, double beta)
{
    rgammad = std::make_shared<std::gamma_distribution<>>(alpha,beta);
}

template<typename TSeq>
inline double & Model<TSeq>::operator()(std::string pname) {

    if (parameters.find(pname) == parameters.end())
        throw std::range_error("The parameter "+ pname + "is not in the model.");

    return parameters[pname];

}

template<typename TSeq>
inline size_t Model<TSeq>::size() const {
    return population.size();
}

template<typename TSeq>
inline void Model<TSeq>::init(
    unsigned int ndays,
    unsigned int seed
    ) {



    if (initialized) 
        throw std::logic_error("Model already initialized.");

    // Setting up the number of steps
    this->ndays = ndays;

    // Initializing population
    for (auto & p : population)
    {
        p.model = this;
        p.init();
    }

    engine->seed(seed);

    // if (!runifd)
    //     runifd = std::make_shared< std::uniform_real_distribution<> >(0.0, 1.0);

    initialized = true;

    // Starting first infection and tools
    reset();



}

template<typename TSeq>
inline void Model<TSeq>::dist_virus()
{


    // Starting first infection
    int n = size();
    for (unsigned int v = 0; v < viruses.size(); ++v)
    {
        // Picking how many
        std::binomial_distribution<> bd(size(), prevalence_virus[v]);
        int nsampled = bd(*engine);

        while (nsampled > 0)
        {
            int loc = static_cast<unsigned int>(floor(runif() * n));
            if (population[loc].has_virus(viruses[v].get_id()))
                continue;
            
            population[loc].add_virus(today(), viruses[v]);
            db.up_infected(&viruses[v], population[loc].get_status(), STATUS::INFECTED);
            nsampled--;

        }
    }

    NEXT_STATUS()

}

template<typename TSeq>
inline void Model<TSeq>::dist_tools()
{

    // Starting first infection
    int n = size();
    for (unsigned int t = 0; t < tools.size(); ++t)
    {
        // Picking how many
        std::binomial_distribution<> bd(size(), prevalence_tool[t]);
        int nsampled = bd(*engine);

        while (nsampled > 0)
        {
            int loc = static_cast<unsigned int>(floor(runif() * n));
            if (population[loc].has_tool(tools[t].get_id()))
                continue;
            
            population[loc].add_tool(today(), tools[t]);
            nsampled--;

        }
    }

}

template<typename TSeq>
inline void Model<TSeq>::chrono_start() {
    time_start = std::chrono::steady_clock::now();
}

template<typename TSeq>
inline void Model<TSeq>::chrono_end() {
    time_end = std::chrono::steady_clock::now();
    time_elapsed += (time_end - time_start);
    time_n++;
}

template<typename TSeq>
inline void Model<TSeq>::set_backup() {
    backup = std::unique_ptr<Model<TSeq>>(new Model<TSeq>(*this));
}

template<typename TSeq>
inline void Model<TSeq>::restore_backup() {
    if (backup != nullptr)
        *this = *backup;
}

template<typename TSeq>
inline std::mt19937 * Model<TSeq>::get_rand_endgine()
{
    return engine.get();
}

template<typename TSeq>
inline double Model<TSeq>::runif() {
    // CHECK_INIT()
    return runifd->operator()(*engine);
}

template<typename TSeq>
inline double Model<TSeq>::rnorm() {
    // CHECK_INIT()
    return (rnormd->operator()(*engine));
}

template<typename TSeq>
inline double Model<TSeq>::rnorm(double mean, double sd) {
    // CHECK_INIT()
    return (rnormd->operator()(*engine)) * sd + mean;
}

template<typename TSeq>
inline double Model<TSeq>::rgamma() {
    return rgammad->operator()(*engine);
}

template<typename TSeq>
inline double Model<TSeq>::rgamma(double alpha, double beta) {
    auto old_param = rgammad->param();
    rgammad->param(std::gamma_distribution<>::param_type(alpha, beta));
    double ans = rgammad->operator()(*engine);
    rgammad->param(old_param);
    return ans;
}

template<typename TSeq>
inline void Model<TSeq>::seed(unsigned int s) {
    this->engine->seed(s);
}

template<typename TSeq>
inline void Model<TSeq>::add_virus(Virus<TSeq> v, double preval)
{

    // Setting the id
    v.set_id(viruses.size());
    
    // Adding new virus
    viruses.push_back(v);
    prevalence_virus.push_back(preval);


}

template<typename TSeq>
inline void Model<TSeq>::add_tool(Tool<TSeq> t, double preval)
{
    t.id = tools.size();
    tools.push_back(t);
    prevalence_tool.push_back(preval);
}

template<typename TSeq>
inline void Model<TSeq>::pop_from_adjlist(
    std::string fn,
    int skip,
    bool directed,
    int min_id,
    int max_id
    ) {

    AdjList al;
    al.read_edgelist(fn, skip, directed, min_id, max_id);
    this->pop_from_adjlist(al);

}

template<typename TSeq>
inline void Model<TSeq>::pop_from_adjlist(AdjList al) {

    // Resizing the people
    population.clear();
    population_ids.clear();
    population.resize(al.vcount(), Person<TSeq>());

    const auto & tmpdat = al.get_dat();
    
    int loc;
    for (const auto & n : tmpdat)
    {
        if (population_ids.find(n.first) == population_ids.end())
            population_ids[n.first] = population_ids.size();

        loc = population_ids[n.first];

        population[loc].model = this;
        population[loc].id    = n.first;
        population[loc].index = loc;

        for (const auto & link: n.second)
        {
            if (population_ids.find(link.first) == population_ids.end())
                population_ids[link.first] = population_ids.size();

            unsigned int loc_link   = population_ids[link.first];
            population[loc_link].id    = link.first;
            population[loc_link].index = loc_link;

            population[loc].add_neighbor(
                &population[population_ids[link.first]],
                true, true
                );
        }

    }

}

template<typename TSeq>
inline bool Model<TSeq>::is_directed() const
{
    if (population.size() == 0u)
        throw std::logic_error("The population hasn't been initialized.");

    return directed;
}

template<typename TSeq>
inline int Model<TSeq>::today() const {
    return this->current_date;
}

template<typename TSeq>
inline void Model<TSeq>::next() {

    db.record();
    ++this->current_date;
    
    // Advicing the progress bar
    if (verbose)
        pb.next();

    return ;
}

template<typename TSeq>
inline void Model<TSeq>::run() 
{

    // Initializing the simulation
    chrono_start();
    EPIWORLD_RUN((*this))
    {

        // We can execute these components in whatever order the
        // user needs.
        this->update_status();
        this->mutate_variant();
        this->next();

        // In this case we are applying degree sequence rewiring
        // to change the network just a bit.
        this->rewire();

    }
    chrono_end();

}

template<typename TSeq>
inline void Model<TSeq>::update_status() {

    // Next status
    for (auto & p: population)
        p.update_status();

    NEXT_STATUS()

}

template<typename TSeq>
inline void Model<TSeq>::mutate_variant() {

    for (auto & p: population)
    {
        if (IN(p.get_status(), status_infected))
            p.mutate_variant();

    }

}

template<typename TSeq>
inline void Model<TSeq>::record_variant(Virus<TSeq> * v) {

    // Updating registry
    db.record_variant(v);
    return;
    
} 

template<typename TSeq>
inline int Model<TSeq>::get_nvariants() const {
    return db.size();
}

template<typename TSeq>
inline unsigned int Model<TSeq>::get_ndays() const {
    return ndays;
}

template<typename TSeq>
inline void Model<TSeq>::set_ndays(unsigned int ndays) {
    this->ndays = ndays;
}

template<typename TSeq>
inline bool Model<TSeq>::get_verbose() const {
    return verbose;
}

template<typename TSeq>
inline void Model<TSeq>::verbose_on() {
    verbose = true;
}

template<typename TSeq>
inline void Model<TSeq>::verbose_off() {
    verbose = false;
}

template<typename TSeq>
inline void Model<TSeq>::set_rewire_fun(
    std::function<void(std::vector<Person<TSeq>>*,Model<TSeq>*,double)> fun
    ) {
    rewire_fun = fun;
}

template<typename TSeq>
inline void Model<TSeq>::set_rewire_prop(double prop)
{

    if (prop < 0.0)
        throw std::range_error("Proportions cannot be negative.");

    if (prop > 1.0)
        throw std::range_error("Proportions cannot be above 1.0.");

    rewire_prop = prop;
}

template<typename TSeq>
inline double Model<TSeq>::get_rewire_prop() const {
    return rewire_prop;
}

template<typename TSeq>
inline void Model<TSeq>::rewire() {

    if (rewire_fun)
        rewire_fun(&population, this, rewire_prop);
}


template<typename TSeq>
inline void Model<TSeq>::write_data(
    std::string fn_variant_info,
    std::string fn_variant_hist,
    std::string fn_total_hist,
    std::string fn_transmission
    ) const
{

    db.write_data(fn_variant_info,fn_variant_hist,fn_total_hist,fn_transmission);

}

template<typename TSeq>
inline void Model<TSeq>::write_edgelist(
    std::string fn
    ) const
{



    std::ofstream efile(fn, std::ios_base::out);
    efile << "source target\n";
    for (const auto & p : population)
    {
        for (auto & n : p.neighbors)
            efile << p.id << " " << n->id << "\n";
    }



}

template<typename TSeq>
inline std::map<std::string,double> & Model<TSeq>::params()
{
    return parameters;
}

template<typename TSeq>
inline void Model<TSeq>::reset() {
    
    // Restablishing people
    pb = Progress(ndays, 80);

    if (backup != nullptr)
    {
        backup->clone_population(
            population,
            population_ids,
            directed,
            this
        );
    }

    for (auto & p : population)
    {
        p.reset();
        
        if (update_susceptible)
            p.set_update_susceptible(update_susceptible);
        else if (!p.update_susceptible)
            throw std::logic_error("No update_susceptible function set.");
        if (update_infected)
            p.set_update_infected(update_infected);
        else if (!p.update_infected)
            throw std::logic_error("No update_infected function set.");
        if (update_removed)
            p.set_update_removed(update_removed);
        
    }
    
    current_date = 0;

    db.set_model(*this);

    // Recording variants
    for (Virus<TSeq> & v : viruses)
        record_variant(&v);

    // Re distributing tools and virus
    dist_virus();
    dist_tools();

}

template<typename TSeq>
inline void Model<TSeq>::print() const
{

    // Horizontal line
    std::string line = "";
    for (unsigned int i = 0u; i < 80u; ++i)
        line += "_";

    printf_epiworld("\n%s\n%s\n\n",line.c_str(), "SIMULATION STUDY");
    printf_epiworld("Population size    : %i\n", static_cast<int>(size()));
    printf_epiworld("Days (duration)    : %i (of %i)\n", today(), ndays);
    printf_epiworld("Number of variants : %i\n", static_cast<int>(db.get_nvariants()));
    if (time_n > 0u)
    {
        std::string abbr;
        double elapsed;
        double total;
        get_elapsed("auto", &elapsed, &total, nullptr, &abbr, false);
        printf_epiworld("Last run elapsed t : %.2f%s\n", elapsed, abbr.c_str());
        if (time_n > 1u)
        {
            printf_epiworld("Total elapsed t    : %.2f%s (%i runs)\n", total, abbr.c_str(), time_n);
        }

    } else {
        printf_epiworld("Last run elapsed t : -\n");
    }
    
    if (rewire_fun)
    {
        printf_epiworld("Rewiring           : on (%.2f)\n\n", rewire_prop);
    } else {
        printf_epiworld("Rewiring           : off\n\n");
    }

    printf_epiworld("Virus(es):\n");
    int i = 0;
    for (auto & v : viruses)
    {    printf_epiworld(
            " - %s (baseline prevalence: %.2f)\n",
            v.get_name().c_str(),
            prevalence_virus[i++]
            );
    }

    printf_epiworld("Tool(s):\n");
    i = 0;
    for (auto & t : tools)
    {    printf_epiworld(
            " - %s (baseline prevalence: %.2f)\n",
            t.get_name().c_str(),
            prevalence_tool[i++]
            );
    }

    // Information about the parameters included
    printf_epiworld("\nModel parameters:\n");
    unsigned int nchar = 0u;
    for (auto & p : parameters)
        if (p.first.length() > nchar)
            nchar = p.first.length();

    std::string fmt = " - %-" + std::to_string(nchar + 1) + "s: ";
    for (auto & p : parameters)
    {
        std::string fmt_tmp = fmt;
        if (std::fabs(p.second) < 0.0001)
            fmt_tmp += "%.1e\n";
        else
            fmt_tmp += "%.4f\n";

        printf_epiworld(
            fmt_tmp.c_str(),
            p.first.c_str(),
            p.second
        );
        
    }


    nchar = 0u;
    for (auto & p : status_susceptible_labels)
        if (p.length() > nchar)
            nchar = p.length();
    
    for (auto & p : status_infected_labels)
        if (p.length() > nchar)
            nchar = p.length();

    for (auto & p : status_removed_labels)
        if (p.length() > nchar)
            nchar = p.length();

    if (initialized) 
        fmt = " - Total %-" + std::to_string(nchar + 1 + 4) + "s: %i\n";
    else
        fmt = " - Total %-" + std::to_string(nchar + 1 + 4) + "s: %s\n";
        
    printf_epiworld("\nDistribution of the population at time %i:\n", today());
    for (unsigned int s = 0u; s < status_susceptible.size(); ++s)
    {
        if (initialized)
        {
            
            printf_epiworld(
                fmt.c_str(),
                (status_susceptible_labels[s] + " (S)").c_str(),
                db.today_total[ status_susceptible[s] ]
                );

        } else {
            printf_epiworld(
                fmt.c_str(),
                (status_susceptible_labels[s] + " (S)").c_str(),
                " - "
                );
        }
    }

    // printf_epiworld("\nStatistics (infected):\n");
    for (unsigned int s = 0u; s < status_infected.size(); ++s)
    {
        if (initialized)
        {
            
            printf_epiworld(
                fmt.c_str(),
                (status_infected_labels[s] + " (I)").c_str(),
                db.today_total[ status_infected[s] ]
                );

        } else {
            printf_epiworld(
                fmt.c_str(),
                (status_infected_labels[s] + " (I)").c_str(),
                " - "
                );
        }
    }

    // printf_epiworld("\nStatistics (removed):\n");
    for (unsigned int s = 0u; s < status_removed.size(); ++s)
    {
        if (initialized)
        {
            
            printf_epiworld(
                fmt.c_str(),
                (status_removed_labels[s] + " (R)").c_str(),
                db.today_total[ status_removed[s] ]
                );

        } else {
            printf_epiworld(
                fmt.c_str(),
                (status_removed_labels[s] + " (R)").c_str(),
                " - "
                );
        }
    }
    
    printf_epiworld(
        "\n(S): Susceptible, (I): Infected, (R): Recovered\n%s\n\n",
        line.c_str()
        );

    return;

}

template<typename TSeq>
inline Model<TSeq> && Model<TSeq>::clone() const {

    // Step 1: Regen the individuals and make sure that:
    //  - Neighbors point to the right place
    //  - DB is pointing to the right place
    Model<TSeq> res(*this);

    // Pointing to the right place
    res.get_db().set_model(res);

    // Removing old neighbors
    for (auto & p: res.population)
        p.neighbors.clear();
    
    // Rechecking individuals
    for (unsigned int p = 0u; p < size(); ++p)
    {
        // Making room
        const Person<TSeq> & person_this = population[p];
        Person<TSeq> & person_res  = res.population[p];

        // Person pointing to the right model and person
        person_res.model        = &res;
        person_res.viruses.host = &person_res;
        person_res.tools.person = &person_res;

        // Readding
        std::vector< Person<TSeq> * > neigh = person_this.neighbors;
        for (unsigned int n = 0u; n < neigh.size(); ++n)
        {
            // Point to the right neighbors
            int loc = res.population_ids[neigh[n]->get_id()];
            person_res.add_neighbor(&res.population[loc], true, true);

        }

    }

    return res;

}

#define EPIWORLD_CHECK_STATUS(a, b) \
    for (auto & i : b) \
        if (a == i) \
            throw std::logic_error("The status " + std::to_string(i) + " already exists."); 
#define EPIWORLD_CHECK_ALL_STATUSES(a) \
    EPIWORLD_CHECK_STATUS(a, status_susceptible) \
    EPIWORLD_CHECK_STATUS(a, status_infected) \
    EPIWORLD_CHECK_STATUS(a, status_removed)

template<typename TSeq>
inline void Model<TSeq>::add_status_susceptible(unsigned int s, std::string lab)
{

    EPIWORLD_CHECK_ALL_STATUSES(s)
    status_susceptible.push_back(s);
    status_susceptible_labels.push_back(lab);
    nstatus++;

}

template<typename TSeq>
inline void Model<TSeq>::add_status_infected(unsigned int s, std::string lab)
{

    EPIWORLD_CHECK_ALL_STATUSES(s)
    status_infected.push_back(s);
    status_infected_labels.push_back(lab);

}

template<typename TSeq>
inline void Model<TSeq>::add_status_removed(unsigned int s, std::string lab)
{

    EPIWORLD_CHECK_ALL_STATUSES(s)
    status_removed.push_back(s);
    status_removed_labels.push_back(lab);

}

template<typename TSeq>
inline void Model<TSeq>::add_status_susceptible(std::string lab)
{
    status_susceptible.push_back(nstatus++);
    status_susceptible_labels.push_back(lab);
}

template<typename TSeq>
inline void Model<TSeq>::add_status_infected(std::string lab)
{
    status_infected.push_back(nstatus++);
    status_infected_labels.push_back(lab);
}

template<typename TSeq>
inline void Model<TSeq>::add_status_removed(std::string lab)
{
    status_removed.push_back(nstatus++);
    status_removed_labels.push_back(lab);
}

#define EPIWORLD_COLLECT_STATUSES(out,id,lab) \
    std::vector< std::pair<unsigned int, std::string> > out; \
    for (unsigned int i = 0; i < id.size(); ++i) \
        out.push_back( \
            std::pair<int,std::string>( \
                id[i], lab[i] \
            ) \
        );

template<typename TSeq>
inline const std::vector< unsigned int > &
Model<TSeq>::get_status_susceptible() const
{
    return status_susceptible;
}

template<typename TSeq>
inline const std::vector< unsigned int > &
Model<TSeq>::get_status_infected() const
{
    return status_infected;
}

template<typename TSeq>
inline const std::vector< unsigned int > &
Model<TSeq>::get_status_removed() const
{
    return status_removed;
}

template<typename TSeq>
inline const std::vector< std::string > &
Model<TSeq>::get_status_susceptible_labels() const
{
    return status_susceptible_labels;
}

template<typename TSeq>
inline const std::vector< std::string > &
Model<TSeq>::get_status_infected_labels() const
{
    return status_infected_labels;
}

template<typename TSeq>
inline const std::vector< std::string > &
Model<TSeq>::get_status_removed_labels() const
{
    return status_removed_labels;
}

#define CASE_PAR(a,b) case a: b = &(parameters[pname]);break;
#define CASES_PAR(a) \
    switch (a) \
    { \
    CASE_PAR(0u, p0) CASE_PAR(1u, p1) CASE_PAR(2u, p2) CASE_PAR(3u, p3) \
    CASE_PAR(4u, p4) CASE_PAR(5u, p5) CASE_PAR(6u, p6) CASE_PAR(7u, p7) \
    CASE_PAR(8u, p8) CASE_PAR(9u, p9) \
    CASE_PAR(10u, p10) CASE_PAR(11u, p11) CASE_PAR(12u, p12) CASE_PAR(13u, p13) \
    CASE_PAR(14u, p14) CASE_PAR(15u, p15) CASE_PAR(16u, p16) CASE_PAR(17u, p17) \
    CASE_PAR(18u, p18) CASE_PAR(19u, p19) \
    CASE_PAR(20u, p20) CASE_PAR(21u, p21) CASE_PAR(22u, p22) CASE_PAR(23u, p23) \
    CASE_PAR(24u, p24) CASE_PAR(25u, p25) CASE_PAR(26u, p26) CASE_PAR(27u, p27) \
    CASE_PAR(28u, p28) CASE_PAR(29u, p29) \
    CASE_PAR(30u, p30) CASE_PAR(31u, p31) CASE_PAR(32u, p22) CASE_PAR(33u, p23) \
    CASE_PAR(34u, p34) CASE_PAR(35u, p35) CASE_PAR(36u, p26) CASE_PAR(37u, p27) \
    CASE_PAR(38u, p38) CASE_PAR(39u, p39) \
    default: \
        break; \
    }

template<typename TSeq>
inline double Model<TSeq>::add_param(
    double initial_value,
    std::string pname
    ) {

    if (parameters.find(pname) == parameters.end())
        parameters[pname] = initial_value;

    CASES_PAR(npar_used++)
    
    return initial_value;

}

template<typename TSeq>
inline double Model<TSeq>::set_param(
    std::string pname
    ) {

    if (parameters.find(pname) == parameters.end())
        throw std::logic_error("The parameter " + pname + " does not exists.");

    CASES_PAR(npar_used++)

    return parameters[pname];
    
}

template<typename TSeq>
inline double Model<TSeq>::get_param(std::string pname)
{
    if (parameters.find(pname) == parameters.end())
        throw std::logic_error("The parameter " + pname + " does not exists.");

    return parameters[pname];
}

template<typename TSeq>
inline double Model<TSeq>::par(std::string pname)
{
    return parameters[pname];
}

#define DURCAST(tunit,txtunit) {\
        elapsed       = std::chrono::duration_cast<std::chrono:: tunit>(\
            time_end - time_start).count(); \
        elapsed_total = std::chrono::duration_cast<std::chrono:: tunit>(time_elapsed).count(); \
        abbr_unit     = txtunit;}

template<typename TSeq>
inline void Model<TSeq>::get_elapsed(
    std::string unit,
    double * last_elapsed,
    double * total_elapsed,
    unsigned int * n_replicates,
    std::string * unit_abbr,
    bool print
) const {

    // Preparing the result
    double elapsed, elapsed_total;
    std::string abbr_unit;

    // Figuring out the length
    if (unit == "auto")
    {

        size_t tlength = std::to_string(
            static_cast<int>(floor(time_elapsed.count()))
            ).length();
        
        if (tlength <= 1)
            unit = "nanoseconds";
        else if (tlength <= 3)
            unit = "microseconds";
        else if (tlength <= 6)
            unit = "milliseconds";
        else if (tlength <= 8)
            unit = "seconds";
        else if (tlength <= 9)
            unit = "minutes";
        else 
            unit = "hours";

    }

    if (unit == "nanoseconds")       DURCAST(nanoseconds,"ns")
    else if (unit == "microseconds") DURCAST(microseconds,"\xC2\xB5s")
    else if (unit == "milliseconds") DURCAST(milliseconds,"ms")
    else if (unit == "seconds")      DURCAST(seconds,"s")
    else if (unit == "minutes")      DURCAST(minutes,"m")
    else if (unit == "hours")        DURCAST(hours,"h")
    else
        throw std::range_error("The time unit " + unit + " is not supported.");


    if (last_elapsed != nullptr)
        *last_elapsed = elapsed;
    if (total_elapsed != nullptr)
        *total_elapsed = elapsed_total;
    if (n_replicates != nullptr)
        *n_replicates = time_n;
    if (unit_abbr != nullptr)
        *unit_abbr = abbr_unit;

    if (!print)
        return;

    if (time_n > 1u)
    {
        printf_epiworld("last run elapsed time : %.2f%s\n",
            elapsed, abbr_unit.c_str());
        printf_epiworld("total elapsed time    : %.2f%s\n",
            elapsed_total, abbr_unit.c_str());
        printf_epiworld("total runs            : %i\n",
            static_cast<int>(time_n));
        printf_epiworld("mean run elapsed time : %.2f%s\n",
            elapsed_total/static_cast<double>(time_n), abbr_unit.c_str());

    } else {
        printf_epiworld("last run elapsed time : %.2f%s.\n", elapsed, abbr_unit.c_str());
    }
}

template<typename TSeq>
inline void Model<TSeq>::set_update_susceptible(UpdateFun<TSeq> fun) {
    
    update_susceptible = fun;

}

template<typename TSeq>
inline void Model<TSeq>::set_update_infected(UpdateFun<TSeq> fun) {
    
    update_infected = fun;

}

template<typename TSeq>
inline void Model<TSeq>::set_update_removed(UpdateFun<TSeq> fun) {
    
    update_removed = fun;

}

#undef DURCAST

#undef CASES_PAR
#undef CASE_PAR

#undef EPIWORLD_CHECK_STATE
#undef EPIWORLD_CHECK_ALL_STATES
#undef EPIWORLD_COLLECT_STATUSES
#undef NEXT_STATUS

#undef CHECK_INIT
#endif