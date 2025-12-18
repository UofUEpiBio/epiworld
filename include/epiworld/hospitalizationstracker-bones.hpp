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

#endif
