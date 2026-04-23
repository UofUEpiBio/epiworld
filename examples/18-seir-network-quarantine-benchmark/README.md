## Example: `18-seir-network-quarantine-benchmark`

Output from the program:

```
=== Benchmark: SEIRMixingQuarantine vs SEIRNetworkQuarantine ===
Population: 20000 agents in 20 groups of 1000
Mean contacts: 50.0, Days: 100, Reps: 50

SEIRMixingQuarantine:  avg time per run = 218.03 ms
________________________________________________________________________________
SEIR with Mixing and Quarantine
It features 20000 agents, 1 virus(es), and 0 tool(s).
The model has 9 states.
The final distribution is: 19998 Susceptible, 0 Exposed, 0 Infected, 0 Isolated, 0 Quarantined Susceptible, 0 Quarantined Exposed, 0 Isolated Recovered, 0 Hospitalized, and 2 Recovered.

SEIRNetworkQuarantine: avg time per run = 44.95 ms
________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : SEIR with Network and Quarantine
Population size     : 20000
Agents' data        : (none)
Number of entities  : 0
Days (duration)     : 100 (of 100)
Number of viruses   : 1
Last run elapsed t  : 0.00s
Total elapsed t     : 1.00s (51 runs)
Last run speed      : 140.99 million agents x day / second
Average run speed   : 53.30 million agents x day / second
Rewiring            : off
Last seed used      : 50

Global events:
 - Quarantine process (runs daily)

Virus(es):
 - Flu

Tool(s):
 (none)

Model parameters:
 - Avg. Incubation days         : 7.0000
 - Contact tracing days prior   : 4.0000
 - Contact tracing success rate : 1.0000
 - Days undetected              : 3.0000
 - Hospitalization period       : 7.0000
 - Hospitalization rate         : 0.0500
 - Isolation period             : 10.0000
 - Isolation willingness        : 0.9000
 - Prob. Recovery               : 0.2000
 - Prob. Transmission           : 0.0250
 - Quarantine period            : 7.0000
 - Quarantine willingness       : 0.8000

Distribution of the population at time 100:
  - ( 0) Susceptible             : 19999 -> 18815
  - ( 1) Exposed                 :     1 -> 13
  - ( 2) Infected                :     0 -> 3
  - ( 3) Isolated                :     0 -> 7
  - ( 4) Detected Hospitalized   :     0 -> 7
  - ( 5) Quarantined Susceptible :     0 -> 1037
  - ( 6) Quarantined Exposed     :     0 -> 17
  - ( 7) Isolated Recovered      :     0 -> 23
  - ( 8) Hospitalized            :     0 -> 0
  - ( 9) Recovered               :     0 -> 78

Transition Probabilities:
 - Susceptible              1.00  0.00     -     -     -  0.00  0.00     -     -     -
 - Exposed                     -  0.83  0.13     -     -     -  0.04     -     -     -
 - Infected                    -     -  0.49  0.20  0.02     -     -  0.07  0.03  0.19
 - Isolated                    -     -  0.01  0.72  0.06     -     -  0.20     -  0.00
 - Detected Hospitalized       -     -     -     -  0.92     -     -     -     -  0.08
 - Quarantined Susceptible  0.03     -     -     -     -  0.97     -     -     -     -
 - Quarantined Exposed         -  0.07  0.01  0.12     -     -  0.80     -     -     -
 - Isolated Recovered          -     -     -     -     -     -     -  0.95     -  0.05
 - Hospitalized                -     -     -     -     -     -     -     -  0.81  0.19
 - Recovered                   -     -     -     -     -     -     -     -     -  1.00


Average outbreak size - Mixing: 1402.10
Average outbreak size - Network: 576.14
```
