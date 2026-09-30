# Transmission Benchmark

Times the network transmission step in each transmission mode (`"auto"`, `"push"`, and `"pull"`; see [Push and Pull Transmission](https://UofUEpi.github.io/epiworld/impl/transmission-sampling/)) on these network models:

- **A**: the SEIRH model of the [epiworld-benchmark](https://github.com/UofUEpiBio/epiworld-benchmark) study. It runs on a Watts–Strogatz network with mean degree 10, $R_0 = 2$ and 100 initial cases, and is built exactly as that study's epiworldR runner builds it. Its susceptibles ignore exposed, hospitalized and recovered neighbors, and agents keep the virus after recovery.
- **B**: `ModelSEIR` on the same network (a large outbreak).
- **C**: a dense (mean degree 50), high-prevalence `ModelSIR`, where pulling is the cheaper step near the peak.
- **D** (not run by default; meant for large populations): a measles-like `ModelSEIR`. It is highly transmissible (0.3 per contact-day on a mean-degree-10 network), has a 10-day latent period during which agents do not transmit, and starts from 10 cases. With 1,000,000 agents it infects about 2% of the population by day 60, 30% by day 90, and nearly everyone by day 120:

  ```sh
  ./main --sizes 1000000 --reps 10 --scenarios D --days 60
  ```

- **E** (not run by default; meant for 165,000 agents): a heterogeneous `ModelSEIR` built like a collapsed activity-based population (e.g., GeoPops): household cliques of 1 to 6 agents plus a heavy-tailed workplace/school layer, mean degree about 4.4, 100 initial cases, latent agents that do not transmit. About 60% of the agents are infected within 100 days. It is the case where the fixed cost of visiting an agent matters as much as scanning its ties, and where the automatic choice used to pull on a third of the days around the peak:

  ```sh
  ./main --sizes 165000 --reps 10 --scenarios E
  ```

For each cell, the program prints:

- the median (Q1, Q3) CPU milliseconds per 100-day `run()`;
- the median final size;
- how many steps pushed and pulled;
- a checksum of every replicate's daily counts and transmissions.

Equal checksums mean bit-identical runs, e.g., `"pull"` against epiworld 0.15, or queuing on against off. The file also compiles against versions without transmission modes (which always pull), so two versions can be timed side by side:

```sh
./main --sizes 10000,100000 --reps 100 --scenarios A,B,C --modes auto,push,pull
```

Other options are `--days` and `--queuing on|off`. The defaults are small (10,000 agents, five replicates) so that the example runs quickly.

## Per-day trace

`--trace` runs one replicate per cell and prints one line per day instead of the summary: the mode used (`P` for push, `L` for pull), the four sums `"auto"` compares as they stood when the step began (carrier degree, susceptible degree, carriers, susceptibles), and the CPU microseconds since the previous day. Tracing `push` and `pull` on the same scenario gives each mode's cost on the same days, which is how the cost model in [Push and Pull Transmission](https://UofUEpi.github.io/epiworld/impl/transmission-sampling/) was fitted:

```sh
./main --sizes 165000 --scenarios E --modes push --trace
./main --sizes 165000 --scenarios E --modes pull --trace
```
