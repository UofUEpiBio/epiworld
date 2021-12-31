#ifndef EPIWORLD_MODEL_MEAT_HPP
#define EPIWORLD_MODEL_MEAT_HPP

#define CHECK_INIT() if (!initialized) \
        throw std::logic_error("Model not initialized.");

template<typename TSeq>
inline Model<TSeq>::Model(const Model<TSeq> & model) :
    db(model.db),
    persons(model.persons),
    persons_ids(model.persons_ids),
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
    for (auto & p: persons)
        p.neighbors.clear();
    
    // Rechecking individuals
    for (unsigned int p = 0u; p < size(); ++p)
    {
        // Making room
        const Person<TSeq> & person_this = model.persons[p];
        Person<TSeq> & person_res  = persons[p];

        // Person pointing to the right model and person
        person_res.model        = this;
        person_res.viruses.host = &person_res;
        person_res.tools.person = &person_res;

        // Readding
        std::vector< Person<TSeq> * > neigh = person_this.neighbors;
        for (unsigned int n = 0u; n < neigh.size(); ++n)
        {
            // Point to the right neighbors
            int loc = persons_ids[neigh[n]->get_id()];
            person_res.add_neighbor(&persons[loc], true, true);

        }

    }

    // Finally, seeds are resetted automatically based on the original
    // engine
    seed(floor(model.runifd() * UINT_MAX));

}

template<typename TSeq>
inline DataBase<TSeq> & Model<TSeq>::get_db()
{
    return db;
}

template<typename TSeq>
inline std::vector<Person<TSeq>> * Model<TSeq>::get_persons()
{
    return &persons;
}

template<typename TSeq>
inline void Model<TSeq>::set_rand_engine(std::mt19937 & eng)
{
    engine = std::make_shared< std::mt19937 >(eng);
}

template<typename TSeq>
inline double & Model<TSeq>::operator()(std::string pname) {

    if (parameters.find(pname) == parameters.end())
        throw std::range_error("The parameter "+ pname + "is not in the model.");

    return parameters[pname];

}

template<typename TSeq>
inline size_t Model<TSeq>::size() const {
    return persons.size();
}

template<typename TSeq>
inline void Model<TSeq>::init(
    unsigned int ndays,
    unsigned int seed
    ) {

    EPIWORLD_CLOCK_START("(00) Init model")

    if (initialized) 
        throw std::logic_error("Model already initialized.");

    // Setting up the number of steps
    this->ndays = ndays;

    // Initializing persons
    for (auto & p : persons)
    {
        p.model = this;
        p.init();
    }

    if (!engine)
        engine = std::make_shared< std::mt19937 >();

    engine->seed(seed);

    if (!runifd)
        runifd = std::make_shared< std::uniform_real_distribution<> >(0.0, 1.0);

    initialized = true;

    // Starting first infection and tools
    reset();

    EPIWORLD_CLOCK_END("(00) Init model")

}

template<typename TSeq>
inline void Model<TSeq>::dist_virus()
{

    // Starting first infection
    for (unsigned int v = 0; v < viruses.size(); ++v)
    {
        for (auto & p : persons)
            if (runif() < prevalence_virus[v])
                p.add_virus(0, viruses[v]);

    }

}

template<typename TSeq>
inline void Model<TSeq>::dist_tools()
{
    // Tools
    for (unsigned int t = 0; t < tools.size(); ++t)
    {
        for (auto & p : persons)
            if (runif() < prevalence_tool[t])
                p.add_tool(0, tools[t]);

    }

}

template<typename TSeq>
inline std::mt19937 * Model<TSeq>::get_rand_endgine()
{
    return engine.get();
}

template<typename TSeq>
inline double Model<TSeq>::runif() {
    CHECK_INIT()
    return runifd->operator()(*engine);
}

template<typename TSeq>
inline void Model<TSeq>::seed(unsigned int s) {
    this->engine->seed(s);
}

template<typename TSeq>
inline void Model<TSeq>::add_virus(Virus<TSeq> v, double preval)
{
    
    viruses.push_back(v);
    prevalence_virus.push_back(preval);

}

template<typename TSeq>
inline void Model<TSeq>::add_tool(Tool<TSeq> t, double preval)
{
    tools.push_back(t);
    prevalence_tool.push_back(preval);
}

template<typename TSeq>
inline void Model<TSeq>::pop_from_adjlist(std::string fn, int skip, bool directed) {

    AdjList al;
    al.read_edgelist(fn, skip, directed);
    this->pop_from_adjlist(al);

}

template<typename TSeq>
inline void Model<TSeq>::pop_from_adjlist(AdjList al) {

    // Resizing the people
    persons.clear();
    persons_ids.clear();
    persons.resize(al.vcount(), Person<TSeq>());

    const auto & tmpdat = al.get_dat();
    
    int loc;
    for (const auto & n : tmpdat)
    {
        if (persons_ids.find(n.first) == persons_ids.end())
            persons_ids[n.first] = persons_ids.size();

        loc = persons_ids[n.first];

        persons[loc].model = this;
        persons[loc].id    = n.first;
        persons[loc].index = loc;

        for (const auto & link: n.second)
        {
            if (persons_ids.find(link.first) == persons_ids.end())
                persons_ids[link.first] = persons_ids.size();

            unsigned int loc_link   = persons_ids[link.first];
            persons[loc_link].id    = link.first;
            persons[loc_link].index = loc_link;

            persons[loc].add_neighbor(
                &persons[persons_ids[link.first]],
                true, true
                );
        }

    }

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
inline void Model<TSeq>::update_status() {

    // Next status
    for (auto & p: persons)
        p.update_status();

    // Making the change effective
    for (auto & p: persons)
        if (p.status != STATUS::DECEASED)
            p.status = p.status_next;

}

template<typename TSeq>
inline void Model<TSeq>::mutate_variant() {

    for (auto & p: persons)
    {
        if (p.get_status() == INFECTED)
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
inline const std::vector<TSeq> & Model<TSeq>::get_variant_sequence() const {
    return db.get_sequence();
}

template<typename TSeq>
inline const std::vector<int> & Model<TSeq>::get_variant_nifected() const {
    return db.get_today_variant("ninfected");
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
inline void Model<TSeq>::rewire_degseq(int nrewires)
{

    // Only swap if needed
    int N = persons.size();
    while (nrewires-- > 0)
    {

        // Picking egos
        int id0 = std::floor(runif() * N);
        if (persons[id0].neighbors.size() == 0u)
            continue;

        Person<TSeq> & p0 = persons[id0];

        int id1 = std::floor(runif() * N - 1);     

        // Correcting for under or overflow.
        if (id1 < 0)
            id1 = 0;

        if (id1 == id0)
            id1++;

        // If the same chose, then keep going.
        if (persons[id1].neighbors.size() == 1u)
            continue;
        
        Person<TSeq> & p1 = persons[id1];

        // Picking alters (relative location in their lists)
        int id01 = std::floor(p0.neighbors.size() * runif());
        int id11 = std::floor(p1.neighbors.size() * runif());

        // When rewiring, we need to flip the individuals from the other
        // end as well, since we are dealing withi an undirected graph

        // Finding what neighbour is id0
        unsigned int n0,n1;
        Person<TSeq> & p01 = persons[p0.neighbors[id01]->index];
        for (n0 = 0; n0 < p01.neighbors.size(); ++n0)
        {
            if (p0.id == p01.neighbors[n0]->get_id())
                break;            
        }

        Person<TSeq> & p11 = persons[p1.neighbors[id11]->index];
        for (n1 = 0; n1 < p11.neighbors.size(); ++n1)
        {
            if (p1.id == p11.neighbors[n1]->get_id())
                break;            
        }

        // Moving alter first
        std::swap(p0.neighbors[id01], p1.neighbors[id11]);
        std::swap(p01.neighbors[n0], p11.neighbors[n1]);
        
    }

    return;

}

template<typename TSeq>
inline void Model<TSeq>::write_data(
    std::string fn_variant,
    std::string fn_total
    ) const
{

}

template<typename TSeq>
inline void Model<TSeq>::write_edgelist(
    std::string fn
    ) const
{

    EPIWORLD_CLOCK_START("(03) Writing edgelist")

    std::ofstream efile(fn, std::ios_base::out);
    efile << "source target\n";
    for (const auto & p : persons)
    {
        for (auto & n : p.neighbors)
            efile << p.id << " " << n->id << "\n";
    }

    EPIWORLD_CLOCK_END("(03) Writing edgelist")

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

    for (auto & p : persons)
        p.reset();
    
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
    printf_epiworld("Population size   : %i\n", static_cast<int>(size()));
    printf_epiworld("Days (duration)   : %i (of %i)\n", today(), ndays);
    printf_epiworld("Number of variants: %i\n\n", static_cast<int>(db.get_nvariants()));
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

    printf_epiworld("\nStatistics:\n");
    printf_epiworld(" - Total variants active : %i\n\n", db.get_today_total("nvariants_active"));
    printf_epiworld(" - Total healthy         : %i\n", db.get_today_total("nhealthy"));
    printf_epiworld(" - Total infected        : %i\n", db.get_today_total("ninfected"));
    printf_epiworld(" - Total removed        : %i\n\n", db.get_today_total("nremoved"));
    printf_epiworld(" - Total # of recoveries : %i\n\n", db.get_today_total("nrecovered"));

    // Information about the parameters included
    printf_epiworld("Model parameters:\n");
    unsigned int nchar = 0u;
    for (auto & p : parameters)
        if (p.first.length() > nchar)
            nchar = p.first.length();

    std::string fmt = " - %-" + std::to_string(nchar + 1) + "s : ";
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
    for (auto & p: res.persons)
        p.neighbors.clear();
    
    // Rechecking individuals
    for (unsigned int p = 0u; p < size(); ++p)
    {
        // Making room
        const Person<TSeq> & person_this = persons[p];
        Person<TSeq> & person_res  = res.persons[p];

        // Person pointing to the right model and person
        person_res.model        = &res;
        person_res.viruses.host = &person_res;
        person_res.tools.person = &person_res;

        // Readding
        std::vector< Person<TSeq> * > neigh = person_this.neighbors;
        for (unsigned int n = 0u; n < neigh.size(); ++n)
        {
            // Point to the right neighbors
            int loc = res.persons_ids[neigh[n]->get_id()];
            person_res.add_neighbor(&res.persons[loc], true, true);

        }

    }

    return res;

}

#undef CHECK_INIT
#endif