#ifndef EPIWORLD_DATABASE_MEAT_HPP
#define EPIWORLD_DATABASE_MEAT_HPP

template<typename TSeq>
inline void DataBase<TSeq>::set_model(Model<TSeq> & m)
{
    model = &m;
    user_data.model = &m;

    reset();

    // Initializing the counts
    today_total.resize(m.nstatus);
    std::fill(today_total.begin(), today_total.end(), 0);
    for (auto & p : *m.get_population())
        ++today_total[p.get_status()];
    
    today_total_next.resize(m.nstatus);
    std::fill(today_total_next.begin(), today_total_next.end(), 0);

    transition_matrix.resize(m.nstatus * m.nstatus);
    std::fill(transition_matrix.begin(), transition_matrix.end(), 0);

    transition_matrix_next.resize(m.nstatus * m.nstatus);
    std::fill(transition_matrix_next.begin(), transition_matrix_next.end(), 0);

    return;

}

template<typename TSeq>
inline Model<TSeq> * DataBase<TSeq>::get_model() {
    return model;
}

template<typename TSeq>
inline const std::vector< TSeq > & DataBase<TSeq>::get_sequence() const {
    return sequence;
}

template<typename TSeq>
inline void DataBase<TSeq>::record() 
{

    // Updating values according to today's changes
    for (auto i = 0u; i < model->nstatus; ++i)
    {
        today_total[i] += today_total_next[i];
        today_total_next[i] = 0;
    }

    ////////////////////////////////////////////////////////////////////////////
    // DEBUGGING BLOCK
    ////////////////////////////////////////////////////////////////////////////
    EPI_DEBUG_SUM_INT(today_total, model->size())
    EPI_DEBUG_ALL_NON_NEGATIVE(today_total)

    #ifdef EPI_DEBUG
    // Checking whether the sums correspond
    std::vector< int > _today_total_cp(today_total.size(), 0);
    for (auto & p : model->population)
        _today_total_cp[p.get_status()]++;
    
    EPI_DEBUG_VECTOR_MATCH_INT(_today_total_cp, today_total)
    #endif
    ////////////////////////////////////////////////////////////////////////////

    for (auto v = 0u; v < today_variant.size(); ++v)
    {

        for (auto i = 0u; i < model->nstatus; ++i)
        {
            today_variant[v][i] += today_variant_next[v][i];
            today_variant_next[v][i] = 0;
        }

    }
    
    for (auto i = 0u; i < transition_matrix.size(); ++i)
    {
        transition_matrix[i] += transition_matrix_next[i];
        transition_matrix_next[i] = 0;
    }

    // Only store every now and then
    if ((model->today() % sampling_freq) == 0)
    {

        // Recording variant's history
        for (auto & p : variant_id)
        {

            for (unsigned int s = 0u; s < model->nstatus; ++s)
            {

                hist_variant_date.push_back(model->today());
                hist_variant_id.push_back(p.second);
                hist_variant_status.push_back(s);
                hist_variant_counts.push_back(today_variant[p.second][s]);

            }

        }

        // Recording the overall history
        for (unsigned int s = 0u; s < model->nstatus; ++s)
        {
            hist_total_date.push_back(model->today());
            hist_total_nvariants_active.push_back(today_total_nvariants_active);
            hist_total_status.push_back(s);
            hist_total_counts.push_back(today_total[s]);
        }

        for (auto cell : transition_matrix)
            hist_transition_matrix.push_back(cell);

    }

}

template<typename TSeq>
inline void DataBase<TSeq>::record_variant(Virus<TSeq> * v)
{

    // Updating registry
    std::vector< int > hash = seq_hasher(*v->get_sequence());
    unsigned int old_id = v->get_id();
    unsigned int new_id;
    if (variant_id.find(hash) == variant_id.end())
    {

        new_id = variant_id.size();
        variant_id[hash] = new_id;
        sequence.push_back(*v->get_sequence());
        origin_date.push_back(model->today());
        
        parent_id.push_back(old_id);
        
        today_variant.push_back({});
        today_variant[new_id].resize(
            model->status_susceptible.size() +
            model->status_exposed.size() +
            model->status_removed.size(),
            0
        );

        today_variant_next.push_back(today_variant[new_id]);
        
        // Updating the variant
        v->set_id(new_id);
        v->set_date(model->today());

        today_total_nvariants_active++;

    } else {

        // Finding the id
        new_id = variant_id[hash];

        // Reflecting the change
        v->set_id(new_id);
        v->set_date(origin_date[new_id]);

    }

    // Moving statistics (only if we are affecting an individual)
    if (v->get_host() != nullptr)
    {
        // Correcting math
        epiworld_fast_uint tmp_status = v->get_host()->get_status();
        today_variant_next[old_id][tmp_status]--;
        today_variant_next[new_id][tmp_status]++;

    }
    
    return;
} 

template<typename TSeq>
inline size_t DataBase<TSeq>::size() const
{
    return variant_id.size();
}

template<typename TSeq>
inline void DataBase<TSeq>::up_exposed(
    Virus<TSeq> * v,
    epiworld_fast_uint new_status
) {

    today_variant_next[v->get_id()][new_status]++;

}

template<typename TSeq>
inline void DataBase<TSeq>::down_exposed( 
    Virus<TSeq> * v,
    epiworld_fast_uint prev_status
) {

    today_variant_next[v->get_id()][prev_status]--;

}

template<typename TSeq>
inline void DataBase<TSeq>::state_change(
        epiworld_fast_uint prev_status,
        epiworld_fast_uint new_status
) {
    today_total_next[prev_status]--;
    today_total_next[new_status]++;
    return;
}

template<typename TSeq>
inline void DataBase<TSeq>::record_transition(
    epiworld_fast_uint from,
    epiworld_fast_uint to
) {

    transition_matrix_next[to * model->nstatus + from]++;

}

#define EPIWORLD_GET_STATUS_LABELS(stdstrvec) \
    stdstrvec.resize(model->nstatus); \
    for (epiworld_fast_uint i = 0u; i < model->status_susceptible.size(); ++i) \
        stdstrvec[model->status_susceptible[i]] = model->status_susceptible_labels[i]; \
    for (epiworld_fast_uint i = 0u; i < model->status_exposed.size(); ++i) \
        stdstrvec[model->status_exposed[i]] = model->status_exposed_labels[i]; \
    for (epiworld_fast_uint i = 0u; i < model->status_removed.size(); ++i) \
        stdstrvec[model->status_removed[i]] = model->status_removed_labels[i];


template<typename TSeq>
inline int DataBase<TSeq>::get_today_total(
    std::string what
) const
{
    std::vector< std::string > labels;
    EPIWORLD_GET_STATUS_LABELS(labels)

    for (auto i = 0u; i < labels.size(); ++i)
    {
        if (labels[i] == what)
            return today_total[i];
    }

    throw std::range_error("The value '" + what + "' is not in the model.");

}

template<typename TSeq>
inline void DataBase<TSeq>::get_today_total(
    std::vector< std::string > * status,
    std::vector< int > * counts
) const
{
    if (status != nullptr)
    {
        EPIWORLD_GET_STATUS_LABELS((*status))
    }

    if (counts != nullptr)
        *counts = today_total;

}

template<typename TSeq>
inline void DataBase<TSeq>::get_today_variant(
    std::vector< std::string > & status,
    std::vector< int > & id,
    std::vector< int > & counts
    ) const
{
    
    std::vector< std::string > labels;
    EPIWORLD_GET_STATUS_LABELS(labels)    
    status.resize(today_variant.size(), "");
    id.resize(today_variant.size(), 0);
    counts.resize(today_variant.size(),0);

    int n = 0u;
    for (unsigned int v = 0u; v < today_variant.size(); ++v)
        for (unsigned int s = 0u; s < labels.size(); ++s)
        {
            status[n]   = labels[s];
            id[n]       = static_cast<int>(v);
            counts[n++] = today_variant[v][s];

        }

}

template<typename TSeq>
inline void DataBase<TSeq>::get_hist_total(
    std::vector< int > * date,
    std::vector< std::string > * status,
    std::vector< int > * counts
) const
{

    if (date != nullptr)
        *date = hist_total_date;

    std::vector< std::string > labels;
    EPIWORLD_GET_STATUS_LABELS(labels)

    if (status != nullptr)
    {
        status->resize(hist_total_status.size(), "");
        for (unsigned int i = 0u; i < hist_total_status.size(); ++i)
            status->operator[](i) = labels[hist_total_status[i]];
    }

    if (counts != nullptr)
        *counts = hist_total_counts;

    return;

}

template<typename TSeq>
inline void DataBase<TSeq>::get_hist_variant(
    std::vector< int > & date,
    std::vector< int > & id,
    std::vector< std::string > & status,
    std::vector< int > & counts
) const {

    date = hist_variant_date;
    std::vector< std::string > labels;
    EPIWORLD_GET_STATUS_LABELS(labels)
    id = hist_variant_id;
    status.resize(hist_variant_status.size(), "");
    for (unsigned int i = 0u; i < hist_variant_status.size(); ++i)
        status[i] = labels[hist_variant_status[i]];

    counts = hist_variant_counts;

    return;

}

#undef EPIWORLD_GET_STATUS_LABELS

template<typename TSeq>
inline void DataBase<TSeq>::write_data(
    std::string fn_variant_info,
    std::string fn_variant_hist,
    std::string fn_total_hist,
    std::string fn_transmission,
    std::string fn_transition
) const
{

    if (fn_variant_info != "")
    {
        std::ofstream file_variant_info(fn_variant_info, std::ios_base::out);

        file_variant_info <<
            "id " << "sequence " << "date " << "parent " << "patiente\n";

        for (const auto & v : variant_id)
        {
            int id = v.second;
            file_variant_info <<
                id << " " <<
                seq_writer(sequence[id]) << " " <<
                origin_date[id] << " " <<
                parent_id[id] << " " <<
                parent_id[id] << "\n";
        }

    }

    // Preparing labels
    std::vector< std::string > labels(model->nstatus);
    for (unsigned int i = 0u; i < model->status_susceptible.size(); ++i)
        labels[model->status_susceptible[i]] =
            model->status_susceptible_labels[i];
    
    for (unsigned int i = 0u; i < model->status_exposed.size(); ++i)
        labels[model->status_exposed[i]] =
            model->status_exposed_labels[i];

    for (unsigned int i = 0u; i < model->status_removed.size(); ++i)
        labels[model->status_removed[i]] =
            model->status_removed_labels[i];

    if (fn_variant_hist != "")
    {
        std::ofstream file_variant(fn_variant_hist, std::ios_base::out);
        
        file_variant <<
            "date " << "id " << "status " << "n\n";

        for (unsigned int i = 0; i < hist_variant_id.size(); ++i)
            file_variant <<
                hist_variant_date[i] << " " <<
                hist_variant_id[i] << " " <<
                labels[hist_variant_status[i]] << " " <<
                hist_variant_counts[i] << "\n";
    }

    if (fn_total_hist != "")
    {
        std::ofstream file_total(fn_total_hist, std::ios_base::out);

        file_total <<
            "date " << "nvariants " << "status " << "counts\n";

        for (unsigned int i = 0; i < hist_total_date.size(); ++i)
            file_total <<
                hist_total_date[i] << " " <<
                hist_total_nvariants_active[i] << " \"" <<
                labels[hist_total_status[i]] << "\" " << 
                hist_total_counts[i] << "\n";
    }

    if (fn_transmission != "")
    {
        std::ofstream file_transmission(fn_transmission, std::ios_base::out);
        file_transmission <<
            "date " << "variant " << "source " << "target\n";

        for (unsigned int i = 0; i < transmission_target.size(); ++i)
            file_transmission <<
                transmission_date[i] << " " <<
                transmission_variant[i] << " " <<
                transmission_source[i] << " " <<
                transmission_target[i] << "\n";
                
    }

    if (fn_transition != "")
    {
        std::ofstream file_transition(fn_transition, std::ios_base::out);
        file_transition <<
            "date " << "from " << "to " << "counts\n";

        int ns = model->nstatus;

        for (int i = 0; i <= model->today(); ++i)
        {

            for (int from = 0u; from < ns; ++from)
                for (int to = 0u; to < ns; ++to)
                    file_transition <<
                        i << " " <<
                        labels[from] << " " <<
                        labels[to] << " " <<
                        hist_transition_matrix[i * (ns * ns) + to * ns + from] << "\n";
                
        }
                
    }

}

template<typename TSeq>
inline void DataBase<TSeq>::record_transmission(
    int i,
    int j,
    int variant
) {

    transmission_date.push_back(model->today());
    transmission_source.push_back(i);
    transmission_target.push_back(j);
    transmission_variant.push_back(variant);

}

template<typename TSeq>
inline size_t DataBase<TSeq>::get_nvariants() const
{
    return variant_id.size();
}

template<typename TSeq>
inline void DataBase<TSeq>::reset()
{

    variant_id.clear();
    sequence.clear();
    origin_date.clear();
    parent_id.clear();
    
    hist_variant_date.clear();
    hist_variant_id.clear();
    hist_variant_status.clear();
    hist_variant_counts.clear();
    
    hist_total_date.clear();
    hist_total_nvariants_active.clear();
    hist_total_status.clear();
    hist_total_counts.clear();
    
    transmission_date.clear();
    transmission_source.clear();
    transmission_target.clear();
    transmission_variant.clear();

    today_total_nvariants_active = 0;

    today_total.clear();
    today_total_next.clear();
    
    today_variant.clear();
    today_variant_next.clear();

}

template<typename TSeq>
inline void DataBase<TSeq>::set_user_data(
    std::vector< std::string > names
)
{
    user_data = UserData<TSeq>(names);
    user_data.model = model;
}

template<typename TSeq>
inline void DataBase<TSeq>::add_user_data(
    std::vector< epiworld_double > x
)
{

    user_data.add(x);

}

template<typename TSeq>
inline void DataBase<TSeq>::add_user_data(
    unsigned int k,
    epiworld_double x
)
{

    user_data.add(k, x);

}

template<typename TSeq>
inline UserData<TSeq> & DataBase<TSeq>::get_user_data()
{
    return user_data;
}

#endif