## Overview
The `DataBase` class is responsible for collecting, organizing, and exporting statistical data generated during simulations. This data encompasses a wide range of information, including the state of agents, the history of viruses and tools, and the transmission network. The `DataBase` class is tightly integrated with the `Model` class, which provides the context for the simulation, such as the agents, their states, and the interactions between them.

We track data at multiple levels of granularity. An agent's state is a pair: its stage (in the virus's natural history) and its status (free, quarantined, ...; "Free" in a model without statuses). The counts are kept per pair, so a total by stage or by status is a sum over the rows, and every declared transition is counted by name. Additionally, it maintains a history of virus and tool events, such as the introduction of new variants or the application of interventions. The transmission network is also recorded, capturing the source, target, and timing of each transmission event. This comprehensive data collection allows for detailed analysis of the simulation's behavior.

## Recording During Simulations
We automatically record data at each step of the simulation. This includes the current state of all agents, the distribution of states for each virus and tool, and the overall state distribution. For example, the `record` method is called at the end of each simulation step to update the internal data structures with the latest information. This ensures that the data remains consistent with the simulation's progression.

Viruses and tools get ids in order: the viruses added to the model (`add_virus()`), then the variants their mutations make during a run (each a child of the variant it changed); the tools in the order they were added. Each transmission is recorded with its source, target, date, virus and layer.

The counts can be read during or after a run:

```cpp
const auto & db = model.get_db();

// How many agents are in a stage (any status), or in a status (any stage)
int infected = db.get_today_total_stage("Infected");
int isolated = db.get_today_total_status("Isolated");

// The daily counts of each (status, stage) pair
std::vector< int > date, counts;
std::vector< std::string > status, stage;
db.get_hist_total(&date, &status, &stage, &counts);

// The daily count of each declared transition, by name
std::vector< int > tdate, tcounts;
std::vector< std::string > transition;
db.get_hist_transitions(tdate, transition, tcounts);
```

A stage and a status may share a name: each of `get_today_total_stage()` and `get_today_total_status()` looks only among its own, and throws if the name is not one of them.

## Exporting Data

The `write_data` method (of the model, or of its database) allows for exporting simulation results to external files. This method allows users to specify multiple output files, each corresponding to a specific type of data. For example, one file may contain information about viruses, while another contains the transmission network. The data is written in a tabular format, making it easy to import into statistical software or spreadsheet applications for further analysis.

```cpp
model.write_data(
    "virus_info.csv",
    "virus_hist.csv",
    "tool_info.csv",
    "tool_hist.csv",
    "total_hist.csv",
    "transmission.csv",
    "transition.csv",
    "reproductive_number.csv",
    "generation_time.csv",
    "active_cases.csv",
    "outbreak_size.csv"
);
```

Each file contains a specific subset of the simulation data:
- `virus_info.csv`: Information about viruses, including their ids, names, origin dates, and parents.
- `virus_hist.csv`: The daily counts of each virus, by status and stage.
- `tool_info.csv`: Information about tools, including their ids, names, and origin dates.
- `tool_hist.csv`: The daily counts of each tool, by status and stage.
- `total_hist.csv`: The daily counts of each (status, stage) pair.
- `transmission.csv`: The transmission network, including the source, target, and timing of each event.
- `transition.csv`: The daily transition matrix, from each (status, stage) pair to each other.
- `reproductive_number.csv`: The reproductive number for each case.
- `generation_time.csv`: The generation time, which is the time between the infection of the source and the infection of the target.

The same tables are available in memory: `db.get_run_outputs({"total_hist", "transmission"})` returns them (see `run_output_names()` for the names), and `SaverMemory` keeps them for every replicate of `run_multiple()`.

## Basic Analysis
The `DataBase` class includes methods for performing basic analysis on the simulation data. These methods allow users to calculate metrics such as transition probabilities, reproductive numbers, and generation times. For example, the `get_transition_probability` method calculates the probability of transitioning from one state to another, while the `get_reproductive_number` method computes the reproductive number for each case.

```cpp
std::vector<epiworld_double> transition_probs = db.get_transition_probability(true, true);
MapVec_type<int, int> reproductive_numbers = db.get_reproductive_number();

std::vector<int> agent_id, virus_id, time, gentime;
db.get_generation_time(agent_id, virus_id, time, gentime);
```

As an example of usefulness, transition probabilities can help identify the likelihood of agents moving between different states, while reproductive numbers can indicate the potential for the infection to spread. Generation times can be used to study the timing of transmission events and their impact on the overall dynamics.

It's worth noting that analysis of any great depth should likely not be done in C++. The model data should either be exported (see above) and imported into a tool of your choosing (e.g. R, Excel, or Python), or rewritten using either the `epiworldR` or `epiworldPy` bindings.
### Transmission Network Analysis
The `get_transmissions` method retrieves data about the transmission network, including the source, target, and timing of each event. This data can be used to construct transmission networks and analyze the spread of the infection. For example, users can visualize the network to identify clusters of infections or calculate network metrics such as centrality and clustering coefficients.

```cpp
std::vector<int> date, source, target, virus, source_exposure_date, layer;
db.get_transmissions(date, source, target, virus, source_exposure_date, layer);

for (size_t i = 0; i < date.size(); ++i) {
    std::cout << "Transmission on day " << date[i]
              << " from agent " << source[i]
              << " to agent " << target[i]
              << " with virus " << virus[i] << std::endl;
}
```

This example retrieves the transmission network data and prints it to the console. Each transmission event is described by its date, source, target, and the virus involved.

## See Also

- [Library Architecture](library-architecture.md) — the `Model` class that owns and populates the `DataBase`.
- [Reproducibility and `run_multiple`](reproducibility-and-run-multiple.md) — using saver callbacks with `run_multiple` to export data from repeated simulations.
- [Extending the Library](extending-the-library.md) — custom data collection strategies using global events and `user_data`.
- [Generation Interval and Reproductive Number Calculation](generation-interval-and-reproductive-number.md) — the mathematical details behind reproductive numbers and generation times computed by the database.