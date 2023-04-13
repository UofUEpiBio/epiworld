## Example: 06b-sir-omp

Output from the program:

```
Starting multiple runs (20) using 2 threads
_________________________________________________________________________
_________________________________________________________________________
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
 done.
Elapsed time: 3887 milliseconds

________________________________________________________________________________
SIMULATION STUDY

Name of the model   : Susceptible-Infected-Recovered (SIR)
Population size     : 100000
Number of entities  : 0
Days (duration)     : 100 (of 100)
Number of variants  : 1
Last run elapsed t  : 0.00s
Total elapsed t     : 3.00s (20 runs)
Last run speed      : 31.53 million agents x day / second
Average run speed   : 63.60 million agents x day / second
Rewiring            : off

Virus(es):
 - a virus (baseline prevalence: 1.00%)

Tool(s):
 (none)

Model parameters:
 - Infectiousness    : 0.9000
 - Prob. of Recovery : 0.3000

Distribution of the population at time 100:
 - (0) Susceptible :  99000 -> 3837
 - (1) Infected    :   1000 -> 338
 - (2) Recovered   :      0 -> 95825

Transition Probabilities:
 - Susceptible  0.97  0.03  0.00
 - Infected     0.00  0.70  0.30
 - Recovered    0.00  0.00  1.00

```
