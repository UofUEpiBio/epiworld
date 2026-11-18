## Custom Agent Behaviors
In epiworld, agents are the fundamental units of simulation. Each `Agent` represents an individual in the modeled population; what it does at each step follows from its stage (in the virus's natural history) and its status (free, quarantined, isolated, ...). Both are declared, not coded: the transitions out of a stage or a status say how an agent leaves it (see [Extending the Library with Models](extending-the-library-with-models.md)).

A transition's value can be a function of the agent and the model, so behavior can depend on anything the model holds:

```cpp
// The daily probability of recovery grows with the days infected
virus.rate(I, R, Value([I](Agent & agent, Model & model) -> epiworld_double {
    const int days = model.today() - model.get_stage_entered(agent.get_id(), I);
    return std::min(1.0, 0.05 * days);
})).effect(Effect::recovery);
```

So can a duration (`Duration` from a function) and the sampler of an infection (`.sampler()`, which picks the infector itself instead of using the contacts). A new "Isolated" behavior is a status with contact weight 0, which takes its agents out of every layer, and the transitions in and out of it:

```cpp
Statuses st;
int free = st.add_status("Free");
int isolated = st.add_status("Isolated");
st.set_contact(isolated, 0.0);
st.rate(free, isolated, 0.3).name("Detected").when({I});      // only infected agents
st.after(isolated, free, Duration::fixed(7)).name("Released");
model.set_statuses(st);
```

Code can move an agent along a declared transition with `Model::fire(transition, agent)` (e.g., from a global event), and `Model::react()` fires a transition, or calls a function, when an agent enters a stage or a status. `Model::get_stage(agent)` and `Model::get_status(agent)` read where an agent is. Tools (`add_tool()`) change what a virus does to an agent: its susceptibility, its transmission, its recovery and its death (`Effect` on a transition says which a rate takes).

## Custom Network Structures
A network layer is built from a graph, fixed or drawn by a generator at every reset (`layer::network(name, graph)`, `layer::network(name, generator)`). Its ties can be edited at any point of a run with `add_edge()` and `rm_edge()`, and `set_rewiring()` gives it a function called at the end of every step, after the global events:

```cpp
// Each day, infected agents drop their ties (e.g., they stay at home)
auto net = layer::network("contacts", graph::smallworld(10, 0.05));
net->set_rewiring([I](layer::Network & layer, Model & model) -> void {
    for (auto & agent : model.get_agents())
        if (model.get_stage(agent) == I)
            while (layer.get_degree(agent.get_id()) > 0u)
                layer.rm_edge(
                    agent.get_id(),
                    (*layer.neighbors_view(agent.get_id()).begin())->get_id()
                );
});
model.add_layer(net);
```

`graph::rewire_degseq()` returns a rewiring that swaps ties and keeps every agent's degree. An edit made during a run applies to that run only: the next reset restores a fixed graph, or draws a new one from the generator. A model can have several layers (a household network, a workplace network, a community mixing layer), and a layer can be restricted to some viruses or given a weight.

## Implementing New Global Events
Global events represent actions that occur at the level of the entire simulation, rather than being tied to individual agents. They are executed either on specific dates or repeatedly at the end of every day, depending on how they are configured. The intent behind this system is to provide a clean and flexible way to modify the model state during runtime, such as changing parameters, adding or removing viruses, deploying tools, or otherwise altering the environment in which agents operate. This allows users to model interventions, seasonal effects, or policy implementations without having to modify agent-level logic directly.

A global event is defined as a function matching the signature `void fun(Model*)`. Within this function, the user has access to the entire model object, and can therefore manipulate global variables, change parameters, modify the population, or trigger any built-in methods. For instance, a simple "lockdown" event might reduce the transmission probability or remove ties from a network layer. To register such an event, the user calls `add_globalevent(fun, name, date)`, where `fun` is the function to execute, `name` is a descriptive label, and `date` determines when it will be triggered. If the date is a nonnegative integer, the event will be called at the end of that specific day; if it is negative (typically -1), it will be called after every simulation step.

```cpp
void lockdown_event(epiworld::Model* model) {
    double tr = model->get_param("Transmission probability");
    model->set_param("Transmission probability", tr * 0.5);
}

model.add_globalevent(lockdown_event, "Lockdown Policy", 30);
```

Once registered, the event becomes part of the model’s internal schedule and will automatically execute at the designated time. The model also allows retrieval and management of global events through `get_globalevent()` and `rm_globalevent()` functions, which can be used to inspect or remove events dynamically.

```cpp
auto &ge = model.get_globalevent("Lockdown Policy");
model.rm_globalevent("Lockdown Policy");
```

Global events can also interact with the random number generator, the database, and other components of the model. For example, an event could sample a subset of agents and give them a tool the model has:

```cpp
void vaccination_event(epiworld::Model* model) {
    epiworld::Tool & vaccine = model->get_tool("Vaccine");
    for (auto & agent : model->get_agents())
        if (model->runif() < 0.01)
            agent.add_tool(*model, vaccine);
}
```

Because global events execute in the same runtime context as the model itself, they can modify any aspect of it—including parameters, the population vector, the event queue, and even the set of available tools or viruses. This flexibility allows epiworld to support sophisticated intervention-driven or phased modeling workflows that go beyond static parameterization.

## Creating Custom Data Collection Strategies
Data collection in epiworld is handled primarily through the `DataBase` component, which records simulation results such as infection counts, transmission histories, and per-agent or per-virus summaries. The built-in mechanisms already support a range of outputs—total histories, virus-specific and tool-specific information, transmission trees, and more—but users can also extend or replace these mechanisms to collect custom data according to their own needs. Custom data collection strategies are useful for recording specialized statistics, debugging model behavior, or integrating the simulation with external analysis workflows.

Each `Model` object owns a `DataBase` instance accessible via `get_db()`. By default, the model writes data to this database automatically as the simulation progresses. However, users can modify or extend this behavior by interacting directly with the database object or by defining custom recorders. The database keeps track of simulation-level quantities and agent-level states through standardized internal tables, which can be written to disk using `model.write_data()` after a run. The output can include files for total counts over time, transmission histories, transition matrices, and per-virus or per-tool data, depending on which options are enabled.

For models requiring custom tracking, users can either subclass `DataBase` or define their own collection logic via the `Model`'s hooks. A simple approach is to register a global event that records quantities of interest each day—for instance, the proportion of vaccinated agents, or the number of currently infected nodes with a particular attribute. The event function can append results to an external data structure or to the `user_data` table within the model. The `Model` class provides convenience methods such as `set_user_data()`, `add_user_data()`, and `get_user_data()` to store arbitrary numeric variables associated with each timestep. This system is lightweight but flexible enough for most purposes.

```cpp
void record_daily_prevalence(epiworld::Model* model) {
    const int infected = model->get_db().get_today_total_stage("Infected");
    model->add_user_data(0, static_cast< double >(infected) / model->size());
}

model.set_user_data({"Prevalence"});
model.add_globalevent(record_daily_prevalence, "Daily prevalence", -1);
```

For more advanced data collection, users can directly access the model’s internals through `get_agents()`, `get_layer()`, and `get_viruses()` to query agent states, infection statuses, or tool distributions at any point during the simulation. Because epiworld maintains all these objects in contiguous containers, iterating over them to compute aggregate statistics is efficient. Users can combine these features to implement real-time monitoring or adaptive experiments where results from one run inform modifications to subsequent runs.

The `make_save_run()` utility function offers another extension point. It returns a callable that can be used in conjunction with `run_multiple()` to define how output is written after each replicate. For example:

```cpp
auto custom_saver = [](size_t i, epiworld::Model* model) {
    model->write_data(
        "virus_info.csv",
        "virus_hist.csv",
        "tool_info.csv",
        "tool_hist.csv",
        "total_hist.csv",
        "transmission.csv",
        "transition.csv",
        "rnum.csv",
        "generation.csv"
    );
};

model.run_multiple(100, 5, 123, custom_saver);
```

By providing a custom function of type `std::function<void(size_t, Model*)>`, users can collect or export data in any desired format—CSV, binary, or in-memory. This design encourages users to treat data collection as part of the modeling logic rather than as a post-processing step.

Overall, custom data collection in epiworld is best thought of as an open system rather than a closed reporting interface. The built-in database handles common epidemiological summaries automatically, but because the `Model` exposes its entire internal state, users can track virtually anything they need without modifying the core simulation engine, from degree distributions to conditional infection probabilities.

## See Also

- [Library Architecture](library-architecture.md) — the modular `Agent`/`Model` design that these extension points build on.
- [Extending the Library with Models](extending-the-library-with-models.md) — creating new models via inheritance from `Model`.
- [Events and Multi-Event Handling](events-and-multi-event-handling.md) — the event queue that applies the day's changes.
- [Data Collection and Analysis](data-collection-and-analysis.md) — the `DataBase` class and built-in recording mechanisms.
- [Reproducibility and `run_multiple`](reproducibility-and-run-multiple.md) — using saver callbacks and parallel execution with `run_multiple`.