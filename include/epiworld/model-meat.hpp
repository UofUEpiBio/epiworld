#ifndef EPIWORLD_MODEL_MEAT_HPP
#define EPIWORLD_MODEL_MEAT_HPP

#define CHECK_INIT() if (!initialized) \
        throw std::logic_error("Model not initialized.");

/**
 * @brief Function factory for saving model runs
 * 
 * @details This function is the default behavior of the `run_multiple`
 * member of `Model<TSeq>`. By default only the total history (
 * case counts by unit of time.)
 * 
 * @tparam TSeq 
 * @param fmt 
 * @param total_hist 
 * @param variant_info 
 * @param variant_hist 
 * @param tool_info 
 * @param tool_hist 
 * @param transmission 
 * @param transition 
 * @return std::function<void(size_t,Model<TSeq>*)> 
 */
template<typename TSeq>
inline std::function<void(size_t,Model<TSeq>*)> make_save_run(
    std::string fmt,
    bool total_hist,
    bool variant_info,
    bool variant_hist,
    bool tool_info,
    bool tool_hist,
    bool transmission,
    bool transition,
    bool reproductive
    )
{

    // Counting number of %
    int n_fmt = 0;
    for (auto & f : fmt)
        if (f == '%')
            n_fmt++;

    if (n_fmt != 1)
        throw std::logic_error("The -fmt- argument must have only one \"%\" symbol.");

    // Listting things to save
    std::vector< bool > what_to_save = {
        variant_info,
        variant_hist,
        tool_info,
        tool_hist,
        total_hist,
        transmission,
        transition,
        reproductive
    };

    std::function<void(size_t,Model<TSeq>*)> saver = [fmt,what_to_save](
        size_t niter, Model<TSeq> * m
    ) -> void {

        std::string variant_info = "";
        std::string variant_hist = "";
        std::string tool_info = "";
        std::string tool_hist = "";
        std::string total_hist = "";
        std::string transmission = "";
        std::string transition = "";
        std::string reproductive = "";

        char buff[128];
        if (what_to_save[0u])
        {
            variant_info = fmt + std::string("_variant_info.csv");
            snprintf(buff, sizeof(buff), variant_info.c_str(), niter);
            variant_info = buff;
        } 
        if (what_to_save[1u])
        {
            variant_hist = fmt + std::string("_variant_hist.csv");
            snprintf(buff, sizeof(buff), variant_hist.c_str(), niter);
            variant_hist = buff;
        } 
        if (what_to_save[2u])
        {
            tool_info = fmt + std::string("_tool_info.csv");
            snprintf(buff, sizeof(buff), tool_info.c_str(), niter);
            tool_info = buff;
        } 
        if (what_to_save[3u])
        {
            tool_hist = fmt + std::string("_tool_hist.csv");
            snprintf(buff, sizeof(buff), tool_hist.c_str(), niter);
            tool_hist = buff;
        } 
        if (what_to_save[4u])
        {
            total_hist = fmt + std::string("_total_hist.csv");
            snprintf(buff, sizeof(buff), total_hist.c_str(), niter);
            total_hist = buff;
        } 
        if (what_to_save[5u])
        {
            transmission = fmt + std::string("_transmission.csv");
            snprintf(buff, sizeof(buff), transmission.c_str(), niter);
            transmission = buff;
        } 
        if (what_to_save[6u])
        {
            transition = fmt + std::string("_transition.csv");
            snprintf(buff, sizeof(buff), transition.c_str(), niter);
            transition = buff;
        } 
        if (what_to_save[7u])
        {

            reproductive = fmt + std::string("_reproductive.csv");
            snprintf(buff, sizeof(buff), reproductive.c_str(), niter);
            reproductive = buff;

        }
    
        m->write_data(
            variant_info,
            variant_hist,
            tool_info,
            tool_hist,
            total_hist,
            transmission,
            transition,
            reproductive
        );

    };

    return saver;
}

template<typename TSeq>
inline void Model<TSeq>::actions_add(
    Agent<TSeq> * agent_,
    VirusPtr<TSeq> virus_,
    ToolPtr<TSeq> tool_,
    Entity<TSeq> * entity_,
    epiworld_fast_uint new_status_,
    epiworld_fast_int queue_,
    ActionFun<TSeq> call_,
    int idx_agent_,
    int idx_object_
) {
    
    ++nactions;

    #ifdef EPI_DEBUG
    if (nactions == 0)
        throw std::logic_error("Actions cannot be zero!!");
    #endif

    if (nactions > actions.size())
    {

        actions.push_back(
            Action<TSeq>(
                agent_, virus_, tool_, entity_, new_status_, queue_, call_,
                idx_agent_, idx_object_
            ));

    }
    else 
    {

        Action<TSeq> & A = actions.at(nactions - 1u);
        A.agent = agent_;
        A.virus = virus_;
        A.tool = tool_;
        A.entity = entity_;
        A.new_status = new_status_;
        A.queue = queue_;
        A.call = call_;
        A.idx_agent = idx_agent_;
        A.idx_object = idx_object_;

    }

    return;

}

template<typename TSeq>
inline void Model<TSeq>::actions_run()
{
    // Making the call
    while (nactions != 0u)
    {

        Action<TSeq>   a = actions[--nactions];
        Agent<TSeq> * p  = a.agent;

        // Applying function
        if (a.call)
        {
            a.call(a, this);
        }

        // Updating status
        if (static_cast<epiworld_fast_int>(p->status) != a.new_status)
        {

            if (a.new_status >= static_cast<epiworld_fast_int>(nstatus))
                throw std::range_error(
                    "The proposed status " + std::to_string(a.new_status) + " is out of range. " +
                    "The model currently has " + std::to_string(nstatus - 1) + " statuses.");

            // Figuring out if we need to undo a change
            // If the agent has made a change in the status recently, then we
            // need to undo the accounting, e.g., if A->B was made, we need to
            // undo it and set B->A so that the daily accounting is right.
            if (p->status_last_changed == today())
            {

                // Updating accounting
                db.update_state(p->status_prev, p->status, true); // Undoing
                db.update_state(p->status_prev, a.new_status);

                for (size_t v = 0u; v < p->n_viruses; ++v)
                {
                    db.update_virus(p->viruses[v]->id, p->status, p->status_prev); // Undoing
                    db.update_virus(p->viruses[v]->id, p->status_prev, a.new_status);
                }

                for (size_t t = 0u; t < p->n_tools; ++t)
                {
                    db.update_tool(p->tools[t]->id, p->status, p->status_prev); // Undoing
                    db.update_tool(p->tools[t]->id, p->status_prev, a.new_status);
                }

                // Changing to the new status, we won't update the
                // previous status in case we need to undo the change
                p->status = a.new_status;

            } else {

                // Updating accounting
                db.update_state(p->status, a.new_status);

                for (size_t v = 0u; v < p->n_viruses; ++v)
                    db.update_virus(p->viruses[v]->id, p->status, a.new_status);

                for (size_t t = 0u; t < p->n_tools; ++t)
                    db.update_tool(p->tools[t]->id, p->status, a.new_status);


                // Saving the last status and setting the new one
                p->status_prev = p->status;
                p->status      = a.new_status;

                // It used to be a day before, but we still
                p->status_last_changed = today();

            }
            
        }

        #ifdef EPI_DEBUG
        if (p->status >= static_cast<epiworld_fast_int>(nstatus))
                throw std::range_error(
                    "The new status " + std::to_string(p->status) + " is out of range. " +
                    "The model currently has " + std::to_string(nstatus - 1) + " statuses.");
        #endif

        // Updating queue
        if (a.queue == QueueValues::Everyone)
            queue += p;
        else if (a.queue == -QueueValues::Everyone)
            queue -= p;
        else if (a.queue == QueueValues::OnlySelf)
            queue[p->get_id()]++;
        else if (a.queue == -QueueValues::OnlySelf)
            queue[p->get_id()]--;
        else if (a.queue != QueueValues::NoOne)
            throw std::logic_error(
                "The proposed queue change is not valid. Queue values can be {-2, -1, 0, 1, 2}."
                );

    }

    return;
    
}

/**
 * @name Default function for combining susceptibility_reduction levels
 * 
 * @tparam TSeq 
 * @param pt 
 * @return epiworld_double 
 */
///@{
template<typename TSeq>
inline epiworld_double susceptibility_reduction_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq> * m
)
{
    epiworld_double total = 1.0;
    for (auto & tool : p->get_tools())
        total *= (1.0 - tool->get_susceptibility_reduction(v));

    return 1.0 - total;
    
}

template<typename TSeq>
inline epiworld_double transmission_reduction_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq>* m
)
{
    epiworld_double total = 1.0;
    for (auto & tool : p->get_tools())
        total *= (1.0 - tool->get_transmission_reduction(v));

    return (1.0 - total);
    
}

template<typename TSeq>
inline epiworld_double recovery_enhancer_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq>* m
)
{
    epiworld_double total = 1.0;
    for (auto & tool : p->get_tools())
        total *= (1.0 - tool->get_recovery_enhancer(v));

    return 1.0 - total;
    
}

template<typename TSeq>
inline epiworld_double death_reduction_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq>* m
)
{
    epiworld_double total = 1.0;
    for (auto & tool : p->get_tools())
    {
        total *= (1.0 - tool->get_death_reduction(v));
    } 

    return 1.0 - total;
    
}
///@}

template<typename TSeq>
inline Model<TSeq>::Model(const Model<TSeq> & model) :
    db(model.db),
    viruses(model.viruses),
    prevalence_virus(model.prevalence_virus),
    prevalence_virus_as_proportion(model.prevalence_virus_as_proportion),
    viruses_dist_funs(model.viruses_dist_funs),
    tools(model.tools),
    prevalence_tool(model.prevalence_tool),
    prevalence_tool_as_proportion(model.prevalence_tool_as_proportion),
    tools_dist_funs(model.tools_dist_funs),
    entities(model.entities),
    prevalence_entity(model.prevalence_entity),
    prevalence_entity_as_proportion(model.prevalence_entity_as_proportion),
    entities_dist_funs(model.entities_dist_funs),
    engine(model.engine),
    runifd(model.runifd),
    parameters(model.parameters),
    ndays(model.ndays),
    pb(model.pb),
    status_fun(model.status_fun),
    status_labels(model.status_labels),
    nstatus(model.nstatus),
    verbose(model.verbose),
    initialized(model.initialized),
    current_date(model.current_date),
    global_action_functions(model.global_action_functions),
    global_action_dates(model.global_action_dates),
    queue(model.queue),
    use_queuing(model.use_queuing)
{

    // Pointing to the right place
    db.set_model(*this);

    // Removing old neighbors
    model.clone_population(
        population,
        directed,
        this
        );

    // Figure out the queuing
    if (use_queuing)
        queue.set_model(this);

    // Finally, seeds are resetted automatically based on the original
    // engine
    seed(floor(runif() * UINT_MAX));

}

template<typename TSeq>
inline Model<TSeq>::Model(Model<TSeq> && model) :
    db(std::move(model.db)),
    viruses(std::move(model.viruses)),
    prevalence_virus(std::move(model.prevalence_virus)),
    prevalence_virus_as_proportion(std::move(model.prevalence_virus_as_proportion)),
    viruses_dist_funs(std::move(model.viruses_dist_funs)),
    tools(std::move(model.tools)),
    prevalence_tool(std::move(model.prevalence_tool)),
    prevalence_tool_as_proportion(std::move(model.prevalence_tool_as_proportion)),
    tools_dist_funs(std::move(model.tools_dist_funs)),
    entities(std::move(model.entities)),
    prevalence_entity(std::move(model.prevalence_entity)),
    prevalence_entity_as_proportion(std::move(model.prevalence_entity_as_proportion)),
    entities_dist_funs(std::move(model.entities_dist_funs)),
    engine(std::move(model.engine)),
    runifd(std::move(model.runifd)),
    parameters(std::move(model.parameters)),
    ndays(std::move(model.ndays)),
    pb(std::move(model.pb)),
    verbose(std::move(model.verbose)),
    initialized(std::move(model.initialized)),
    current_date(std::move(model.current_date)),
    population(std::move(model.population)),
    directed(std::move(model.directed)),
    global_action_functions(std::move(model.global_action_functions)),
    global_action_dates(std::move(model.global_action_dates)),
    status_fun(std::move(model.status_fun)),
    status_labels(std::move(model.status_labels)),
    nstatus(model.nstatus),
    queue(std::move(model.queue)),
    use_queuing(model.use_queuing)
{

}

template<typename TSeq>
inline void Model<TSeq>::clone_population(
    std::vector< Agent<TSeq> > & p,
    bool & d,
    Model<TSeq> * model
) const {

    // Copy and clean
    p     = population;
    d     = directed;

    for (auto & p: p)
        p.neighbors.clear();
    
    // Relinking individuals
    for (unsigned int i = 0u; i < size(); ++i)
    {
        // Making room
        const Agent<TSeq> & agent_this = population[i];
        Agent<TSeq> & agent_res        = p[i];

        // Readding
        std::vector< Agent<TSeq> * > neigh = agent_this.neighbors;
        for (unsigned int n = 0u; n < neigh.size(); ++n)
        {
            // Point to the right neighbors
            int loc = p[neigh[n]->get_id()].get_id();
            agent_res.add_neighbor(&p[loc], true, true);

        }

    }
}

template<typename TSeq>
inline void Model<TSeq>::clone_population(const Model<TSeq> & m)
{
    m.clone_population(
        population,
        directed,
        this
    );
}

template<typename TSeq>
inline DataBase<TSeq> & Model<TSeq>::get_db()
{
    return db;
}

template<typename TSeq>
inline std::vector<Agent<TSeq>> * Model<TSeq>::get_agents()
{
    return &population;
}

template<typename TSeq>
inline void Model<TSeq>::agents_smallworld(
    unsigned int n,
    unsigned int k,
    bool d,
    epiworld_double p
)
{
    agents_from_adjlist(
        rgraph_smallworld(n, k, p, d, *this)
    );
}

template<typename TSeq>
inline void Model<TSeq>::set_rand_engine(std::mt19937 & eng)
{
    engine = std::make_shared< std::mt19937 >(eng);
}

template<typename TSeq>
inline void Model<TSeq>::set_rand_gamma(epiworld_double alpha, epiworld_double beta)
{
    rgammad = std::make_shared<std::gamma_distribution<>>(alpha,beta);
}

template<typename TSeq>
inline epiworld_double & Model<TSeq>::operator()(std::string pname) {

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

    if (nstatus == 0u)
        throw std::logic_error(
            std::string("No statuses registered in this model. ") +
            std::string("At least one status should be included. See the function -Model::add_status()-")
            );

    // Setting up the number of steps
    this->ndays = ndays;

    // Initializing population
    for (auto & p : population)
        p.model = this;

    engine->seed(seed);
    array_double_tmp.resize(size()/2, 0.0);
    array_virus_tmp.resize(size()/2);

    initialized = true;

    queue.set_model(this);

    // Checking whether the proposed status in/out/removed
    // are valid
    epiworld_fast_int _init, _end, _removed;
    int nstatus_int = static_cast<int>(nstatus);
    for (auto & v : viruses)
    {
        v->get_status(&_init, &_end, &_removed);
        
        // Negative unspecified status
        if (((_init != -99) && (_init < 0)) || (_init >= nstatus_int))
            throw std::range_error("Statuses must be between 0 and " +
                std::to_string(nstatus - 1));

        // Negative unspecified status
        if (((_end != -99) && (_end < 0)) || (_end >= nstatus_int))
            throw std::range_error("Statuses must be between 0 and " +
                std::to_string(nstatus - 1));

        if (((_removed != -99) && (_removed < 0)) || (_removed >= nstatus_int))
            throw std::range_error("Statuses must be between 0 and " +
                std::to_string(nstatus - 1));

    }

    for (auto & t : tools)
    {
        t->get_status(&_init, &_end);
        
        // Negative unspecified status
        if (((_init != -99) && (_init < 0)) || (_init >= nstatus_int))
            throw std::range_error("Statuses must be between 0 and " +
                std::to_string(nstatus - 1));

        // Negative unspecified status
        if (((_end != -99) && (_end < 0)) || (_end >= nstatus_int))
            throw std::range_error("Statuses must be between 0 and " +
                std::to_string(nstatus - 1));

    }

    // Starting first infection and tools
    reset();



}

template<typename TSeq>
inline void Model<TSeq>::dist_virus()
{

    // Starting first infection
    int n = size();
    std::vector< size_t > idx(n);

    int n_left = n;
    std::iota(idx.begin(), idx.end(), 0);

    for (unsigned int v = 0; v < viruses.size(); ++v)
    {

        if (viruses_dist_funs[v])
        {

            viruses_dist_funs[v](*viruses[v], this);

        } else {

            // Picking how many
            int nsampled;
            if (prevalence_virus_as_proportion[v])
            {
                nsampled = static_cast<int>(std::floor(prevalence_virus[v] * size()));
            }
            else
            {
                nsampled = static_cast<int>(prevalence_virus[v]);
            }

            if (nsampled > static_cast<int>(size()))
                throw std::range_error("There are only " + std::to_string(size()) + 
                " individuals in the population. Cannot add the virus to " + std::to_string(nsampled));


            VirusPtr<TSeq> virus = viruses[v];
            
            while (nsampled > 0)
            {

                int loc = static_cast<unsigned int>(floor(runif() * (n_left--)));

                Agent<TSeq> & agent = population[idx[loc]];
                
                // Adding action
                agent.add_virus(virus, virus->status_init, virus->queue_init);

                // Adjusting sample
                nsampled--;
                std::swap(idx[loc], idx[n_left]);

            }

        }

        // Apply the actions
        actions_run();
    }

}

template<typename TSeq>
inline void Model<TSeq>::dist_tools()
{

    // Starting first infection
    int n = size();
    std::vector< size_t > idx(n);
    for (unsigned int t = 0; t < tools.size(); ++t)
    {

        if (tools_dist_funs[t])
        {

            tools_dist_funs[t](*tools[t], this);

        } else {

            // Picking how many
            int nsampled;
            if (prevalence_tool_as_proportion[t])
            {
                nsampled = static_cast<int>(std::floor(prevalence_tool[t] * size()));
            }
            else
            {
                nsampled = static_cast<int>(prevalence_tool[t]);
            }

            if (nsampled > static_cast<int>(size()))
                throw std::range_error("There are only " + std::to_string(size()) + 
                " individuals in the population. Cannot add the tool to " + std::to_string(nsampled));
            
            ToolPtr<TSeq> tool = tools[t];

            int n_left = n;
            std::iota(idx.begin(), idx.end(), 0);
            while (nsampled > 0)
            {
                int loc = static_cast<unsigned int>(floor(runif() * n_left--));
                
                population[idx[loc]].add_tool(tool, tool->status_init, tool->queue_init);
                
                nsampled--;

                std::swap(idx[loc], idx[n_left]);

            }

        }

        // Apply the actions
        actions_run();

    }

}

template<typename TSeq>
inline void Model<TSeq>::dist_entities()
{

    // Starting first infection
    int n = size();
    std::vector< size_t > idx(n);
    for (unsigned int e = 0; e < entities.size(); ++e)
    {

        if (entities_dist_funs[e])
        {

            entities_dist_funs[e](entities[e], this);

        } else {

            // Picking how many
            int nsampled;
            if (prevalence_entity_as_proportion[e])
            {
                nsampled = static_cast<int>(std::floor(prevalence_entity[e] * size()));
            }
            else
            {
                nsampled = static_cast<int>(prevalence_entity[e]);
            }

            if (nsampled > static_cast<int>(size()))
                throw std::range_error("There are only " + std::to_string(size()) + 
                " individuals in the population. Cannot add the entity to " + std::to_string(nsampled));
            
            Entity<TSeq> & entity = entities[e];

            int n_left = n;
            std::iota(idx.begin(), idx.end(), 0);
            while (nsampled > 0)
            {
                int loc = static_cast<unsigned int>(floor(runif() * n_left--));
                
                population[idx[loc]].add_entity(entity, entity.status_init, entity.queue_init);
                
                nsampled--;

                std::swap(idx[loc], idx[n_left]);

            }

        }

        // Apply the actions
        actions_run();

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
    n_replicates++;
}

template<typename TSeq>
inline void Model<TSeq>::set_backup()
{

    backup = std::unique_ptr<Model<TSeq>>(new Model<TSeq>(*this));

}

template<typename TSeq>
inline void Model<TSeq>::restore_backup()
{

    if (backup != nullptr)
    {

        clone_population(*backup);

        db = backup->db;
        db.set_model(*this);

    }

}

template<typename TSeq>
inline std::mt19937 * Model<TSeq>::get_rand_endgine()
{
    return engine.get();
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::runif() {
    // CHECK_INIT()
    return runifd->operator()(*engine);
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rnorm() {
    // CHECK_INIT()
    return (rnormd->operator()(*engine));
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rnorm(epiworld_double mean, epiworld_double sd) {
    // CHECK_INIT()
    return (rnormd->operator()(*engine)) * sd + mean;
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rgamma() {
    return rgammad->operator()(*engine);
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rgamma(epiworld_double alpha, epiworld_double beta) {
    auto old_param = rgammad->param();
    rgammad->param(std::gamma_distribution<>::param_type(alpha, beta));
    epiworld_double ans = rgammad->operator()(*engine);
    rgammad->param(old_param);
    return ans;
}

template<typename TSeq>
inline void Model<TSeq>::seed(unsigned int s) {
    this->engine->seed(s);
}

template<typename TSeq>
inline void Model<TSeq>::add_virus(Virus<TSeq> v, epiworld_double preval)
{

    if (preval > 1.0)
        throw std::range_error("Prevalence of virus cannot be above 1.0");

    if (preval < 0.0)
        throw std::range_error("Prevalence of virus cannot be negative");

    // Checking the status
    epiworld_fast_int init_, post_, rm_;
    v.get_status(&init_, &post_, &rm_);

    if (init_ == -99)
        throw std::logic_error(
            "The virus \"" + v.get_name() + "\" has no -init- status."
            );
    else if (post_ == -99)
        throw std::logic_error(
            "The virus \"" + v.get_name() + "\" has no -post- status."
            );
    // else if (rm_ == -99)
    //     throw std::logic_error(
    //         "The virus \"" + v.get_name() + "\" has no -rm- status."
    //         );

    // Setting the id
    v.set_id(viruses.size());
    
    // Adding new virus
    viruses.push_back(std::make_shared< Virus<TSeq> >(v));
    prevalence_virus.push_back(preval);
    prevalence_virus_as_proportion.push_back(true);
    viruses_dist_funs.push_back(nullptr);

}

template<typename TSeq>
inline void Model<TSeq>::add_virus_n(Virus<TSeq> v, unsigned int preval)
{

    // Checking the ids
    epiworld_fast_int init_, post_, rm_;
    v.get_status(&init_, &post_, &rm_);

    if (init_ == -99)
        throw std::logic_error(
            "The virus \"" + v.get_name() + "\" has no -init- status."
            );
    else if (post_ == -99)
        throw std::logic_error(
            "The virus \"" + v.get_name() + "\" has no -post- status."
            );

    // Setting the id
    v.set_id(viruses.size());

    // Adding new virus
    viruses.push_back(std::make_shared< Virus<TSeq> >(v));
    prevalence_virus.push_back(preval);
    prevalence_virus_as_proportion.push_back(false);
    viruses_dist_funs.push_back(nullptr);

}

template<typename TSeq>
inline void Model<TSeq>::add_virus_fun(Virus<TSeq> v, VirusToAgentFun<TSeq> fun)
{

    // Checking the ids
    epiworld_fast_int init_, post_, rm_;
    v.get_status(&init_, &post_, &rm_);

    if (init_ == -99)
        throw std::logic_error(
            "The virus \"" + v.get_name() + "\" has no -init- status."
            );
    else if (post_ == -99)
        throw std::logic_error(
            "The virus \"" + v.get_name() + "\" has no -post- status."
            );

    // Setting the id
    v.set_id(viruses.size());

    // Adding new virus
    viruses.push_back(std::make_shared< Virus<TSeq> >(v));
    prevalence_virus.push_back(0.0);
    prevalence_virus_as_proportion.push_back(false);
    viruses_dist_funs.push_back(fun);

}

template<typename TSeq>
inline void Model<TSeq>::add_tool(Tool<TSeq> t, epiworld_double preval)
{

    if (preval > 1.0)
        throw std::range_error("Prevalence of tool cannot be above 1.0");

    if (preval < 0.0)
        throw std::range_error("Prevalence of tool cannot be negative");

    // Adding the tool to the model (and database.)
    tools.push_back(std::make_shared< Tool<TSeq> >(t));
    prevalence_tool.push_back(preval);
    prevalence_tool_as_proportion.push_back(true);
    tools_dist_funs.push_back(nullptr);

}

template<typename TSeq>
inline void Model<TSeq>::add_tool_n(Tool<TSeq> t, unsigned int preval)
{
    t.id = tools.size();
    tools.push_back(std::make_shared<Tool<TSeq> >(t));
    prevalence_tool.push_back(preval);
    prevalence_tool_as_proportion.push_back(false);
    tools_dist_funs.push_back(nullptr);
}

template<typename TSeq>
inline void Model<TSeq>::add_tool_fun(Tool<TSeq> t, ToolToAgentFun<TSeq> fun)
{
    t.id = tools.size();
    tools.push_back(std::make_shared<Tool<TSeq> >(t));
    prevalence_tool.push_back(0.0);
    prevalence_tool_as_proportion.push_back(false);
    tools_dist_funs.push_back(fun);
}


template<typename TSeq>
inline void Model<TSeq>::add_entity(Entity<TSeq> e, epiworld_double preval)
{

    if (preval > 1.0)
        throw std::range_error("Prevalence of entity cannot be above 1.0");

    if (preval < 0.0)
        throw std::range_error("Prevalence of entity cannot be negative");

    e.model = this;
    e.id = entities.size();
    entities.push_back(e);
    prevalence_entity.push_back(preval);
    prevalence_entity_as_proportion.push_back(false);
    entities_dist_funs.push_back(nullptr);

}

template<typename TSeq>
inline void Model<TSeq>::add_entity_n(Entity<TSeq> e, unsigned int preval)
{

    e.model = this;
    e.id = entities.size();
    entities.push_back(e);
    prevalence_entity.push_back(preval);
    prevalence_entity_as_proportion.push_back(false);
    entities_dist_funs.push_back(nullptr);

}

template<typename TSeq>
inline void Model<TSeq>::add_entity_fun(Entity<TSeq> e, EntityToAgentFun<TSeq> fun)
{

    e.id = entities.size();
    entities.push_back(e);
    prevalence_entity.push_back(0.0);
    prevalence_entity_as_proportion.push_back(false);
    entities_dist_funs.push_back(fun);

}

template<typename TSeq>
inline void Model<TSeq>::agents_from_adjlist(
    std::string fn,
    int size,
    int skip,
    bool directed
    ) {

    AdjList al;
    al.read_edgelist(fn, size, skip, directed);
    this->agents_from_adjlist(al);

}

template<typename TSeq>
inline void Model<TSeq>::agents_from_adjlist(AdjList al) {

    // Resizing the people
    population.clear();
    population.resize(al.vcount(), Agent<TSeq>());

    const auto & tmpdat = al.get_dat();

    // Filling the model and ids
    size_t i = 0u;
    for (auto & p : population)
    {
        p.model = this;
        p.id    = i++;
    }
    
    for (size_t i = 0u; i < tmpdat.size(); ++i)
    {

        population[i].id    = i;
        population[i].model = this;

        for (const auto & link: tmpdat[i])
        {

            population[i].add_neighbor(
                &population[link.first],
                true, true
                );

        }

    }

    #ifdef EPI_DEBUG
    for (auto & p: population)
    {
        if (p.id >= static_cast<int>(al.vcount()))
            throw std::logic_error(
                "Agent's id cannot be negative above or equal to the number of agents!");

        for (const auto & n : p.neighbors)
        {
            if (n == nullptr)
                throw std::logic_error("A neighbor cannot be nullptr!");
        }
    }
    #endif

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

    ++this->current_date;
    db.record();
    
    // Advancing the progress bar
    if (verbose)
        pb.next();

    #ifdef EPI_DEBUG
    // A possible check here
    #endif

    return ;
}

template<typename TSeq>
inline void Model<TSeq>::run() 
{

    if (size() == 0u)
        throw std::logic_error("There's no agents in this model!");

    // Initializing the simulation
    chrono_start();
    EPIWORLD_RUN((*this))
    {

        // We can execute these components in whatever order the
        // user needs.
        this->update_status();
    
        // We start with the global actions
        this->run_global_actions();

        // In this case we are applying degree sequence rewiring
        // to change the network just a bit.
        this->rewire();

        // This locks all the changes
        this->next();

        // Mutation must happen at the very end of all
        this->mutate_variant();

    }
    chrono_end();

}

template<typename TSeq>
inline void Model<TSeq>::run_multiple(
    unsigned int nexperiments,
    std::function<void(size_t,Model<TSeq>*)> fun,
    bool reset,
    bool verbose
)
{

    if (reset)
        set_backup();

    bool old_verb = this->verbose;
    verbose_off();

    Progress pb_multiple(
        nexperiments,
        EPIWORLD_PROGRESS_BAR_WIDTH
        )
        ;
    if (verbose)
    {

        printf_epiworld(
            "Starting multiple runs (%i)\n", 
            static_cast<int>(nexperiments)
        );

        pb_multiple.start();

    }

    for (unsigned int n = 0u; n < nexperiments; ++n)
    {
        
        run();

        if (fun)
            fun(n, this);

        if ((n < (nexperiments - 1u)) && reset)
            this->reset();

        if (verbose)
            pb_multiple.next();
    
    }

    if (verbose)
        pb_multiple.end();

    if (old_verb)
        verbose_on();

    return;

}

template<typename TSeq>
inline void Model<TSeq>::update_status() {

    // Next status
    if (use_queuing)
    {
        int i = -1;
        for (auto & p: population)
            if (queue[++i] > 0)
            {
                if (status_fun[p.status])
                    status_fun[p.status](&p, this);
            }

    }
    else
    {

        for (auto & p: population)
            if (status_fun[p.status])
                    status_fun[p.status](&p, this);

    }

    actions_run();
    
}

template<typename TSeq>
inline void Model<TSeq>::mutate_variant() {

    for (auto & p: population)
    {

        if (p.n_viruses > 0u)
            for (auto & v : p.viruses)
                v->mutate();

    }

}

template<typename TSeq>
inline size_t Model<TSeq>::get_n_variants() const {
    return db.size();
}

template<typename TSeq>
inline size_t Model<TSeq>::get_n_tools() const {
    return tools.size();
}

template<typename TSeq>
inline unsigned int Model<TSeq>::get_ndays() const {
    return ndays;
}

template<typename TSeq>
inline unsigned int Model<TSeq>::get_n_replicates() const
{
    return n_replicates;
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
    std::function<void(std::vector<Agent<TSeq>>*,Model<TSeq>*,epiworld_double)> fun
    ) {
    rewire_fun = fun;
}

template<typename TSeq>
inline void Model<TSeq>::set_rewire_prop(epiworld_double prop)
{

    if (prop < 0.0)
        throw std::range_error("Proportions cannot be negative.");

    if (prop > 1.0)
        throw std::range_error("Proportions cannot be above 1.0.");

    rewire_prop = prop;
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::get_rewire_prop() const {
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
    std::string fn_tool_info,
    std::string fn_tool_hist,
    std::string fn_total_hist,
    std::string fn_transmission,
    std::string fn_transition,
    std::string fn_reproductive_number
    ) const
{

    db.write_data(
        fn_variant_info, fn_variant_hist,
        fn_tool_info, fn_tool_hist,
        fn_total_hist, fn_transmission, fn_transition,
        fn_reproductive_number
        );

}

template<typename TSeq>
inline void Model<TSeq>::write_edgelist(
    std::string fn
    ) const
{

    // Figuring out the writing sequence
    std::vector< const Agent<TSeq> * > wseq(size());
    for (const auto & p: population)
        wseq[p.id] = &p;

    std::ofstream efile(fn, std::ios_base::out);
    efile << "source target\n";
    if (this->is_directed())
    {

        for (const auto & p : wseq)
        {
            for (auto & n : p->neighbors)
                efile << p->id << " " << n->id << "\n";
        }

    } else {

        for (const auto & p : wseq)
        {
            for (auto & n : p->neighbors)
                if (p->id <= n->id)
                    efile << p->id << " " << n->id << "\n";
        }

    }

}

template<typename TSeq>
inline std::map<std::string,epiworld_double> & Model<TSeq>::params()
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
            directed,
            this
        );
    }

    for (auto & p : population)
        p.reset();
    
    current_date = 0;

    db.set_model(*this);

    // Recording variants
    for (auto & v : viruses)
        db.record_variant(*v);

    // Recording tools
    for (auto & t : tools)
        db.record_tool(*t);

    if (use_queuing)
        queue.set_model(this);

    // Re distributing tools and virus
    dist_entities();
    dist_virus();
    dist_tools();

    // Recording the original state
    db.record();

}

// Too big to keep here
#include "model-meat-print.hpp"

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
        const Agent<TSeq> & agent_this = population[p];
        Agent<TSeq> & agent_res  = res.population[p];

        // Agent pointing to the right model and agent
        agent_res.model        = &res;
        agent_res.viruses.agent = &agent_res;
        agent_res.tools.agent = &agent_res;

        // Readding
        std::vector< Agent<TSeq> * > neigh = agent_this.neighbors;
        for (unsigned int n = 0u; n < neigh.size(); ++n)
        {
            // Point to the right neighbors
            int loc = res.population_ids[neigh[n]->get_id()];
            agent_res.add_neighbor(&res.population[loc], true, true);

        }

    }

    return res;

}



template<typename TSeq>
inline void Model<TSeq>::add_status(
    std::string lab, 
    UpdateFun<TSeq> fun
)
{
    if (this->initialized)
        throw std::logic_error("Cannot add status once the model has been initialized.");

    // Checking it doesn't match
    for (auto & s : status_labels)
        if (s == lab)
            throw std::logic_error("Status \"" + s + "\" already registered.");

    status_labels.push_back(lab);
    status_fun.push_back(fun);
    nstatus++;

}


template<typename TSeq>
inline const std::vector< std::string > &
Model<TSeq>::get_status() const
{
    return status_labels;
}

template<typename TSeq>
inline const std::vector< UpdateFun<TSeq> > &
Model<TSeq>::get_status_fun() const
{
    return status_fun;
}

template<typename TSeq>
inline void Model<TSeq>::print_status_codes() const
{

    // Horizontal line
    std::string line = "";
    for (unsigned int i = 0u; i < 80u; ++i)
        line += "_";

    printf_epiworld("\n%s\nSTATUS CODES\n\n", line.c_str());

    unsigned int nchar = 0u;
    for (auto & p : status_labels)
        if (p.length() > nchar)
            nchar = p.length();
    
    std::string fmt = " %2i = %-" + std::to_string(nchar + 1 + 4) + "s\n";
    for (unsigned int i = 0u; i < nstatus; ++i)
    {

        printf_epiworld(
            fmt.c_str(),
            i,
            (status_labels[i] + " (S)").c_str()
        );

    }

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
inline epiworld_double Model<TSeq>::add_param(
    epiworld_double initial_value,
    std::string pname
    ) {

    if (parameters.find(pname) == parameters.end())
        parameters[pname] = initial_value;

    CASES_PAR(npar_used++)
    
    return initial_value;

}

template<typename TSeq>
inline void Model<TSeq>::read_params(std::string fn)
{

    std::ifstream paramsfile(fn);

    if (!paramsfile)
        throw std::logic_error("The file " + fn + " was not found.");

    std::regex pattern("^([^:]+)\\s*[:]\\s*([0-9]+)(\\.[0-9]+)?\\s*$");

    std::string line;
    std::smatch match;
    auto empty = std::sregex_iterator();

    while (std::getline(paramsfile, line))
    {

        // Is it a comment or an empty line?
        if (std::regex_match(line, std::regex("^([*].+|//.+|#.+|\\s*)$")))
            continue;

        // Finding the patter, if it doesn't match, then error
        std::regex_match(line, match, pattern);

        if (match.empty())
            throw std::logic_error("The line does not match parameters:\n" + line);

        // Capturing the number
        std::string anumber = match[2u].str() + match[3u].str();
        epiworld_double tmp_num = static_cast<epiworld_double>(
            std::strtod(anumber.c_str(), nullptr)
            );

        // Trimming text
        

        add_param(
            tmp_num,
            std::regex_replace(
                match[1u].str(),
                std::regex("^\\s+|\\s+$"),
                "")
        );

    }

}

template<typename TSeq>
inline epiworld_double Model<TSeq>::get_param(std::string pname)
{
    if (parameters.find(pname) == parameters.end())
        throw std::logic_error("The parameter " + pname + " does not exists.");

    return parameters[pname];
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::par(std::string pname)
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
    epiworld_double * last_elapsed,
    epiworld_double * total_elapsed,
    std::string * unit_abbr,
    bool print
) const {

    // Preparing the result
    epiworld_double elapsed, elapsed_total;
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
    if (unit_abbr != nullptr)
        *unit_abbr = abbr_unit;

    if (!print)
        return;

    if (n_replicates > 1u)
    {
        printf_epiworld("last run elapsed time : %.2f%s\n",
            elapsed, abbr_unit.c_str());
        printf_epiworld("total elapsed time    : %.2f%s\n",
            elapsed_total, abbr_unit.c_str());
        printf_epiworld("total runs            : %i\n",
            static_cast<int>(n_replicates));
        printf_epiworld("mean run elapsed time : %.2f%s\n",
            elapsed_total/static_cast<epiworld_double>(n_replicates), abbr_unit.c_str());

    } else {
        printf_epiworld("last run elapsed time : %.2f%s.\n", elapsed, abbr_unit.c_str());
    }
}

template<typename TSeq>
inline void Model<TSeq>::set_user_data(std::vector< std::string > names)
{
    db.set_user_data(names);
}

template<typename TSeq>
inline void Model<TSeq>::add_user_data(unsigned int j, epiworld_double x)
{
    db.add_user_data(j, x);
}

template<typename TSeq>
inline void Model<TSeq>::add_user_data(std::vector<epiworld_double> x)
{
    db.add_user_data(x);
}

template<typename TSeq>
inline UserData<TSeq> & Model<TSeq>::get_user_data()
{
    return db.get_user_data();
}

template<typename TSeq>
inline void Model<TSeq>::add_global_action(
    std::function<void(Model<TSeq>*)> fun,
    int date
)
{

    global_action_functions.push_back(fun);
    global_action_dates.push_back(date);

}

template<typename TSeq>
inline void Model<TSeq>::run_global_actions()
{

    for (unsigned int i = 0u; i < global_action_dates.size(); ++i)
    {

        if (global_action_dates[i] < 0)
        {

            global_action_functions[i](this);

        }
        else if (global_action_dates[i] == today())
        {

            global_action_functions[i](this);

        }

        actions_run();

    }

}

template<typename TSeq>
inline void Model<TSeq>::queuing_on()
{
    use_queuing = true;
}

template<typename TSeq>
inline void Model<TSeq>::queuing_off()
{
    use_queuing = false;
}

template<typename TSeq>
inline bool Model<TSeq>::is_queuing_on() const
{
    return use_queuing;
}

template<typename TSeq>
inline Queue<TSeq> & Model<TSeq>::get_queue()
{
    return queue;
}

template<typename TSeq>
inline const std::vector< VirusPtr<TSeq> > & Model<TSeq>::get_viruses() const
{
    return viruses;
}

template<typename TSeq>
const std::vector< ToolPtr<TSeq> > & Model<TSeq>::get_tools() const
{
    return tools;
}

template<typename TSeq>
inline void Model<TSeq>::set_agents_data(double * data_, size_t ncols_)
{
    population_data = data_;
    population_data_n_features = ncols_;
}

#undef DURCAST

#undef CASES_PAR
#undef CASE_PAR

#undef CHECK_INIT
#endif
