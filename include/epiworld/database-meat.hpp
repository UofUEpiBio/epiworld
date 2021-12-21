#ifndef EPIWORLD_DATABASE_MEAT_HPP
#define EPIWORLD_DATABASE_MEAT_HPP

template<typename TSeq>
inline void DataBase<TSeq>::set_model(Model<TSeq> & m)
{
    model = &m;
    today_total_nhealthy = model->get_persons()->size();
    return;
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
            hist_variant_date.push_back(model->today());
            hist_variant_id.push_back(p.second);
            hist_variant_ninfected.push_back(today_variant_ninfected[p.second]);
            hist_variant_nrecovered.push_back(today_variant_nrecovered[p.second]);
            hist_variant_ndeceased.push_back(today_variant_ndeceased[p.second]);

        }

        // Recording the overall history
        hist_total_date.push_back(model->today());
        hist_total_nvariants_active.push_back(today_total_nvariants_active);
        hist_total_nhealthy.push_back(today_total_nhealthy);
        hist_total_nrecovered.push_back(today_total_nrecovered);
        hist_total_ninfected.push_back(today_total_ninfected);
        hist_total_ndeceased.push_back(today_total_ndeceased);

    }
}

template<typename TSeq>
inline void DataBase<TSeq>::record_variant(Virus<TSeq> * v) {

    // Updating registry
    std::vector< int > hash = seq_hasher(*v->get_sequence());
    if (variant_id.find(hash) == variant_id.end())
    {
        variant_id[hash] = variant_id.size();
        sequence.push_back(*v->get_sequence());
        origin_date.push_back(model->today());

        // Collecting old ID to make the accounting
        int tmp_id = v->get_id();

        parent_id.push_back(tmp_id);
        
        // Moving statistics
        if (variant_id.size() > 1)
            today_variant_ninfected[tmp_id]--;
            
        today_variant_ninfected.push_back(1);
        today_variant_nrecovered.push_back(0);
        today_variant_ndeceased.push_back(0);
        
        // Updating the variant
        v->set_id(variant_id.size() - 1);
        v->set_date(model->today());

        today_total_nvariants_active++;

    } else {

        int new_id = variant_id[hash];

        // Accounting (from the old to the new)
        today_variant_ninfected[v->get_id()]--;
        today_variant_ninfected[new_id]++;

        // Reflecting the change
        v->set_id(new_id);
        v->set_date(origin_date[new_id]);

    }    

    // today_total_nhealthy--;
    // today_total_ninfected++;

    
    return;
} 

template<typename TSeq>
inline int DataBase<TSeq>::get_today_total(std::string what) const
{

    if (what == "nvariants_active")
        return today_total_nvariants_active;
    else if (what == "nhealthty")
        return today_total_nhealthy;
    else if (what == "nrecovered")
        return today_total_nrecovered;
    else if (what == "ninfected")
        return today_total_ninfected;
    else if (what == "ndeceased")
        return today_total_ndeceased;
    else 
        throw std::logic_error("\""+ what + "\" is not included in the database.");   

}

template<typename TSeq>
inline const std::vector<int> & DataBase<TSeq>::get_today_variant(std::string what) const
{
    
    if (what == "nrecovered")
        return today_variant_nrecovered;
    else if (what == "ninfected")
        return today_variant_ninfected;
    else if (what == "ndeceased")
        return today_variant_ndeceased;
    else 
        throw std::logic_error("\""+ what + "\" is not included in the database.");   

}

template<typename TSeq>
inline const std::vector<int> & DataBase<TSeq>::get_hist_total(std::string what) const
{
    if (what == "date")
        return hist_total_date;
    else if (what == "nvariants_active")
        return hist_total_nvariants_active;
    else if (what == "nhealthty")
        return hist_total_nhealthy;
    else if (what == "nrecovered")
        return hist_total_nrecovered;
    else if (what == "ninfected")
        return hist_total_ninfected;
    else if (what == "ndeceased")
        return hist_total_ndeceased;
    else 
        throw std::logic_error("\""+ what + "\" is not included in the database.");   

}

template<typename TSeq>
inline const std::vector<int> & DataBase<TSeq>::get_hist_variant(std::string what) const
{
    
    if (what == "date")
        return hist_variant_date;
    else if (what == "id")
        return hist_variant_id;
    else if (what == "nrecovered")
        return hist_variant_nrecovered;
    else if (what == "ninfected")
        return hist_variant_ninfected;
    else if (what == "ndeceased")
        return hist_variant_ndeceased;
    else 
        throw std::logic_error("\""+ what + "\" is not included in the database.");   

}

template<typename TSeq>
inline size_t DataBase<TSeq>::size() const
{
    return variant_id.size();
}

template<typename TSeq>
inline void DataBase<TSeq>::up_infected(Virus<TSeq> * v) {

    today_variant_ninfected[v->get_id()]++;
    today_total_nhealthy--;
    today_total_ninfected++;

}

template<typename TSeq>
inline void DataBase<TSeq>::up_recovered(Virus<TSeq> * v) {

    int tmp_id = v->get_id();
    today_variant_ninfected[tmp_id]--;
    today_variant_nrecovered[tmp_id]++;
    today_total_ninfected--;
    today_total_nhealthy++;
    today_total_nrecovered++;

}

template<typename TSeq>
inline void DataBase<TSeq>::up_deceased(Virus<TSeq> * v) {

    int tmp_id = v->get_id();
    today_variant_ninfected[tmp_id]--;
    today_variant_ndeceased[tmp_id]++;
    
    today_total_ninfected--;
    today_total_ndeceased++;

}

template<typename TSeq>
inline void DataBase<TSeq>::write_data(
    std::string fn_variant_info,
    std::string fn_variant_hist,
    std::string fn_total_hist,
    std::string fn_transmision
) const {

    EPIWORLD_CLOCK_START("Writing data")

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
            "date " << "id " << "ninfected " << "nrecovered " << "ndeceased\n";

        for (unsigned int i = 0; i < hist_variant_id.size(); ++i)
            file_variant <<
                hist_variant_date[i] << " " <<
                hist_variant_id[i] << " " <<
                hist_variant_ninfected[i] << " " <<
                hist_variant_nrecovered[i] << " " <<
                hist_variant_ndeceased[i] << "\n";
    }

    if (fn_total_hist != "")
    {
        std::ofstream file_total(fn_total_hist, std::ios_base::out);

        file_total <<
            "date " << "nvariants " << "nhealthy " << "ninfected " << "nrecovered " << "ndeceased\n";

        for (unsigned int i = 0; i < hist_total_nhealthy.size(); ++i)
            file_total <<
                hist_total_date[i] << " " <<
                hist_total_nvariants_active[i] << " " <<
                hist_total_nhealthy[i] << " " <<
                hist_total_ninfected[i] << " " <<
                hist_total_nrecovered[i] << " " <<
                hist_total_ndeceased[i] << "\n";
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
    

    EPIWORLD_CLOCK_END("Writing data")

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

#endif