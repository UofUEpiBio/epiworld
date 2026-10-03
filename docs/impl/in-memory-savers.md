# In-memory Savers

`run_multiple` can pass the outputs of each simulation to a `Saver` instead of
a callback. Savers keep results in memory, so language bindings (R, Python,
WebAssembly) get them without writing and parsing temporary files.

```cpp
epiworld::epimodels::ModelSEIRCONN<> model(
    "virus", 1000, 0.01, 4.0, 0.1, 3.0, 1.0 / 7.0);
model.verbose_off();

epiworld::SaveOptions options; // total_hist only, by default
options.transition = true;

epiworld::SaverMemory<> saver(options);
model.run_multiple(100, 10, 42, saver, true, false, 4);

auto results = saver.results();
// results.total_hist: sim_id, date, nviruses, state, counts
// results.transition: sim_id, date, from, to, counts
```

## Outputs

`SaveOptions` selects the outputs: `total_hist`, `virus_info`, `virus_hist`,
`tool_info`, `tool_hist`, `transmission`, `transition`, `reproductive`,
`generation`, `active_cases`, `outbreak_size`, and `hospitalizations`.

`RunOutputs` holds one table per output (`RunOutputs::TotalHist`,
`RunOutputs::Transition`, ...), stored column by column. Each table has the
columns and rows of the matching file from `DataBase::write_data()`, which
writes those same tables, plus a `sim_id` column. Unselected tables are
empty. `DataBase::get_run_outputs(options)` extracts the tables of the last
simulation.

## Savers

- `SaverMemory` keeps each simulation in its own slot, allocated by
  `begin()`. `results()` concatenates them in simulation order, whatever
  order they finished in; `take_results()` does the same while freeing the
  saver's copy. Running `run_multiple` again with the same saver clears it.
- `SaverFiles(format, options)` writes the files of `write_data()`, named
  `<prefix>_<output>.csv`. The prefix is `format` with its one integer
  placeholder replaced by the simulation ID (e.g., `"%03lu-episimulation"`).
  The legacy `make_save_run()` is a wrapper around it.
- `SaverCallback(options, callback)` passes each `RunOutputs` to a function.
  Subclass it to override `begin(nexperiments)` and `end()`.

## Threads

Under OpenMP, each thread calls `extract()` and `write()` for its own
simulations, without locks. So `write()` may run concurrently for different
simulation IDs, and a saver must only touch state that belongs to that ID
(`SaverMemory` writes its own slot, `SaverFiles` its own files) or
synchronize internally (`SaverCallback` runs one callback at a time).

If a saver throws, the other threads stop at their next simulation, and
`run_multiple` rethrows the exception. `end()` is not called after a failure.

Bindings that schedule simulations themselves (e.g., across Web Workers) can
call `begin(n)`, `extract(sim_id, model)`, `write(sim_id, outputs)`, and
`end()` directly.

In debug builds (`EPI_DEBUG`), files have an extra first column with the
thread that wrote each row. Tables in memory do not, so they do not depend on
the number of threads.
