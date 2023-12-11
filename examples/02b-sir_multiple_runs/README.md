## Example: 02b-sir_multiple_runs

Output from the program:

```
Starting multiple runs (100)
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
Days (duration)     : 60 (of 60)
Number of viruses   : 1
Last run elapsed t  : 5.00ms
Total elapsed t     : 580.00ms (100 runs)
Last run speed      : 104.73 million agents x day / second
Average run speed   : 103.36 million agents x day / second
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

Distribution of the population at time 60:
  - (0) Susceptible :  9900 -> 1075
  - (1) Infected    :   100 -> 152
  - (2) Recovered   :     0 -> 8773

Transition Probabilities:
 - Susceptible  0.96  0.04  0.00
 - Infected     0.00  0.70  0.30
 - Recovered    0.00  0.00  1.00

last run elapsed time : 5.00ms
total elapsed time    : 580.00ms
total runs            : 100
mean run elapsed time : 5.80ms
```
