#ifndef EPIWORLD_DATABASE_MEAT_HPP
#define EPIWORLD_DATABASE_MEAT_HPP

template<typename TSeq>
inline void DataBase<TSeq>::set_model(Model<TSeq> & m)
{
    model = &m;
    reset();
    today_total_nhealthy = model->get_persons()->size();

    // Initializing the counts
    today_total.resize(
        m.status_susceptible.size() +
        m.status_infected.size() +
        m.status_removed.size()
    );

    for (const auto & p : *m.get_persons())
        ++today_total[p.get_status()];

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
            // hist_variant_date.push_back(model->today());
            // hist_variant_id.push_back(p.second);
            // // hist_variant
            // hist_variant_ninfected.push_back(today_variant_ninfected[p.second]);
            // hist_variant_nrecovered.push_back(today_variant_nrecovered[p.second]);
            // hist_variant_nremoved.push_back(today_variant_nremoved[p.second]);

        }

        // Recording the overall history
        for (unsigned int s = 0u; s < model->nstatus; ++s)
        {
            hist_total_date.push_back(model->today());
            hist_total_nvariants_active.push_back(today_total_nvariants_active);
            hist_total_status.push_back(s);
            hist_total_counts.push_back(today_total[s]);

        }
        // hist_total_date.push_back(model->today());
        // hist_total_nvariants_active.push_back(today_total_nvariants_active);
        // hist_total_nhealthy.push_back(today_total_nhealthy);
        // hist_total_nrecovered.push_back(today_total_nrecovered);
        // hist_total_ninfected.push_back(today_total_ninfected);
        // hist_total_nremoved.push_back(today_total_nremoved);

    }
}

template<typename TSeq>
inline void DataBase<TSeq>::record_variant(Virus<TSeq> * v) {

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
            model->status_infected.size() +
            model->status_removed.size(),
            0
        );
        
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
        if (variant_id.size() > 1)
            today_variant_ninfected[old_id]--;
            
        today_variant_ninfected.push_back(1);
        today_variant_nrecovered.push_back(0);
        today_variant_nremoved.push_back(0);

        // Correcting math
        unsigned int tmp_status = v->get_host()->get_status();
        today_variant[old_id][tmp_status]--;
        today_variant[new_id][tmp_status]++;

    }
    
    return;
} 

template<typename TSeq>
inline int DataBase<TSeq>::get_today_total(std::string what) const
{
    if (what == "")
    { 
        printf_epiworld(
            "The following values are allowed in -what-: \"nvariants_active\", \"nhealthy\", \"nrecovered\", \"ninfected\", and \"nremoved\".");
        return 0;
    } else if (what == "nvariants_active")
        return today_total_nvariants_active;
    else if (what == "nhealthy")
        return today_total_nhealthy;
    else if (what == "nrecovered")
        return today_total_nrecovered;
    else if (what == "ninfected")
        return today_total_ninfected;
    else if (what == "nremoved")
        return today_total_nremoved;
    else 
        throw std::logic_error("\""+ what + "\" is not included in the database.");   

}

template<typename TSeq>
inline const std::vector<int> & DataBase<TSeq>::get_today_variant(std::string what) const
{
    if (what == "")
    {

        printf_epiworld(
        "The following values are allowed in -what-: \"nrecovered\", \"ninfected\", and \"nremoved\"."
        );
        
        return today_variant_nrecovered;

    } else if (what == "nrecovered")
        return today_variant_nrecovered;
    else if (what == "ninfected")
        return today_variant_ninfected;
    else if (what == "nremoved")
        return today_variant_nremoved;
    else 
        throw std::logic_error("\""+ what + "\" is not included in the database.");   

}

template<typename TSeq>
inline const std::vector<int> & DataBase<TSeq>::get_hist_total(std::string what) const
{
    if (what == "")
    {

        printf_epiworld(
        "The following values are allowed in -what-: \"nvariants_active\", \"nhealthy\", \"nrecovered\", \"ninfected\", and \"nremoved\"."
        );
        
        return hist_total_date;

    } else if (what == "date")
        return hist_total_date;
    else if (what == "nvariants_active")
        return hist_total_nvariants_active;
    else if (what == "nhealthty")
        return hist_total_nhealthy;
    else if (what == "nrecovered")
        return hist_total_nrecovered;
    else if (what == "ninfected")
        return hist_total_ninfected;
    else if (what == "nremoved")
        return hist_total_nremoved;
    else 
        throw std::logic_error("\""+ what + "\" is not included in the database.");   

}

template<typename TSeq>
inline const std::vector<int> & DataBase<TSeq>::get_hist_variant(std::string what) const
{
    
    if (what == "")
    {

         printf_epiworld(
            "The following values are allowed in -what-: \"date\", \"id\", \"nrecovered\", \"ninfected\", and \"nremoved\"."
        );
        
        return hist_variant_date;

    } else if (what == "date")
        return hist_variant_date;
    else if (what == "id")
        return hist_variant_id;
    else if (what == "nrecovered")
        return hist_variant_nrecovered;
    else if (what == "ninfected")
        return hist_variant_ninfected;
    else if (what == "nremoved")
        return hist_variant_nremoved;
    else 
        throw std::logic_error("\""+ what + "\" is not included in the database.");   

}

template<typename TSeq>
inline size_t DataBase<TSeq>::size() const
{
    return variant_id.size();
}

template<typename TSeq>
inline void DataBase<TSeq>::up_infected(
    Virus<TSeq> * v,
    unsigned int prev_status,
    unsigned int new_status
) {

    today_total[prev_status]--;
    today_total[new_status]++;

    today_variant[v->get_id()][new_status]++;

}

template<typename TSeq>
inline void DataBase<TSeq>::down_infected(
    Virus<TSeq> * v,
    unsigned int prev_status,
    unsigned int new_status
) {

    today_total[prev_status]--;
    today_total[new_status]++;

    today_variant[v->get_id()][prev_status]--;


}

template<typename TSeq>
inline void DataBase<TSeq>::write_data(
    std::string fn_variant_info,
    std::string fn_variant_hist,
    std::string fn_total_hist,
    std::string fn_transmision
) const {

    EPIWORLD_CLOCK_START("(02) Writing data")

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

    if (fn_variant_hist != "")
    {
        std::ofstream file_variant(fn_variant_hist, std::ios_base::out);
        
        file_variant <<
            "date " << "id " << "status " << "n\n";

        for (unsigned int i = 0; i < hist_variant_id.size(); ++i)
            file_variant <<
                hist_variant_date[i] << " " <<
                hist_variant_id[i] << " " <<
                hist_variant_status[i] << " " <<
                hist_variant_counts[i] << "\n";
    }

    if (fn_total_hist != "")
    {
        std::ofstream file_total(fn_total_hist, std::ios_base::out);

        file_total <<
            "date " << "nvariants " << "status" << "counts\n";

        for (unsigned int i = 0; i < hist_total_nhealthy.size(); ++i)
            file_total <<
                hist_total_date[i] << " " <<
                hist_total_nvariants_active[i] << " " <<
                hist_total_status[i] << " " << 
                hist_total_counts[i] << "\n";
    }

    if (fn_transmision != "")
    {
        std::ofstream file_transmision(fn_transmision, std::ios_base::out);
        file_transmision <<
            "date " << "variant " << "source " << "target\n";

        for (unsigned int i = 0; i < transmision_target.size(); ++i)
            file_transmision <<
                transmision_date[i] << " " <<
                transmision_variant[i] << " " <<
                transmision_source[i] << " " <<
                transmision_target[i] << "\n";
                
    }
    

    EPIWORLD_CLOCK_END("(02) Writing data")

}

template<typename TSeq>
inline void DataBase<TSeq>::record_transmision(
    int i,
    int j,
    int variant
) {
    transmision_date.push_back(model->today());
    transmision_source.push_back(i);
    transmision_target.push_back(j);
    transmision_variant.push_back(variant);
}

template<typename TSeq>
inline size_t DataBase<TSeq>::get_nvariants() const {
    return variant_id.size();
}

template<typename TSeq>
inline void DataBase<TSeq>::reset() {
    variant_id.clear();
    sequence.clear();
    origin_date.clear();
    parent_id.clear();
    today_variant_ninfected.clear();
    today_variant_nrecovered.clear();
    today_variant_nremoved.clear();
    hist_variant_date.clear();
    hist_variant_id.clear();
    hist_variant_ninfected.clear();
    hist_variant_nrecovered.clear();
    hist_variant_nremoved.clear();
    hist_total_date.clear();
    hist_total_nvariants_active.clear();
    hist_total_nhealthy.clear();
    hist_total_nrecovered.clear();
    hist_total_ninfected.clear();
    hist_total_nremoved.clear();
    transmision_date.clear();
    transmision_source.clear();
    transmision_target.clear();
    transmision_variant.clear();

    today_total_nvariants_active = 0;
    today_total_nhealthy = 0;
    today_total_nrecovered = 0;
    today_total_ninfected = 0;
    today_total_nremoved = 0;

}

#endif