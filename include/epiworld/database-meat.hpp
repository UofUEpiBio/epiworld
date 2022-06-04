#ifndef EPIWORLD_DATABASE_MEAT_HPP
#define EPIWORLD_DATABASE_MEAT_HPP

template<typename TSeq>
inline void DataBase<TSeq>::set_model(Model<TSeq> & m)
{
    model           = &m;
    user_data.model = &m;

    reset();

    // Initializing the counts
    today_total.resize(m.nstatus);
    std::fill(today_total.begin(), today_total.end(), 0);
    for (auto & p : *m.get_agents())
        ++today_total[p.get_status()];
    
    transition_matrix.resize(m.nstatus * m.nstatus);
    std::fill(transition_matrix.begin(), transition_matrix.end(), 0);
    for (size_t s = 0u; s < m.nstatus; ++s)
        transition_matrix[s + s*m.nstatus] = today_total[s];


    return;

}

template<typename TSeq>
inline Model<TSeq> * DataBase<TSeq>::get_model() {
    return model;
}

template<typename TSeq>
inline const std::vector< TSeq > & DataBase<TSeq>::get_sequence() const {
    return variant_sequence;
}

template<typename TSeq>
inline void DataBase<TSeq>::record() 
{

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

        // Recording tool's history
        for (auto & p : tool_id)
        {

            for (unsigned int s = 0u; s < model->nstatus; ++s)
            {

                hist_tool_date.push_back(model->today());
                hist_tool_id.push_back(p.second);
                hist_tool_status.push_back(s);
                hist_tool_counts.push_back(today_tool[p.second][s]);

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

        // Now the diagonal must reflect the state
        for (size_t s_i = 0u; s_i < model->nstatus; ++s_i)
        {
            for (size_t s_j = 0u; s_j < model->nstatus; ++s_j)
            {
                if ((s_i != s_j) && (transition_matrix[s_i + s_j * model->nstatus] > 0))
                {
                    transition_matrix[s_j + s_j * model->nstatus] +=
                        transition_matrix[s_i + s_j * model->nstatus];

                    transition_matrix[s_i + s_j * model->nstatus] = 0;
                }
         
            }

            #ifdef EPI_DEBUG
            if (transition_matrix[s_i + s_i * model->nstatus] != 
                today_total[s_i])
                throw std::logic_error(
                    "The diagonal of the updated transition Matrix should match the daily totals"
                    );
            #endif
        }


    }

}

template<typename TSeq>
inline void DataBase<TSeq>::record_variant(Virus<TSeq> & v)
{

    // Updating registry
    std::vector< int > hash = seq_hasher(*v.get_sequence());
    unsigned int old_id = v.get_id();
    unsigned int new_id;
    if (variant_id.find(hash) == variant_id.end())
    {

        new_id = variant_id.size();
        variant_id[hash] = new_id;
        variant_name.push_back(v.get_name());
        variant_sequence.push_back(*v.get_sequence());
        variant_origin_date.push_back(model->today());
        
        variant_parent_id.push_back(old_id);
        
        today_variant.push_back({});
        today_variant[new_id].resize(model->nstatus, 0);
       
        // Updating the variant
        v.set_id(new_id);
        v.set_date(model->today());

        today_total_nvariants_active++;

    } else {

        // Finding the id
        new_id = variant_id[hash];

        // Reflecting the change
        v.set_id(new_id);
        v.set_date(variant_origin_date[new_id]);

    }

    // Moving statistics (only if we are affecting an individual)
    if (v.get_agent() != nullptr)
    {
        // Correcting math
        epiworld_fast_uint tmp_status = v.get_agent()->get_status();
        today_variant[old_id][tmp_status]--;
        today_variant[new_id][tmp_status]++;

    }
    
    return;
} 

template<typename TSeq>
inline void DataBase<TSeq>::record_tool(Tool<TSeq> & t)
{

    // Updating registry
    std::vector< int > hash = seq_hasher(*t.get_sequence());
    unsigned int old_id = t.get_id();
    unsigned int new_id;
    if (tool_id.find(hash) == tool_id.end())
    {

        new_id = tool_id.size();
        tool_id[hash] = new_id;
        tool_name.push_back(t.get_name());
        tool_sequence.push_back(*t.get_sequence());
        tool_origin_date.push_back(model->today());
                
        today_tool.push_back({});
        today_tool[new_id].resize(model->nstatus, 0);

        // Updating the tool
        t.set_id(new_id);
        t.set_date(model->today());

    } else {

        // Finding the id
        new_id = tool_id[hash];

        // Reflecting the change
        t.set_id(new_id);
        t.set_date(tool_origin_date[new_id]);

    }

    // Moving statistics (only if we are affecting an individual)
    if (t.get_agent() != nullptr)
    {
        // Correcting math
        epiworld_fast_uint tmp_status = t.get_agent()->get_status();
        today_tool[old_id][tmp_status]--;
        today_tool[new_id][tmp_status]++;

    }
    
    return;
} 

template<typename TSeq>
inline size_t DataBase<TSeq>::size() const
{
    return variant_id.size();
}

template<typename TSeq>
inline void DataBase<TSeq>::update_state(
        epiworld_fast_uint prev_status,
        epiworld_fast_uint new_status
) {

    today_total[prev_status]--;
    today_total[new_status]++;

    record_transition(prev_status, new_status);
    
    return;
}

template<typename TSeq>
inline void DataBase<TSeq>::update_virus(
        epiworld_fast_uint virus_id,
        epiworld_fast_uint prev_status,
        epiworld_fast_uint new_status
) {

    today_variant[virus_id][prev_status]--;
    today_variant[virus_id][new_status]++;

    return;
    
}

template<typename TSeq>
inline void DataBase<TSeq>::update_tool(
        epiworld_fast_uint tool_id,
        epiworld_fast_uint prev_status,
        epiworld_fast_uint new_status
) {


    today_tool[tool_id][prev_status]--;    
    today_tool[tool_id][new_status]++;

    return;

}

template<typename TSeq>
inline void DataBase<TSeq>::record_transition(
    epiworld_fast_uint from,
    epiworld_fast_uint to
) {

    transition_matrix[to * model->nstatus + from]++;
    transition_matrix[from * model->nstatus + from]--;

    #ifdef EPI_DEBUG
    if (transition_matrix[from * model->nstatus + from] < 0)
        throw std::logic_error("An entry in transition matrix is negative.");
    #endif

}

template<typename TSeq>
inline int DataBase<TSeq>::get_today_total(
    std::string what
) const
{

    for (auto i = 0u; i < model->status_labels.size(); ++i)
    {
        if (model->status_labels[i] == what)
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
        (*status) = model->status_labels;

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
      
    status.resize(today_variant.size(), "");
    id.resize(today_variant.size(), 0);
    counts.resize(today_variant.size(),0);

    int n = 0u;
    for (unsigned int v = 0u; v < today_variant.size(); ++v)
        for (unsigned int s = 0u; s < model->status_labels.size(); ++s)
        {
            status[n]   = model->status_labels[s];
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

    if (status != nullptr)
    {
        status->resize(hist_total_status.size(), "");
        for (unsigned int i = 0u; i < hist_total_status.size(); ++i)
            status->operator[](i) = model->status_labels[hist_total_status[i]];
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
    labels = model->status_labels;
    
    id = hist_variant_id;
    status.resize(hist_variant_status.size(), "");
    for (unsigned int i = 0u; i < hist_variant_status.size(); ++i)
        status[i] = labels[hist_variant_status[i]];

    counts = hist_variant_counts;

    return;

}

template<typename TSeq>
inline void DataBase<TSeq>::write_data(
    std::string fn_variant_info,
    std::string fn_variant_hist,
    std::string fn_tool_info,
    std::string fn_tool_hist,
    std::string fn_total_hist,
    std::string fn_transmission,
    std::string fn_transition
) const
{

    if (fn_variant_info != "")
    {
        std::ofstream file_variant_info(fn_variant_info, std::ios_base::out);

        file_variant_info <<
            "id " << "variant_name " << "variant_sequence " << "date_recorded " << "parent\n";

        for (const auto & v : variant_id)
        {
            int id = v.second;
            file_variant_info <<
                id << " \"" <<
                variant_name[id] << "\" " <<
                seq_writer(variant_sequence[id]) << " " <<
                variant_origin_date[id] << " " <<
                variant_parent_id[id] << "\n";
        }

    }

    if (fn_variant_hist != "")
    {
        std::ofstream file_variant(fn_variant_hist, std::ios_base::out);
        
        file_variant <<
            "date " << "id " << "status " << "n\n";

        for (unsigned int i = 0; i < hist_variant_id.size(); ++i)
            file_variant <<
                hist_variant_date[i] << " " <<
                hist_variant_id[i] << " " <<
                model->status_labels[hist_variant_status[i]] << " " <<
                hist_variant_counts[i] << "\n";
    }

    if (fn_tool_info != "")
    {
        std::ofstream file_tool_info(fn_tool_info, std::ios_base::out);

        file_tool_info <<
            "id " << "tool_name " << "tool_sequence " << "date_recorded\n";

        for (const auto & t : tool_id)
        {
            int id = t.second;
            file_tool_info <<
                id << " \"" <<
                tool_name[id] << "\" " <<
                seq_writer(tool_sequence[id]) << " " <<
                tool_origin_date[id] << "\n";
        }

    }

    if (fn_tool_hist != "")
    {
        std::ofstream file_tool_hist(fn_tool_hist, std::ios_base::out);
        
        file_tool_hist <<
            "date " << "id " << "status " << "n\n";

        for (unsigned int i = 0; i < hist_tool_id.size(); ++i)
            file_tool_hist <<
                hist_tool_date[i] << " " <<
                hist_tool_id[i] << " " <<
                model->status_labels[hist_tool_status[i]] << " " <<
                hist_tool_counts[i] << "\n";
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
                model->status_labels[hist_total_status[i]] << "\" " << 
                hist_total_counts[i] << "\n";
    }

    if (fn_transmission != "")
    {
        std::ofstream file_transmission(fn_transmission, std::ios_base::out);
        file_transmission <<
            "date " << "variant " << "source_exposure_date " << "source " << "target\n";

        for (unsigned int i = 0; i < transmission_target.size(); ++i)
            file_transmission <<
                transmission_date[i] << " " <<
                transmission_variant[i] << " " <<
                transmission_source_exposure_date[i] << " " <<
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
                        model->status_labels[from] << " " <<
                        model->status_labels[to] << " " <<
                        hist_transition_matrix[i * (ns * ns) + to * ns + from] << "\n";
                
        }
                
    }

}

template<typename TSeq>
inline void DataBase<TSeq>::record_transmission(
    int i,
    int j,
    int variant,
    int i_expo_date
) {

    transmission_date.push_back(model->today());
    transmission_source.push_back(i);
    transmission_target.push_back(j);
    transmission_variant.push_back(variant);
    transmission_source_exposure_date.push_back(i_expo_date);

}

template<typename TSeq>
inline size_t DataBase<TSeq>::get_n_variants() const
{
    return variant_id.size();
}

template<typename TSeq>
inline size_t DataBase<TSeq>::get_n_tools() const
{
    return tool_id.size();
}

template<typename TSeq>
inline void DataBase<TSeq>::reset()
{

    variant_id.clear();
    variant_name.clear();
    variant_sequence.clear();
    variant_origin_date.clear();
    variant_parent_id.clear();
    
    hist_variant_date.clear();
    hist_variant_id.clear();
    hist_variant_status.clear();
    hist_variant_counts.clear();

    tool_id.clear();
    tool_name.clear();
    tool_sequence.clear();
    tool_origin_date.clear();

    hist_tool_date.clear();
    hist_tool_id.clear();
    hist_tool_status.clear();
    hist_tool_counts.clear();
    
    hist_total_date.clear();
    hist_total_nvariants_active.clear();
    hist_total_status.clear();
    hist_total_counts.clear();
    
    transmission_date.clear();
    transmission_source.clear();
    transmission_target.clear();
    transmission_variant.clear();
    transmission_source_exposure_date.clear();

    transition_matrix.clear();
    hist_transition_matrix.clear();

    today_total_nvariants_active = 0;

    today_total.clear();
    
    today_variant.clear();

    today_tool.clear();

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

template<typename TSeq>
inline MapVec_type<int,int> DataBase<TSeq>::reproductive_number()
const {

    // Checking size
    MapVec_type<int,int> map;

    // Number of digits of maxid
    for (size_t i = 0u; i < transmission_date.size(); ++i)
    {
        // Fabricating id
        std::vector< int > h = {
            transmission_variant[i],
            transmission_source[i],
            transmission_source_exposure_date[i]
        };

        // Adding to counter
        if (map.find(h) == map.end())
            map[h] = 1;
        else
            map[h]++;
    }

    return map;

}

template<typename TSeq>
inline void DataBase<TSeq>::reproductive_number(
    std::string fn
) const {


    auto map = reproductive_number();

    std::ofstream fn_file(fn, std::ios_base::out);

    fn_file << "variant source exposure_id rt\n";

    for (auto & m : map)
        fn_file <<
            m.first[0u] << " " <<
            m.first[1u] << " " <<
            m.first[2u] << " " <<
            m.second << "\n";

    return;

}

template<typename TSeq>
inline std::vector< epiworld_double > DataBase<TSeq>::transition_probability(
    bool print
) const {

    auto status_labels = model->get_status();
    size_t n_status = status_labels.size();
    size_t n_days   = model->get_ndays();
    std::vector< epiworld_double > res(n_status * n_status, 0.0);
    std::vector< epiworld_double > days_to_include(n_status, 0.0);

    for (size_t t = 1; t < n_days; ++t)
    {

        for (size_t s_i = 0; s_i < n_status; ++s_i)
        {
            epiworld_double daily_total = hist_total_counts[(t - 1) * n_status + s_i];

            if (daily_total == 0)
                continue;

            days_to_include[s_i] += 1.0; 

            for (size_t s_j = 0u; s_j < n_status; ++s_j)
            {
                #ifdef EPI_DEBUG
                epiworld_double entry = hist_transition_matrix[
                    s_i + s_j * n_status +
                    t * (n_status * n_status)
                    ];

                if (entry > daily_total)
                    throw std::logic_error(
                        "The entry in hist_transition_matrix cannot have more elememnts than the total"
                        );

                res[s_i + s_j * n_status] += (entry / daily_total);
                #else
                    res[s_i + s_j * n_status] += (
                        hist_transition_matrix[
                            s_i + s_j * n_status +
                            t * (n_status * n_status)
                        ] / daily_total
                    );
                #endif
            }

        }

    }

    for (size_t s_i = 0; s_i < n_status; ++s_i)
    {
        for (size_t s_j = 0; s_j < n_status; ++s_j)
            res[s_i + s_j * n_status] /= days_to_include[s_i];
    }

    if (print)
    {   

        size_t nchar = 0u;
        for (auto & l : status_labels)
            if (l.length() > nchar)
                nchar = l.length();

        std::string fmt = " - %" + std::to_string(nchar) + "s";
        
        printf_epiworld("\nTransition Probabilities:\n");
        for (size_t s_i = 0u; s_i < n_status; ++s_i)
        {
            printf_epiworld(fmt.c_str(), status_labels[s_i].c_str());
            for (size_t s_j = 0u; s_j < n_status; ++s_j)
            {
                if (std::isnan(res[s_i + s_j * n_status]))
                {
                    printf_epiworld("   - ");
                } else {
                    printf_epiworld(" % 4.2f", res[s_i + s_j * n_status]);
                }
            }
            printf_epiworld("\n");
        }

        printf_epiworld("\n");

    }

    return res;


} 

#endif