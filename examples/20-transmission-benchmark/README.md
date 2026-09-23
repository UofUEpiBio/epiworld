# Transmission Benchmark

Times the network transmission step in each transmission mode (`"auto"`, `"push"`, and `"pull"`; see [Push and Pull Transmission](https://UofUEpi.github.io/epiworld/impl/transmission-sampling/)) on three network models:

- **A**: the SEIRH model of the [epiworld-benchmark](https://github.com/UofUEpiBio/epiworld-benchmark) study. It runs on a Watts–Strogatz network with mean degree 10, $R_0 = 2$ and 100 initial cases, and is built exactly as that study's epiworldR runner builds it. Its susceptibles ignore exposed, hospitalized and recovered neighbors, and agents keep the virus after recovery.
- **B**: `ModelSEIR` on the same network (a large outbreak).
- **C**: a dense (mean degree 50), high-prevalence `ModelSIR`, where pulling is the cheaper step near the peak.
- **D** (not run by default; meant for large populations): a measles-like `ModelSEIR`. It is highly transmissible (0.3 per contact-day on a mean-degree-10 network), has a 10-day latent period during which agents do not transmit, and starts from 10 cases. With 1,000,000 agents it infects about 2% of the population by day 60, 30% by day 90, and nearly everyone by day 120:

  ```sh
  ./main --sizes 1000000 --reps 10 --scenarios D --days 60
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
