# In-memory simulation results

Use `SaverMemory` to collect repeated simulations without writing temporary
files. The existing callback overload and its file-writing default remain
available.

```cpp
epiworld::epimodels::ModelSEIRCONN<> model(
    "virus", 1000, 0.01, 4.0, 0.1, 3.0, 1.0 / 7.0);
model.verbose_off();
epiworld::SaveOptions options;
options.transition = true;
epiworld::SaverMemory<> saver(options);
model.run_multiple(100, 10, 42, saver, true, false, 4);
auto results = saver.results();
// results.total_hist.date, .nviruses, .state, .counts, .sim_id
// results.transition.date, .from, .to, .counts, .sim_id
```

`SaveOptions` enables total history by default. The other flags are
`virus_info`, `virus_hist`, `tool_info`, `tool_hist`, `transmission`,
`transition`, `reproductive`, `generation`, `active_cases`, `outbreak_size`,
and `hospitalizations`. Disabled tables are empty. Each table has the same
columns and row order as its existing CSV counterpart, including sparse
transition and hospitalization rows. Strings are stored as strings, without
CSV quoting; sequence columns contain the existing sequence writer's output.

`DataBase::get_run_outputs(options)` extracts a single completed run.
`Saver::extract(sim_id, model)` additionally fills each table's `sim_id`
column. Extract only while that model is idle; other threads may extract from
their own models concurrently.

`SaverMemory::results()` returns a concatenated copy ordered by simulation ID,
regardless of completion order. Starting another `run_multiple` with the same
saver clears its previous runs. Bindings that schedule runs themselves can
call `begin(n)`, `extract(id, model)`, `write(id, std::move(out))`, and `end()`.

`SaverCallback<>(options, callback)` sends each `RunOutputs&&` to a custom sink.
Subclass it to override `begin(size_t)` and `end()` if needed. Writes and
callbacks are serialized under OpenMP; extraction happens outside the critical
section. A sink exception is rethrown on the calling thread after workers join.
`end()` signifies successful completion and is not called after a failure.

`SaverFiles<>(format, options)` writes the existing space-delimited CSV format.
The format contains one printf integer placeholder (for example,
`"simulation-%03lu"`); incompatible placeholders are rejected. The legacy
`make_save_run(format, bools...)` is a wrapper around this strategy.

In debug builds, files retain their diagnostic thread column. In-memory tables
omit that diagnostic column, so data does not depend on the number of threads.
