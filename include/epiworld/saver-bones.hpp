#ifndef EPIWORLD_SAVER_BONES_HPP
#define EPIWORLD_SAVER_BONES_HPP

/** Select outputs without positional boolean arguments. */
struct SaveOptions {
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

/** Columnar total_hist table, with the same fields and row order as write_data. */
struct Output_total_hist {
    std::vector<size_t> sim_id;
    std::vector<int> date;
    std::vector<int> nviruses;
    std::vector<std::string> state;
    std::vector<int> counts;
    size_t size() const { return date.size(); }
    void append(size_t id, const Output_total_hist& other) {
        sim_id.insert(sim_id.end(), other.size(), id);
        date.insert(date.end(), other.date.begin(), other.date.end());
        nviruses.insert(nviruses.end(), other.nviruses.begin(), other.nviruses.end());
        state.insert(state.end(), other.state.begin(), other.state.end());
        counts.insert(counts.end(), other.counts.begin(), other.counts.end());
    }
    void write(std::ostream& out) const {
#ifdef EPI_DEBUG
        out << "thread ";
#endif
        out << "date nviruses state counts\n";
        for (size_t i = 0; i < size(); ++i) {
#ifdef EPI_DEBUG
            out << EPI_GET_THREAD_ID() << ' ';
#endif
            out << date[i] << ' ' << nviruses[i] << ' ' << '"' << state[i] << '"' << ' ' << counts[i] << '\n';
        }
    }
};

/** Columnar virus_info table, with the same fields and row order as write_data. */
struct Output_virus_info {
    std::vector<size_t> sim_id;
    std::vector<int> virus_id;
    std::vector<std::string> virus;
    std::vector<std::string> virus_sequence;
    std::vector<int> date_recorded;
    std::vector<int> parent;
    size_t size() const { return virus_id.size(); }
    void append(size_t id, const Output_virus_info& other) {
        sim_id.insert(sim_id.end(), other.size(), id);
        virus_id.insert(virus_id.end(), other.virus_id.begin(), other.virus_id.end());
        virus.insert(virus.end(), other.virus.begin(), other.virus.end());
        virus_sequence.insert(virus_sequence.end(), other.virus_sequence.begin(), other.virus_sequence.end());
        date_recorded.insert(date_recorded.end(), other.date_recorded.begin(), other.date_recorded.end());
        parent.insert(parent.end(), other.parent.begin(), other.parent.end());
    }
    void write(std::ostream& out) const {
#ifdef EPI_DEBUG
        out << "thread ";
#endif
        out << "virus_id virus virus_sequence date_recorded parent\n";
        for (size_t i = 0; i < size(); ++i) {
#ifdef EPI_DEBUG
            out << EPI_GET_THREAD_ID() << ' ';
#endif
            out << virus_id[i] << ' ' << '"' << virus[i] << '"' << ' ' << virus_sequence[i] << ' ' << date_recorded[i] << ' ' << parent[i] << '\n';
        }
    }
};

/** Columnar virus_hist table, with the same fields and row order as write_data. */
struct Output_virus_hist {
    std::vector<size_t> sim_id;
    std::vector<int> date;
    std::vector<int> virus_id;
    std::vector<std::string> virus;
    std::vector<std::string> state;
    std::vector<int> n;
    size_t size() const { return date.size(); }
    void append(size_t id, const Output_virus_hist& other) {
        sim_id.insert(sim_id.end(), other.size(), id);
        date.insert(date.end(), other.date.begin(), other.date.end());
        virus_id.insert(virus_id.end(), other.virus_id.begin(), other.virus_id.end());
        virus.insert(virus.end(), other.virus.begin(), other.virus.end());
        state.insert(state.end(), other.state.begin(), other.state.end());
        n.insert(n.end(), other.n.begin(), other.n.end());
    }
    void write(std::ostream& out) const {
#ifdef EPI_DEBUG
        out << "thread ";
#endif
        out << "date virus_id virus state n\n";
        for (size_t i = 0; i < size(); ++i) {
#ifdef EPI_DEBUG
            out << EPI_GET_THREAD_ID() << ' ';
#endif
            out << date[i] << ' ' << virus_id[i] << ' ' << '"' << virus[i] << '"' << ' ' << '"' << state[i] << '"' << ' ' << n[i] << '\n';
        }
    }
};

/** Columnar tool_info table, with the same fields and row order as write_data. */
struct Output_tool_info {
    std::vector<size_t> sim_id;
    std::vector<int> id;
    std::vector<std::string> tool_name;
    std::vector<std::string> tool_sequence;
    std::vector<int> date_recorded;
    size_t size() const { return id.size(); }
    void append(size_t id, const Output_tool_info& other) {
        sim_id.insert(sim_id.end(), other.size(), id);
        this->id.insert(this->id.end(), other.id.begin(), other.id.end());
        tool_name.insert(tool_name.end(), other.tool_name.begin(), other.tool_name.end());
        tool_sequence.insert(tool_sequence.end(), other.tool_sequence.begin(), other.tool_sequence.end());
        date_recorded.insert(date_recorded.end(), other.date_recorded.begin(), other.date_recorded.end());
    }
    void write(std::ostream& out) const {
#ifdef EPI_DEBUG
        out << "thread ";
#endif
        out << "id tool_name tool_sequence date_recorded\n";
        for (size_t i = 0; i < size(); ++i) {
#ifdef EPI_DEBUG
            out << EPI_GET_THREAD_ID() << ' ';
#endif
            out << id[i] << ' ' << '"' << tool_name[i] << '"' << ' ' << tool_sequence[i] << ' ' << date_recorded[i] << '\n';
        }
    }
};

/** Columnar tool_hist table, with the same fields and row order as write_data. */
struct Output_tool_hist {
    std::vector<size_t> sim_id;
    std::vector<int> date;
    std::vector<int> id;
    std::vector<std::string> state;
    std::vector<int> n;
    size_t size() const { return date.size(); }
    void append(size_t id, const Output_tool_hist& other) {
        sim_id.insert(sim_id.end(), other.size(), id);
        date.insert(date.end(), other.date.begin(), other.date.end());
        this->id.insert(this->id.end(), other.id.begin(), other.id.end());
        state.insert(state.end(), other.state.begin(), other.state.end());
        n.insert(n.end(), other.n.begin(), other.n.end());
    }
    void write(std::ostream& out) const {
#ifdef EPI_DEBUG
        out << "thread ";
#endif
        out << "date id state n\n";
        for (size_t i = 0; i < size(); ++i) {
#ifdef EPI_DEBUG
            out << EPI_GET_THREAD_ID() << ' ';
#endif
            out << date[i] << ' ' << id[i] << ' ' << '"' << state[i] << '"' << ' ' << n[i] << '\n';
        }
    }
};

/** Columnar transmission table, with the same fields and row order as write_data. */
struct Output_transmission {
    std::vector<size_t> sim_id;
    std::vector<int> date;
    std::vector<int> virus_id;
    std::vector<std::string> virus;
    std::vector<int> source_exposure_date;
    std::vector<int> source;
    std::vector<int> target;
    size_t size() const { return date.size(); }
    void append(size_t id, const Output_transmission& other) {
        sim_id.insert(sim_id.end(), other.size(), id);
        date.insert(date.end(), other.date.begin(), other.date.end());
        virus_id.insert(virus_id.end(), other.virus_id.begin(), other.virus_id.end());
        virus.insert(virus.end(), other.virus.begin(), other.virus.end());
        source_exposure_date.insert(source_exposure_date.end(), other.source_exposure_date.begin(), other.source_exposure_date.end());
        source.insert(source.end(), other.source.begin(), other.source.end());
        target.insert(target.end(), other.target.begin(), other.target.end());
    }
    void write(std::ostream& out) const {
#ifdef EPI_DEBUG
        out << "thread ";
#endif
        out << "date virus_id virus source_exposure_date source target\n";
        for (size_t i = 0; i < size(); ++i) {
#ifdef EPI_DEBUG
            out << EPI_GET_THREAD_ID() << ' ';
#endif
            out << date[i] << ' ' << virus_id[i] << ' ' << '"' << virus[i] << '"' << ' ' << source_exposure_date[i] << ' ' << source[i] << ' ' << target[i] << '\n';
        }
    }
};

/** Columnar transition table, with the same fields and row order as write_data. */
struct Output_transition {
    std::vector<size_t> sim_id;
    std::vector<int> date;
    std::vector<std::string> from;
    std::vector<std::string> to;
    std::vector<int> counts;
    size_t size() const { return date.size(); }
    void append(size_t id, const Output_transition& other) {
        sim_id.insert(sim_id.end(), other.size(), id);
        date.insert(date.end(), other.date.begin(), other.date.end());
        from.insert(from.end(), other.from.begin(), other.from.end());
        to.insert(to.end(), other.to.begin(), other.to.end());
        counts.insert(counts.end(), other.counts.begin(), other.counts.end());
    }
    void write(std::ostream& out) const {
#ifdef EPI_DEBUG
        out << "thread ";
#endif
        out << "date from to counts\n";
        for (size_t i = 0; i < size(); ++i) {
#ifdef EPI_DEBUG
            out << EPI_GET_THREAD_ID() << ' ';
#endif
            out << date[i] << ' ' << '"' << from[i] << '"' << ' ' << '"' << to[i] << '"' << ' ' << counts[i] << '\n';
        }
    }
};

/** Columnar reproductive table, with the same fields and row order as write_data. */
struct Output_reproductive {
    std::vector<size_t> sim_id;
    std::vector<int> virus_id;
    std::vector<std::string> virus;
    std::vector<int> source;
    std::vector<int> source_exposure_date;
    std::vector<int> rt;
    size_t size() const { return virus_id.size(); }
    void append(size_t id, const Output_reproductive& other) {
        sim_id.insert(sim_id.end(), other.size(), id);
        virus_id.insert(virus_id.end(), other.virus_id.begin(), other.virus_id.end());
        virus.insert(virus.end(), other.virus.begin(), other.virus.end());
        source.insert(source.end(), other.source.begin(), other.source.end());
        source_exposure_date.insert(source_exposure_date.end(), other.source_exposure_date.begin(), other.source_exposure_date.end());
        rt.insert(rt.end(), other.rt.begin(), other.rt.end());
    }
    void write(std::ostream& out) const {
#ifdef EPI_DEBUG
        out << "thread ";
#endif
        out << "virus_id virus source source_exposure_date rt\n";
        for (size_t i = 0; i < size(); ++i) {
#ifdef EPI_DEBUG
            out << EPI_GET_THREAD_ID() << ' ';
#endif
            out << virus_id[i] << ' ' << '"' << virus[i] << '"' << ' ' << source[i] << ' ' << source_exposure_date[i] << ' ' << rt[i] << '\n';
        }
    }
};

/** Columnar generation table, with the same fields and row order as write_data. */
struct Output_generation {
    std::vector<size_t> sim_id;
    std::vector<int> virus;
    std::vector<int> source;
    std::vector<int> source_exposure_date;
    std::vector<int> gentime;
    size_t size() const { return virus.size(); }
    void append(size_t id, const Output_generation& other) {
        sim_id.insert(sim_id.end(), other.size(), id);
        virus.insert(virus.end(), other.virus.begin(), other.virus.end());
        source.insert(source.end(), other.source.begin(), other.source.end());
        source_exposure_date.insert(source_exposure_date.end(), other.source_exposure_date.begin(), other.source_exposure_date.end());
        gentime.insert(gentime.end(), other.gentime.begin(), other.gentime.end());
    }
    void write(std::ostream& out) const {
#ifdef EPI_DEBUG
        out << "thread ";
#endif
        out << "virus source source_exposure_date gentime\n";
        for (size_t i = 0; i < size(); ++i) {
#ifdef EPI_DEBUG
            out << EPI_GET_THREAD_ID() << ' ';
#endif
            out << virus[i] << ' ' << source[i] << ' ' << source_exposure_date[i] << ' ' << gentime[i] << '\n';
        }
    }
};

/** Columnar active_cases table, with the same fields and row order as write_data. */
struct Output_active_cases {
    std::vector<size_t> sim_id;
    std::vector<int> date;
    std::vector<int> virus_id;
    std::vector<std::string> virus;
    std::vector<int> active_cases;
    size_t size() const { return date.size(); }
    void append(size_t id, const Output_active_cases& other) {
        sim_id.insert(sim_id.end(), other.size(), id);
        date.insert(date.end(), other.date.begin(), other.date.end());
        virus_id.insert(virus_id.end(), other.virus_id.begin(), other.virus_id.end());
        virus.insert(virus.end(), other.virus.begin(), other.virus.end());
        active_cases.insert(active_cases.end(), other.active_cases.begin(), other.active_cases.end());
    }
    void write(std::ostream& out) const {
#ifdef EPI_DEBUG
        out << "thread ";
#endif
        out << "date virus_id virus active_cases\n";
        for (size_t i = 0; i < size(); ++i) {
#ifdef EPI_DEBUG
            out << EPI_GET_THREAD_ID() << ' ';
#endif
            out << date[i] << ' ' << virus_id[i] << ' ' << '"' << virus[i] << '"' << ' ' << active_cases[i] << '\n';
        }
    }
};

/** Columnar outbreak_size table, with the same fields and row order as write_data. */
struct Output_outbreak_size {
    std::vector<size_t> sim_id;
    std::vector<int> date;
    std::vector<int> virus_id;
    std::vector<std::string> virus;
    std::vector<int> outbreak_size;
    size_t size() const { return date.size(); }
    void append(size_t id, const Output_outbreak_size& other) {
        sim_id.insert(sim_id.end(), other.size(), id);
        date.insert(date.end(), other.date.begin(), other.date.end());
        virus_id.insert(virus_id.end(), other.virus_id.begin(), other.virus_id.end());
        virus.insert(virus.end(), other.virus.begin(), other.virus.end());
        outbreak_size.insert(outbreak_size.end(), other.outbreak_size.begin(), other.outbreak_size.end());
    }
    void write(std::ostream& out) const {
#ifdef EPI_DEBUG
        out << "thread ";
#endif
        out << "date virus_id virus outbreak_size\n";
        for (size_t i = 0; i < size(); ++i) {
#ifdef EPI_DEBUG
            out << EPI_GET_THREAD_ID() << ' ';
#endif
            out << date[i] << ' ' << virus_id[i] << ' ' << '"' << virus[i] << '"' << ' ' << outbreak_size[i] << '\n';
        }
    }
};

/** Columnar hospitalizations table, with the same fields and row order as write_data. */
struct Output_hospitalizations {
    std::vector<size_t> sim_id;
    std::vector<int> date;
    std::vector<int> virus_id;
    std::vector<int> tool_id;
    std::vector<int> count;
    std::vector<double> weight;
    size_t size() const { return date.size(); }
    void append(size_t id, const Output_hospitalizations& other) {
        sim_id.insert(sim_id.end(), other.size(), id);
        date.insert(date.end(), other.date.begin(), other.date.end());
        virus_id.insert(virus_id.end(), other.virus_id.begin(), other.virus_id.end());
        tool_id.insert(tool_id.end(), other.tool_id.begin(), other.tool_id.end());
        count.insert(count.end(), other.count.begin(), other.count.end());
        weight.insert(weight.end(), other.weight.begin(), other.weight.end());
    }
    void write(std::ostream& out) const {
#ifdef EPI_DEBUG
        out << "thread ";
#endif
        out << "date virus_id tool_id count weight\n";
        for (size_t i = 0; i < size(); ++i) {
#ifdef EPI_DEBUG
            out << EPI_GET_THREAD_ID() << ' ';
#endif
            out << date[i] << ' ' << virus_id[i] << ' ' << tool_id[i] << ' ' << count[i] << ' ' << weight[i] << '\n';
        }
    }
};

/** Results for one simulation; concatenated results include sim_id per row. */
struct RunOutputs {
    Output_total_hist total_hist;
    Output_virus_info virus_info;
    Output_virus_hist virus_hist;
    Output_tool_info tool_info;
    Output_tool_hist tool_hist;
    Output_transmission transmission;
    Output_transition transition;
    Output_reproductive reproductive;
    Output_generation generation;
    Output_active_cases active_cases;
    Output_outbreak_size outbreak_size;
    Output_hospitalizations hospitalizations;
    void append(size_t id, const RunOutputs& other) {
        total_hist.append(id, other.total_hist);
        virus_info.append(id, other.virus_info);
        virus_hist.append(id, other.virus_hist);
        tool_info.append(id, other.tool_info);
        tool_hist.append(id, other.tool_hist);
        transmission.append(id, other.transmission);
        transition.append(id, other.transition);
        reproductive.append(id, other.reproductive);
        generation.append(id, other.generation);
        active_cases.append(id, other.active_cases);
        outbreak_size.append(id, other.outbreak_size);
        hospitalizations.append(id, other.hospitalizations);
    }
};

template<typename TSeq> class Model;

/** Extraction can run concurrently; writes are serialized by run_multiple. */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class Saver {
protected:
    SaveOptions options;
public:
    explicit Saver(SaveOptions opts = {}) : options(opts) {}
    virtual ~Saver() = default;
    virtual void begin(size_t) {}
    RunOutputs extract(size_t, const Model<TSeq>& model) const;
    virtual void write(size_t sim_id, RunOutputs&& out) = 0;
    virtual void end() {}
};

template<typename TSeq = EPI_DEFAULT_TSEQ>
class SaverMemory : public Saver<TSeq> {
    std::map<size_t, RunOutputs> runs;
public:
    using Saver<TSeq>::Saver;
    void begin(size_t) override { runs.clear(); }
    void write(size_t id, RunOutputs&& out) override {
        runs.insert_or_assign(id, std::move(out));
    }
    RunOutputs results() const {
        RunOutputs out;
        for (const auto& run : runs)
            out.append(run.first, run.second);
        return out;
    }
};

template<typename TSeq = EPI_DEFAULT_TSEQ>
class SaverCallback : public Saver<TSeq> {
    std::function<void(size_t, RunOutputs&&)> callback;
public:
    SaverCallback(SaveOptions opts,
        std::function<void(size_t, RunOutputs&&)> fun) :
        Saver<TSeq>(opts), callback(std::move(fun)) {
        if (!callback)
            throw std::invalid_argument("SaverCallback requires a callback.");
    }
    void write(size_t id, RunOutputs&& out) override {
        callback(id, std::move(out));
    }
};

template<typename TSeq = EPI_DEFAULT_TSEQ>
class SaverFiles : public Saver<TSeq> {
    std::string format;
public:
    explicit SaverFiles(std::string fmt, SaveOptions opts = {});
    void write(size_t id, RunOutputs&& out) override;
};

#endif
