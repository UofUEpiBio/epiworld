## Example: 06b-sir-omp

Output from the program:

```
Starting multiple runs (20) using 2 thread(s)
_________________________________________________________________________
_________________________________________________________________________
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
 done.
________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : Susceptible-Infected-Recovered (SIR)
Population size     : 10000
Agents' data        : (none)
Number of entities  : 0
Days (duration)     : 100 (of 100)
Number of viruses   : 1
Last run elapsed t  : 10.00ms
Total elapsed t     : 105.00ms (20 runs)
Last run speed      : 99.18 million agents x day / second
Average run speed   : 189.38 million agents x day / second
Rewiring            : off

Global events:
 (none)

Virus(es):
 - a virus (baseline prevalence: 1.00%)

Tool(s):
 (none)

Model parameters:
 - Recovery rate     : 0.3000
 - Transmission rate : 0.9000

Distribution of the population at time 100:
  - (0) Susceptible :  9900 -> 226
  - (1) Infected    :   100 -> 28
  - (2) Recovered   :     0 -> 9746

Transition Probabilities:
 - Susceptible  0.96  0.04  0.00
 - Infected     0.00  0.71  0.29
 - Recovered    0.00  0.00  1.00

```
