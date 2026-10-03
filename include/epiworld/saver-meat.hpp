#ifndef EPIWORLD_SAVER_MEAT_HPP
#define EPIWORLD_SAVER_MEAT_HPP

template<typename TTable>
template<typename TFun>
inline void SaverTable<TTable>::for_each_column(TFun && fun)
{
    std::apply(
        [&fun](const auto & ... column) { (fun(column), ...); },
        TTable::columns()
    );
}

template<typename TTable>
inline size_t SaverTable<TTable>::size() const
{
    const auto & self = static_cast< const TTable & >(*this);
    return (self.*(std::get<0u>(TTable::columns()).values)).size();
}

template<typename TTable>
inline void SaverTable<TTable>::set_sim_id(int id)
{
    sim_id.assign(size(), id);
}

template<typename TTable>
inline void SaverTable<TTable>::append(const TTable & other)
{

    if (other.sim_id.size() != other.size())
        throw std::logic_error(
            "SaverTable::append: the rows to append have no simulation ID."
        );

    sim_id.insert(sim_id.end(), other.sim_id.begin(), other.sim_id.end());

    auto & self = static_cast< TTable & >(*this);
    for_each_column([&](const auto & column) {
        auto & values = self.*(column.values);
        const auto & other_values = other.*(column.values);
        values.insert(values.end(), other_values.begin(), other_values.end());
    });

}

template<typename TTable>
inline void SaverTable<TTable>::write(std::ostream & out) const
{

    const auto & self = static_cast< const TTable & >(*this);

    #ifdef EPI_DEBUG
    out << "thread ";
    #endif
    const char * sep = "";
    for_each_column([&](const auto & column) {
        out << sep << column.name;
        sep = " ";
    });
    out << "\n";

    for (size_t i = 0u; i < size(); ++i)
    {

        #ifdef EPI_DEBUG
        out << EPI_GET_THREAD_ID() << " ";
        #endif
        sep = "";
        for_each_column([&](const auto & column) {
            const char * quote = column.quoted ? "\"" : "";
            out << sep << quote << (self.*(column.values))[i] << quote;
            sep = " ";
        });
        out << "\n";

    }

}

template<typename TTable>
inline bool SaverTable<TTable>::operator==(
    const SaverTable<TTable> & other
) const
{

    if (sim_id != other.sim_id)
        return false;

    const auto & self = static_cast< const TTable & >(*this);
    const auto & other_table = static_cast< const TTable & >(other);
    bool equal = true;
    for_each_column([&](const auto & column) {
        equal = equal && (self.*(column.values) == other_table.*(column.values));
    });

    return equal;

}

template<typename TFun>
inline void RunOutputs::for_each_table(TFun && fun)
{
    fun("total_hist", &SaveOptions::total_hist, &RunOutputs::total_hist);
    fun("virus_info", &SaveOptions::virus_info, &RunOutputs::virus_info);
    fun("virus_hist", &SaveOptions::virus_hist, &RunOutputs::virus_hist);
    fun("tool_info", &SaveOptions::tool_info, &RunOutputs::tool_info);
    fun("tool_hist", &SaveOptions::tool_hist, &RunOutputs::tool_hist);
    fun("transmission", &SaveOptions::transmission, &RunOutputs::transmission);
    fun("transition", &SaveOptions::transition, &RunOutputs::transition);
    fun("reproductive", &SaveOptions::reproductive, &RunOutputs::reproductive);
    fun("generation", &SaveOptions::generation, &RunOutputs::generation);
    fun("active_cases", &SaveOptions::active_cases, &RunOutputs::active_cases);
    fun("outbreak_size", &SaveOptions::outbreak_size, &RunOutputs::outbreak_size);
    fun("hospitalizations", &SaveOptions::hospitalizations, &RunOutputs::hospitalizations);
}

inline void RunOutputs::set_sim_id(int id)
{
    for_each_table([&](const char *, auto, auto table) {
        (this->*table).set_sim_id(id);
    });
}

inline void RunOutputs::append(const RunOutputs & other)
{
    for_each_table([&](const char *, auto, auto table) {
        (this->*table).append(other.*table);
    });
}

inline bool RunOutputs::operator==(const RunOutputs & other) const
{
    bool equal = true;
    for_each_table([&](const char *, auto, auto table) {
        equal = equal && (this->*table == other.*table);
    });
    return equal;
}

template<typename TTable>
inline void saver_write_file(const std::string & fn, const TTable & table)
{

    std::ofstream file(fn, std::ios_base::out);
    if (!file)
        throw std::runtime_error(
            "Could not open file \"" + fn + "\" for writing."
        );

    table.write(file);
    if (!file)
        throw std::runtime_error("Could not write file \"" + fn + "\".");

}

template<typename TSeq>
inline RunOutputs Saver<TSeq>::extract(
    size_t sim_id,
    const Model<TSeq> & model
) const
{
    RunOutputs outputs = model.get_db().get_run_outputs(options);
    outputs.set_sim_id(static_cast<int>(sim_id));
    return outputs;
}

template<typename TSeq>
inline void SaverMemory<TSeq>::begin(size_t nexperiments)
{
    runs.clear();
    runs.resize(nexperiments);
}

template<typename TSeq>
inline void SaverMemory<TSeq>::write(size_t sim_id, RunOutputs && outputs)
{

    if (sim_id >= runs.size())
        throw std::out_of_range(
            "SaverMemory::write: simulation " + std::to_string(sim_id) +
            " is out of range. Call begin() with the number of simulations "
            "first."
        );

    outputs.set_sim_id(static_cast<int>(sim_id));
    runs[sim_id] = std::move(outputs);

}

template<typename TSeq>
inline RunOutputs SaverMemory<TSeq>::results() const
{
    RunOutputs out;
    for (const auto & run : runs)
        out.append(run);
    return out;
}

template<typename TSeq>
inline RunOutputs SaverMemory<TSeq>::take_results()
{

    RunOutputs out;
    for (auto & run : runs)
    {
        out.append(run);
        run = RunOutputs();
    }

    runs.clear();
    return out;

}

template<typename TSeq>
inline SaverCallback<TSeq>::SaverCallback(
    SaveOptions options_,
    std::function<void(size_t, RunOutputs &&)> callback_
) : Saver<TSeq>(options_), callback(std::move(callback_))
{
    if (!callback)
        throw std::invalid_argument("SaverCallback requires a callback.");
}

template<typename TSeq>
inline void SaverCallback<TSeq>::write(size_t sim_id, RunOutputs && outputs)
{
    std::lock_guard< std::mutex > lock(callback_mutex);
    callback(sim_id, std::move(outputs));
}

/**
 * @brief Formats a simulation ID with a `printf` format (see `SaverFiles`)
 */
template<typename TValue>
inline std::string saver_format_id(const std::string & format, TValue id)
{

    const int length = snprintf(nullptr, 0, format.c_str(), id);
    if (length < 0)
        throw std::runtime_error("SaverFiles: could not format the file name.");

    std::vector< char > buffer(static_cast<size_t>(length) + 1u);
    snprintf(buffer.data(), buffer.size(), format.c_str(), id);
    return std::string(buffer.data());

}

template<typename TSeq>
inline SaverFiles<TSeq>::SaverFiles(
    std::string format_,
    SaveOptions options_
) : Saver<TSeq>(options_)
{

    // Exactly one integer conversion. Rewriting its length to `ll` lets every
    // write format the ID as a `long long`, whatever the original length.
    std::smatch match;
    const std::regex placeholder(
        "([^%]*%[-+ 0]*[0-9]*(?:\\.[0-9]+)?)(?:ll|l|z)?([udi])([^%]*)"
    );

    if (!std::regex_match(format_, match, placeholder))
        throw std::invalid_argument(
            "SaverFiles: the format \"" + format_ + "\" must contain exactly "
            "one integer placeholder, like \"%03lu\"."
        );

    format = match.str(1u) + "ll" + match.str(2u) + match.str(3u);
    format_signed = match.str(2u) != "u";

}

template<typename TSeq>
inline void SaverFiles<TSeq>::write(size_t sim_id, RunOutputs && outputs)
{

    const std::string prefix = format_signed ?
        saver_format_id(format, static_cast<long long>(sim_id)) :
        saver_format_id(format, static_cast<unsigned long long>(sim_id));

    RunOutputs::for_each_table([&](const char * name, auto option, auto table) {
        if (this->options.*option)
            saver_write_file(prefix + "_" + name + ".csv", outputs.*table);
    });

}

#endif
