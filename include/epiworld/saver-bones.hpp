#ifndef EPIWORLD_SAVER_BONES_HPP
#define EPIWORLD_SAVER_BONES_HPP

template<typename TSeq>
class Model;

/**
 * @brief Outputs to collect from each simulation
 *
 * @details The fields follow the order of the boolean arguments of
 * `make_save_run()`. Only the total history is collected by default.
 */
struct SaveOptions
{
    bool total_hist = true;
    bool virus_info = false;
    bool virus_hist = false;
    bool tool_info = false;
    bool tool_hist = false;
    bool transmission = false;
    bool transition = false;
    bool reproductive = false;
    bool generation = false;
    bool active_cases = false;
    bool outbreak_size = false;
    bool hospitalizations = false;
};

/**
 * @brief A column of an output table
 *
 * @details `name` is the column's name in the file header, and `quoted`
 * whether its values are written between double quotes.
 */
template<typename TTable, typename TValue>
struct SaverColumn
{
    const char * name;
    std::vector< TValue > TTable::* values;
    bool quoted;
};

/**
 * @brief Describes a column of an output table
 *
 * @details Text columns are quoted by default, as in `DataBase::write_data()`.
 */
template<typename TTable, typename TValue>
inline SaverColumn<TTable,TValue> saver_column(
    const char * name,
    std::vector< TValue > TTable::* values,
    bool quoted = std::is_same< TValue, std::string >::value
)
{
    return {name, values, quoted};
}

/**
 * @brief Columnar output table of one or more simulations
 *
 * @details Tables derive from this class and list their columns, in file
 * order, in a static `columns()` function. That list is the only place where
 * a table's layout is defined: the functions below build on it. `sim_id` is
 * not a file column; it identifies the simulation of each row.
 *
 * @tparam TTable The derived table.
 */
template<typename TTable>
class SaverTable
{
public:

    std::vector< int > sim_id; ///< Simulation ID of each row.

    size_t size() const; ///< Number of rows.
    void set_sim_id(int id); ///< Sets the simulation ID of all rows.
    void append(const TTable & other); ///< Appends the rows of `other`.

    /**
     * @brief Writes the table in the format of `DataBase::write_data()`
     * @details Space-separated, with a header. Debug builds (`EPI_DEBUG`)
     * add the calling thread's ID as the first column.
     */
    void write(std::ostream & out) const;

    bool operator==(const SaverTable<TTable> & other) const;

private:
    template<typename TFun>
    static void for_each_column(TFun && fun);

};

/**
 * @brief Outputs of one or more simulations, one table per output
 *
 * @details Tables have the same columns and rows as the matching files from
 * `DataBase::write_data()`. Tables not selected in `SaveOptions` are empty.
 */
struct RunOutputs
{

    struct TotalHist : SaverTable<TotalHist>
    {
        std::vector< int > date;
        std::vector< int > nviruses;
        std::vector< std::string > state;
        std::vector< int > counts;
        static auto columns()
        {
            return std::make_tuple(
                saver_column("date", &TotalHist::date),
                saver_column("nviruses", &TotalHist::nviruses),
                saver_column("state", &TotalHist::state),
                saver_column("counts", &TotalHist::counts)
            );
        }
    };

    struct VirusInfo : SaverTable<VirusInfo>
    {
        std::vector< int > virus_id;
        std::vector< std::string > virus;
        std::vector< std::string > virus_sequence;
        std::vector< int > date_recorded;
        std::vector< int > parent;
        static auto columns()
        {
            return std::make_tuple(
                saver_column("virus_id", &VirusInfo::virus_id),
                saver_column("virus", &VirusInfo::virus),
                saver_column("virus_sequence", &VirusInfo::virus_sequence, false),
                saver_column("date_recorded", &VirusInfo::date_recorded),
                saver_column("parent", &VirusInfo::parent)
            );
        }
    };

    struct VirusHist : SaverTable<VirusHist>
    {
        std::vector< int > date;
        std::vector< int > virus_id;
        std::vector< std::string > virus;
        std::vector< std::string > state;
        std::vector< int > n;
        static auto columns()
        {
            return std::make_tuple(
                saver_column("date", &VirusHist::date),
                saver_column("virus_id", &VirusHist::virus_id),
                saver_column("virus", &VirusHist::virus),
                saver_column("state", &VirusHist::state),
                saver_column("n", &VirusHist::n)
            );
        }
    };

    struct ToolInfo : SaverTable<ToolInfo>
    {
        std::vector< int > id;
        std::vector< std::string > tool_name;
        std::vector< std::string > tool_sequence;
        std::vector< int > date_recorded;
        static auto columns()
        {
            return std::make_tuple(
                saver_column("id", &ToolInfo::id),
                saver_column("tool_name", &ToolInfo::tool_name),
                saver_column("tool_sequence", &ToolInfo::tool_sequence, false),
                saver_column("date_recorded", &ToolInfo::date_recorded)
            );
        }
    };

    struct ToolHist : SaverTable<ToolHist>
    {
        std::vector< int > date;
        std::vector< int > id;
        std::vector< std::string > state;
        std::vector< int > n;
        static auto columns()
        {
            return std::make_tuple(
                saver_column("date", &ToolHist::date),
                saver_column("id", &ToolHist::id),
                saver_column("state", &ToolHist::state),
                saver_column("n", &ToolHist::n)
            );
        }
    };

    struct Transmission : SaverTable<Transmission>
    {
        std::vector< int > date;
        std::vector< int > virus_id;
        std::vector< std::string > virus;
        std::vector< int > source_exposure_date;
        std::vector< int > source;
        std::vector< int > target;
        static auto columns()
        {
            return std::make_tuple(
                saver_column("date", &Transmission::date),
                saver_column("virus_id", &Transmission::virus_id),
                saver_column("virus", &Transmission::virus),
                saver_column("source_exposure_date", &Transmission::source_exposure_date),
                saver_column("source", &Transmission::source),
                saver_column("target", &Transmission::target)
            );
        }
    };

    struct Transition : SaverTable<Transition>
    {
        std::vector< int > date;
        std::vector< std::string > from;
        std::vector< std::string > to;
        std::vector< int > counts;
        static auto columns()
        {
            return std::make_tuple(
                saver_column("date", &Transition::date),
                saver_column("from", &Transition::from),
                saver_column("to", &Transition::to),
                saver_column("counts", &Transition::counts)
            );
        }
    };

    struct Reproductive : SaverTable<Reproductive>
    {
        std::vector< int > virus_id;
        std::vector< std::string > virus;
        std::vector< int > source;
        std::vector< int > source_exposure_date;
        std::vector< int > rt;
        static auto columns()
        {
            return std::make_tuple(
                saver_column("virus_id", &Reproductive::virus_id),
                saver_column("virus", &Reproductive::virus),
                saver_column("source", &Reproductive::source),
                saver_column("source_exposure_date", &Reproductive::source_exposure_date),
                saver_column("rt", &Reproductive::rt)
            );
        }
    };

    struct Generation : SaverTable<Generation>
    {
        std::vector< int > virus;
        std::vector< int > source;
        std::vector< int > source_exposure_date;
        std::vector< int > gentime;
        static auto columns()
        {
            return std::make_tuple(
                saver_column("virus", &Generation::virus),
                saver_column("source", &Generation::source),
                saver_column("source_exposure_date", &Generation::source_exposure_date),
                saver_column("gentime", &Generation::gentime)
            );
        }
    };

    struct ActiveCases : SaverTable<ActiveCases>
    {
        std::vector< int > date;
        std::vector< int > virus_id;
        std::vector< std::string > virus;
        std::vector< int > active_cases;
        static auto columns()
        {
            return std::make_tuple(
                saver_column("date", &ActiveCases::date),
                saver_column("virus_id", &ActiveCases::virus_id),
                saver_column("virus", &ActiveCases::virus),
                saver_column("active_cases", &ActiveCases::active_cases)
            );
        }
    };

    struct OutbreakSize : SaverTable<OutbreakSize>
    {
        std::vector< int > date;
        std::vector< int > virus_id;
        std::vector< std::string > virus;
        std::vector< int > outbreak_size;
        static auto columns()
        {
            return std::make_tuple(
                saver_column("date", &OutbreakSize::date),
                saver_column("virus_id", &OutbreakSize::virus_id),
                saver_column("virus", &OutbreakSize::virus),
                saver_column("outbreak_size", &OutbreakSize::outbreak_size)
            );
        }
    };

    struct Hospitalizations : SaverTable<Hospitalizations>
    {
        std::vector< int > date;
        std::vector< int > virus_id;
        std::vector< int > tool_id;
        std::vector< int > count;
        std::vector< double > weight;
        static auto columns()
        {
            return std::make_tuple(
                saver_column("date", &Hospitalizations::date),
                saver_column("virus_id", &Hospitalizations::virus_id),
                saver_column("tool_id", &Hospitalizations::tool_id),
                saver_column("count", &Hospitalizations::count),
                saver_column("weight", &Hospitalizations::weight)
            );
        }
    };

    TotalHist total_hist;
    VirusInfo virus_info;
    VirusHist virus_hist;
    ToolInfo tool_info;
    ToolHist tool_hist;
    Transmission transmission;
    Transition transition;
    Reproductive reproductive;
    Generation generation;
    ActiveCases active_cases;
    OutbreakSize outbreak_size;
    Hospitalizations hospitalizations;

    /**
     * @brief Calls `fun(name, option, table)` for each output
     *
     * @details `name` is the output's name (also the suffix of its file),
     * `option` points to its flag in `SaveOptions`, and `table` points to its
     * member in `RunOutputs`. This is the only list of the outputs.
     */
    template<typename TFun>
    static void for_each_table(TFun && fun);

    void set_sim_id(int id); ///< Sets the simulation ID of all tables.
    void append(const RunOutputs & other); ///< Appends the rows of `other`.

    bool operator==(const RunOutputs & other) const;

};

/**
 * @brief Writes a table to a file, throwing if the file cannot be written
 */
template<typename TTable>
inline void saver_write_file(const std::string & fn, const TTable & table);

/**
 * @brief Collects the outputs of the simulations in `Model::run_multiple()`
 *
 * @details A saver splits the work in two: `extract()` copies the outputs out
 * of a finished simulation, and `write()` stores them. Under OpenMP, each
 * thread calls both for its own simulations, without locks. So `write()` may
 * be called concurrently for different simulation IDs, and implementations
 * must only touch state that belongs to that ID (like `SaverMemory` and
 * `SaverFiles`) or synchronize internally (like `SaverCallback`).
 *
 * `begin()` is called once before the first simulation, and `end()` once
 * after the last one; `end()` is not called if a simulation or saver throws.
 * Bindings that run the simulations themselves can call these four functions
 * directly.
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class Saver
{
protected:
    SaveOptions options;

public:

    explicit Saver(SaveOptions options_ = {}) : options(options_) {}
    virtual ~Saver() = default;

    virtual void begin(size_t) {} ///< Before the first simulation.

    /**
     * @brief Copies the selected outputs of a finished simulation
     * @details Thread-safe as long as `model` is not running.
     */
    RunOutputs extract(size_t sim_id, const Model<TSeq> & model) const;

    /**
     * @brief Stores the outputs of a simulation
     * @details May be called concurrently for different `sim_id`s.
     */
    virtual void write(size_t sim_id, RunOutputs && outputs) = 0;

    virtual void end() {} ///< After the last simulation.

};

/**
 * @brief Keeps the outputs in memory, one slot per simulation
 *
 * @details `begin()` allocates the slots, so `write()` only touches its own
 * slot and needs no lock. Results are ordered by simulation ID, whatever the
 * order the simulations finish in. Starting another `run_multiple()` with the
 * same saver clears the previous results.
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class SaverMemory : public Saver<TSeq>
{
private:
    std::vector< RunOutputs > runs;

public:

    using Saver<TSeq>::Saver;

    void begin(size_t nexperiments) override;
    void write(size_t sim_id, RunOutputs && outputs) override;

    RunOutputs results() const; ///< All simulations, concatenated.

    /**
     * @brief Like `results()`, but moves the outputs out of the saver
     * @details Frees each simulation's outputs as it is appended, so peak
     * memory stays close to one copy of the results.
     */
    RunOutputs take_results();

};

/**
 * @brief Passes the outputs of each simulation to a function
 *
 * @details Callbacks often share state across simulations, so they run one at
 * a time, in the order the simulations finish.
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class SaverCallback : public Saver<TSeq>
{
private:
    std::function<void(size_t, RunOutputs &&)> callback;
    std::mutex callback_mutex;

public:

    SaverCallback(
        SaveOptions options_,
        std::function<void(size_t, RunOutputs &&)> callback_
    );

    void write(size_t sim_id, RunOutputs && outputs) override;

};

/**
 * @brief Writes the outputs of each simulation to files
 *
 * @details Writes the files of `DataBase::write_data()`, named
 * `<prefix>_<output>.csv`, where the prefix is `format` with its integer
 * placeholder replaced by the simulation ID (e.g., `"%03lu-episimulation"`).
 * Each simulation writes its own files, so `write()` needs no lock.
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class SaverFiles : public Saver<TSeq>
{
private:
    std::string format; ///< `format_` with its placeholder rewritten for `long long`.
    bool format_signed = false;

public:

    explicit SaverFiles(std::string format_, SaveOptions options_ = {});

    void write(size_t sim_id, RunOutputs && outputs) override;

};

#endif
