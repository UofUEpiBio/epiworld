#ifndef EPIWORLD_HOSPITALIZATIONSTRACKER_BONES_HPP
#define EPIWORLD_HOSPITALIZATIONSTRACKER_BONES_HPP

/**
 * @brief Class to track hospitalizations in an epidemiological model.
 * 
 * @details
 * This class keeps track of hospitalizations in a model. For each hospitalized
 * agent, it records the date, virus ID, and tool IDs with appropriate weights.
 * 
 * Since agents always have at most one virus but may have multiple tools,
 * if an agent has N tools, then N records are created, each with tool_weight
 * equal to 1/N. If the agent has no tools, a single record is created with
 * tool_id = -1 and tool_weight = 1.0.
 * 
 * The `get()` method returns a summary of the hospitalizations grouped by
 * date, virus_id, and tool_id, with tool_weight summed across all matching
 * records.
 * 
 * @tparam TSeq Type of sequence (should match `TSeq` across the model)
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class HospitalizationsTracker {

private:

    std::vector<int> _date;           ///< Date of hospitalization
    std::vector<int> _virus_id;       ///< ID of the virus causing hospitalization
    std::vector<int> _tool_id;        ///< ID of the tool (-1 if no tools)
    std::vector<double> _tool_weight; ///< Weight for the tool (1/N where N is tool count)

public:

    HospitalizationsTracker() = default;

    /**
     * @brief Reset the tracker by clearing all data.
     */
    void reset();

    /**
     * @brief Record a hospitalization event for an agent.
     * 
     * @param agent Reference to the agent being hospitalized.
     * @param model Reference to the model.
     * 
     * @details
     * For each hospitalization, the method records:
     * - The current date from the model
     * - The virus ID from the agent's virus
     * - For each tool the agent has, a separate record with tool_weight = 1/N
     *   where N is the number of tools
     * - If the agent has no tools, a single record with tool_id = -1 and
     *   tool_weight = 1.0
     */
    void record(Agent<TSeq> & agent, Model<TSeq> & model);

    /**
     * @brief Get the summary of hospitalizations.
     * 
     * @param date Output vector for dates.
     * @param virus_id Output vector for virus IDs.
     * @param tool_id Output vector for tool IDs.
     * @param tool_weight Output vector for summed tool weights.
     * 
     * @details
     * Returns aggregated data grouped by (date, virus_id, tool_id) with
     * tool_weight summed across all matching records.
     */
    void get(
        std::vector<int> & date,
        std::vector<int> & virus_id,
        std::vector<int> & tool_id,
        std::vector<double> & tool_weight
    ) const;

    /**
     * @brief Get the number of raw records in the tracker.
     * @return Number of records.
     */
    size_t size() const;

};

template<typename TSeq>
inline void HospitalizationsTracker<TSeq>::reset()
{
    _date.clear();
    _virus_id.clear();
    _tool_id.clear();
    _tool_weight.clear();
}

template<typename TSeq>
inline void HospitalizationsTracker<TSeq>::record(
    Agent<TSeq> & agent,
    Model<TSeq> & model
)
{
    int current_date = model.today();
    
    // Get the virus ID (-1 if no virus)
    int v_id = -1;
    auto & virus = agent.get_virus();
    if (virus != nullptr)
        v_id = virus->get_id();
    
    // Get the number of tools
    size_t n_tools = agent.get_n_tools();
    
    if (n_tools == 0u)
    {
        // No tools: single record with tool_id = -1 and weight = 1.0
        _date.push_back(current_date);
        _virus_id.push_back(v_id);
        _tool_id.push_back(-1);
        _tool_weight.push_back(1.0);
    }
    else
    {
        // Multiple tools: one record per tool with weight = 1/N
        double weight = 1.0 / static_cast<double>(n_tools);
        for (size_t i = 0u; i < n_tools; ++i)
        {
            _date.push_back(current_date);
            _virus_id.push_back(v_id);
            _tool_id.push_back(agent.get_tool(static_cast<int>(i))->get_id());
            _tool_weight.push_back(weight);
        }
    }
}

template<typename TSeq>
inline void HospitalizationsTracker<TSeq>::get(
    std::vector<int> & date,
    std::vector<int> & virus_id,
    std::vector<int> & tool_id,
    std::vector<double> & tool_weight
) const
{
    // Clear output vectors
    date.clear();
    virus_id.clear();
    tool_id.clear();
    tool_weight.clear();
    
    if (_date.empty())
        return;
    
    // Use a map to aggregate by (date, virus_id, tool_id)
    // Key: (date, virus_id, tool_id), Value: sum of tool_weight
    // Note: We use std::map (not unordered_map) to ensure consistent, 
    // ordered output which is useful for reproducibility and debugging.
    std::map<std::tuple<int, int, int>, double> aggregated;
    
    for (size_t i = 0u; i < _date.size(); ++i)
    {
        auto key = std::make_tuple(_date[i], _virus_id[i], _tool_id[i]);
        aggregated[key] += _tool_weight[i];
    }
    
    // Convert map to output vectors
    for (const auto & entry : aggregated)
    {
        date.push_back(std::get<0>(entry.first));
        virus_id.push_back(std::get<1>(entry.first));
        tool_id.push_back(std::get<2>(entry.first));
        tool_weight.push_back(entry.second);
    }
}

template<typename TSeq>
inline size_t HospitalizationsTracker<TSeq>::size() const
{
    return _date.size();
}

#endif
