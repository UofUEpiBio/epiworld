#ifndef EPIWORLD_SAVER_MEAT_HPP
#define EPIWORLD_SAVER_MEAT_HPP

template<typename TSeq>
inline RunOutputs DataBase<TSeq>::get_run_outputs(const SaveOptions& o) const {
    RunOutputs out;
    if (o.total_hist) {
        auto& t = out.total_hist;
        get_hist_total(&t.date, &t.state, &t.counts);
        t.nviruses.assign(hist_total_nviruses_active.begin(), hist_total_nviruses_active.end());
    }
    if (o.virus_info) {
        auto& t = out.virus_info;
        for (const auto& v : virus_id) {
            int id = v.second;
            t.virus_id.push_back(id);
            t.virus.push_back(virus_name[id]);
            t.virus_sequence.push_back(seq_writer(virus_sequence[id]));
            t.date_recorded.push_back(virus_origin_date[id]);
            t.parent.push_back(virus_parent_id[id]);
        }
    }
    if (o.virus_hist) {
        auto& t = out.virus_hist;
        get_hist_virus(t.date, t.virus_id, t.state, t.n);
        for (int id : t.virus_id) t.virus.push_back(virus_name[id]);
    }
    if (o.tool_info) {
        auto& t = out.tool_info;
        for (const auto& tool : tool_id) {
            int id = tool.second;
            t.id.push_back(id);
            t.tool_name.push_back(tool_name[id]);
            t.tool_sequence.push_back(seq_writer(tool_sequence[id]));
            t.date_recorded.push_back(tool_origin_date[id]);
        }
    }
    if (o.tool_hist) {
        auto& t = out.tool_hist;
        get_hist_tool(t.date, t.id, t.state, t.n);
    }
    if (o.transmission) {
        auto& t = out.transmission;
        t.date = transmission_date;
        t.virus_id = transmission_virus;
        t.source_exposure_date = transmission_source_exposure_date;
        t.source = transmission_source;
        t.target = transmission_target;
        for (int id : t.virus_id) t.virus.push_back(virus_name[id]);
    }
    if (o.transition && !hist_transition_matrix.empty()) {
        // write_data uses from-major order, whereas the public matrix getter
        // uses to-major order. Preserve the file's ordering here.
        auto& t = out.transition;
        const int ns = model->get_states().size();
        for (int day = 0; day <= model->today(); ++day)
            for (int from = 0; from < ns; ++from)
                for (int to = 0; to < ns; ++to) {
                    int count = hist_transition_matrix[day * ns * ns + to * ns + from];
                    if (count == 0) continue;
                    t.date.push_back(day);
                    t.from.push_back(model->get_states()[from]);
                    t.to.push_back(model->get_states()[to]);
                    t.counts.push_back(count);
                }
    }
    if (o.reproductive) {
        auto& t = out.reproductive;
        for (const auto& entry : get_reproductive_number()) {
            t.virus_id.push_back(entry.first[0]);
            t.virus.push_back(virus_name[entry.first[0]]);
            t.source.push_back(entry.first[1]);
            t.source_exposure_date.push_back(entry.first[2]);
            t.rt.push_back(entry.second);
        }
    }
    if (o.generation) {
        auto& t = out.generation;
        get_generation_time(t.source, t.virus, t.source_exposure_date, t.gentime);
    }
    if (o.active_cases) {
        auto& t = out.active_cases;
        std::vector<int> dates, ids, counts;
        get_active_cases(dates, ids, counts);
        for (size_t i = 0; i < counts.size(); ++i) {
            if (counts[i] <= 0) continue;
            t.date.push_back(dates[i]);
            t.virus_id.push_back(ids[i]);
            t.virus.push_back(virus_name[ids[i]]);
            t.active_cases.push_back(counts[i]);
        }
    }
    if (o.outbreak_size) {
        auto& t = out.outbreak_size;
        std::vector<int> dates, ids, counts;
        get_outbreak_size(dates, ids, counts);
        for (size_t i = 0; i < counts.size(); ++i) {
            if (counts[i] <= 0) continue;
            t.date.push_back(dates[i]);
            t.virus_id.push_back(ids[i]);
            t.virus.push_back(virus_name[ids[i]]);
            t.outbreak_size.push_back(counts[i]);
        }
    }
    if (o.hospitalizations) {
        auto& t = out.hospitalizations;
        std::vector<int> dates, viruses, tools, counts;
        std::vector<double> weights;
        get_hospitalizations(dates, viruses, tools, counts, weights);
        for (size_t i = 0; i < counts.size(); ++i) {
            if (counts[i] <= 0 && weights[i] <= 0.0) continue;
            t.date.push_back(dates[i]);
            t.virus_id.push_back(viruses[i]);
            t.tool_id.push_back(tools[i]);
            t.count.push_back(counts[i]);
            t.weight.push_back(weights[i]);
        }
    }
    return out;
}

template<typename TSeq>
inline RunOutputs Saver<TSeq>::extract(size_t id, const Model<TSeq>& model) const {
    auto out = model.get_db().get_run_outputs(options);
    out.total_hist.sim_id.assign(out.total_hist.size(), id);
    out.virus_info.sim_id.assign(out.virus_info.size(), id);
    out.virus_hist.sim_id.assign(out.virus_hist.size(), id);
    out.tool_info.sim_id.assign(out.tool_info.size(), id);
    out.tool_hist.sim_id.assign(out.tool_hist.size(), id);
    out.transmission.sim_id.assign(out.transmission.size(), id);
    out.transition.sim_id.assign(out.transition.size(), id);
    out.reproductive.sim_id.assign(out.reproductive.size(), id);
    out.generation.sim_id.assign(out.generation.size(), id);
    out.active_cases.sim_id.assign(out.active_cases.size(), id);
    out.outbreak_size.sim_id.assign(out.outbreak_size.size(), id);
    out.hospitalizations.sim_id.assign(out.hospitalizations.size(), id);
    return out;
}

template<typename TSeq>
inline SaverFiles<TSeq>::SaverFiles(std::string fmt, SaveOptions opts) :
    Saver<TSeq>(opts), format(std::move(fmt)) {
    // Only accept the integer placeholder used for simulation IDs. Validating
    // before snprintf prevents undefined behaviour from incompatible formats.
    if (!std::regex_match(format, std::regex("[^%]*%[-+ #0]*[0-9]*(\\.[0-9]+)?(ll|l|z)?[udi][^%]*")))
        throw std::invalid_argument("SaverFiles format must contain one integer placeholder.");
}

template<typename TSeq>
inline void SaverFiles<TSeq>::write(size_t id, RunOutputs&& out) {
    const size_t conversion = format.find_first_of("udi", format.find('%'));
    const bool is_unsigned = format[conversion] == 'u';
    const auto format_id = [&](auto value) {
        using Value = decltype(value);
        if (id > static_cast<size_t>(std::numeric_limits<Value>::max()))
            throw std::overflow_error("Simulation ID exceeds the filename placeholder range.");
        const int length = snprintf(nullptr, 0, format.c_str(), value);
        if (length < 0) throw std::runtime_error("Could not format saver filename.");
        std::vector<char> buffer(static_cast<size_t>(length) + 1);
        snprintf(buffer.data(), buffer.size(), format.c_str(), value);
        return std::string(buffer.data());
    };
    std::string prefix;
    const char length = format[conversion - 1];
    if (length == 'z') {
        prefix = is_unsigned ? format_id(id) :
            format_id(static_cast<std::make_signed_t<size_t>>(id));
    } else if (length == 'l' && conversion >= 2 && format[conversion - 2] == 'l') {
        prefix = is_unsigned ? format_id(static_cast<unsigned long long>(id)) :
            format_id(static_cast<long long>(id));
    } else if (length == 'l') {
        prefix = is_unsigned ? format_id(static_cast<unsigned long>(id)) :
            format_id(static_cast<long>(id));
    } else {
        prefix = is_unsigned ? format_id(static_cast<unsigned int>(id)) :
            format_id(static_cast<int>(id));
    }
    if (this->options.total_hist) {
        std::ofstream file(prefix + "_total_hist.csv");
        if (!file) throw std::runtime_error("Could not open file " + prefix + "_total_hist.csv");
        out.total_hist.write(file);
        if (!file) throw std::runtime_error("Could not write file " + prefix + "_total_hist.csv");
    }
    if (this->options.virus_info) {
        std::ofstream file(prefix + "_virus_info.csv");
        if (!file) throw std::runtime_error("Could not open file " + prefix + "_virus_info.csv");
        out.virus_info.write(file);
        if (!file) throw std::runtime_error("Could not write file " + prefix + "_virus_info.csv");
    }
    if (this->options.virus_hist) {
        std::ofstream file(prefix + "_virus_hist.csv");
        if (!file) throw std::runtime_error("Could not open file " + prefix + "_virus_hist.csv");
        out.virus_hist.write(file);
        if (!file) throw std::runtime_error("Could not write file " + prefix + "_virus_hist.csv");
    }
    if (this->options.tool_info) {
        std::ofstream file(prefix + "_tool_info.csv");
        if (!file) throw std::runtime_error("Could not open file " + prefix + "_tool_info.csv");
        out.tool_info.write(file);
        if (!file) throw std::runtime_error("Could not write file " + prefix + "_tool_info.csv");
    }
    if (this->options.tool_hist) {
        std::ofstream file(prefix + "_tool_hist.csv");
        if (!file) throw std::runtime_error("Could not open file " + prefix + "_tool_hist.csv");
        out.tool_hist.write(file);
        if (!file) throw std::runtime_error("Could not write file " + prefix + "_tool_hist.csv");
    }
    if (this->options.transmission) {
        std::ofstream file(prefix + "_transmission.csv");
        if (!file) throw std::runtime_error("Could not open file " + prefix + "_transmission.csv");
        out.transmission.write(file);
        if (!file) throw std::runtime_error("Could not write file " + prefix + "_transmission.csv");
    }
    if (this->options.transition) {
        std::ofstream file(prefix + "_transition.csv");
        if (!file) throw std::runtime_error("Could not open file " + prefix + "_transition.csv");
        out.transition.write(file);
        if (!file) throw std::runtime_error("Could not write file " + prefix + "_transition.csv");
    }
    if (this->options.reproductive) {
        std::ofstream file(prefix + "_reproductive.csv");
        if (!file) throw std::runtime_error("Could not open file " + prefix + "_reproductive.csv");
        out.reproductive.write(file);
        if (!file) throw std::runtime_error("Could not write file " + prefix + "_reproductive.csv");
    }
    if (this->options.generation) {
        std::ofstream file(prefix + "_generation.csv");
        if (!file) throw std::runtime_error("Could not open file " + prefix + "_generation.csv");
        out.generation.write(file);
        if (!file) throw std::runtime_error("Could not write file " + prefix + "_generation.csv");
    }
    if (this->options.active_cases) {
        std::ofstream file(prefix + "_active_cases.csv");
        if (!file) throw std::runtime_error("Could not open file " + prefix + "_active_cases.csv");
        out.active_cases.write(file);
        if (!file) throw std::runtime_error("Could not write file " + prefix + "_active_cases.csv");
    }
    if (this->options.outbreak_size) {
        std::ofstream file(prefix + "_outbreak_size.csv");
        if (!file) throw std::runtime_error("Could not open file " + prefix + "_outbreak_size.csv");
        out.outbreak_size.write(file);
        if (!file) throw std::runtime_error("Could not write file " + prefix + "_outbreak_size.csv");
    }
    if (this->options.hospitalizations) {
        std::ofstream file(prefix + "_hospitalizations.csv");
        if (!file) throw std::runtime_error("Could not open file " + prefix + "_hospitalizations.csv");
        out.hospitalizations.write(file);
        if (!file) throw std::runtime_error("Could not write file " + prefix + "_hospitalizations.csv");
    }
}

#endif
