#ifndef EPIWORLD_MODEL_MEAT_HPP
#define EPIWORLD_MODEL_MEAT_HPP

#define CHECK_INIT() if (!initialized) \
        throw std::logic_error("Model not initialized.");

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
inline double & Model<TSeq>::operator()(int i) {
    return parameters[i];
}

template<typename TSeq>
inline size_t Model<TSeq>::size() const {
    return persons.size();
}

template<typename TSeq>
inline void Model<TSeq>::init(int ndays, int seed) {

    EPIWORLD_CLOCK_START("(00) Init model")

    if (initialized) 
        throw std::logic_error("Model already initialized.");

    // Setting up the number of steps
    this->ndays = ndays;
    pb = Progress(ndays, 80);

    // Initializing persons
    for (auto & p : persons)
    {
        p.model = this;
        p.init();
    }

    // Has to happen after setting the persons
    db.set_model(*this);

    // Recording variants
    for (Virus<TSeq> & v : viruses)
        record_variant(&v);

    if (!engine)
        engine = std::make_shared< std::mt19937 >();

    engine->seed(seed);

    if (!runifd)
        runifd = std::make_shared< std::uniform_real_distribution<> >(0.0, 1.0);

    initialized = true;

    // Starting first infection
    for (unsigned int v = 0; v < viruses.size(); ++v)
    {
        for (auto & p : persons)
            if (runif() < prevalence_virus[v])
                p.add_virus(0, viruses[v]);

    }

    // Tools
    for (unsigned int t = 0; t < tools.size(); ++t)
    {
        for (auto & p : persons)
            if (runif() < prevalence_tool[t])
                p.add_tool(0, tools[t]);

    }

    EPIWORLD_CLOCK_END("(00) Init model")

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
    persons.resize(al.vcount(), Person<TSeq>());

    const auto & tmpdat = al.get_dat();
    for (const auto & n : tmpdat)
    {        
        persons[n.first].model = this;
        persons[n.first].id    = n.first;
        for (const auto & link: n.second)
            persons[n.first].add_neighbor(&persons[link.first]);

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
        if (p.status != DECEASED)
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
inline int Model<TSeq>::get_ndays() const {
    return ndays;
}

template<typename TSeq>
inline bool Model<TSeq>::get_verbose() const {
    return verbose;
}

template<typename TSeq>
inline void Model<TSeq>::rewire_degseq(int nrewires)
{

    // Only swap if needed
    int n = persons.size();
    while (nrewires-- > 0)
    {

        // Picking egos
        int id0 = std::floor(runif()*n);
        if (persons[id0].neighbors.size() == 0u)
            continue;

        Person<TSeq> & p0 = persons[id0];

        int id1 = std::floor(runif()*n - 1);     

        if (id1 < 0)
            id1 = 0;

        if (id1 == id0)
            id1++;

        if (persons[id1].neighbors.size() == 1u)
            continue;
        
        Person<TSeq> & p1 = persons[id1];

        // Picking alters
        int id01 = std::floor(p0.neighbors.size() * runif());
        int id11 = std::floor(p1.neighbors.size() * runif());

        // // Finding what neighbour is id0
        // int id0_in_id01 = 0;
        // for (auto n : persons[id0].neighbors->neighbors)
        //     if (persons[id0].id != id0_in_id01)

        // Moving alter first
        std::swap(p0.neighbors[id01], p1.neighbors[id11]);
        
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
inline std::vector<double> & Model<TSeq>::params()
{
    return parameters;
}

#undef CHECK_INIT
#endif