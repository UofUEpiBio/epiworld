## Example: 01-sir

Output from the program:

```
Running the model...
DEBUGGING ON (compiled with EPI_DEBUG defined)
________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : Susceptible-Infected-Recovered (SIR)
Population size     : 50000
Agents' data        : (none)
Number of entities  : 0
Days (duration)     : 50 (of 50)
Number of viruses   : 1
Last run elapsed t  : 70.00ms
Last run speed      : 35.62 million agents x day / second
Rewiring            : off

Global actions:
 (none)

Virus(es):
 - a virus (baseline prevalence: 1.00%)

Tool(s):
 (none)

Model parameters:
 - Recovery rate     : 0.5000
 - Transmission rate : 0.9000

Distribution of the population at time 50:
  - (0) Susceptible : 49500 -> 0
  - (1) Infected    :   500 -> 0
  - (2) Recovered   :     0 -> 50000

Transition Probabilities:
 - Susceptible  0.81  0.19  0.00
 - Infected     0.00  0.49  0.51
 - Recovered    0.00  0.00  1.00

```
