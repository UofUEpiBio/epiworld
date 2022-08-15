## Example: 10-likelihood-free-mcmc

Output from the program:

```

________________________________________________________________________________
SIMULATION STUDY

Population size     : 1000
Number of entitites : 0
Days (duration)     : 50 (of 50)
Number of variants  : 1
Last run elapsed t  : 1.00ms
Rewiring            : off

Virus(es):
 - covid (baseline prevalence: 10.00%)

Tool(s):
 (none)

Model parameters:
 - Infectiousness    : 0.9000
 - Prob. of Recovery : 0.3000

Distribution of the population at time 50:
 - (0) Susceptible :  900 -> 0
 - (1) Infected    :  100 -> 0
 - (2) Recovered   :    0 -> 1000

Transition Probabilities:
 - Susceptible  0.24  0.75  0.01
 - Infected     0.00  0.83  0.17
 - Recovered    0.00  0.00  1.00

___________________________________________

LIKELIHOOD-FREE MARKOV CHAIN MONTE CARLO

N Samples : 1000
Elapsed t : 1.00s

Parameters:
  -Immune recovery :  0.50 [ 0.15,  0.96] (initial :  0.50)
  -Infectiousness  :  0.70 [ 0.27,  0.99] (initial :  0.50)

Statistics:
  -Susceptible :     0.18 [    0.00,     2.00] (Observed:  0.00)
  -Infected    :     0.03 [    0.00,     1.00] (Observed:  0.00)
  -Recovered   :   994.79 [  998.00,  1000.00] (Observed:  1000.00)
___________________________________________

```
