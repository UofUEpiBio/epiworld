## Example: 10-likelihood-free-mcmc

Output from the program:

```
________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : Susceptible-Infected-Recovered (SIR)
Population size     : 1000
Agents' data        : (none)
Number of entities  : 0
Days (duration)     : 50 (of 50)
Number of viruses   : 1
Last run elapsed t  : 685.00µs
Last run speed      : 72.99 million agents x day / second
Rewiring            : off

Global actions:
 (none)

Virus(es):
 - covid (baseline prevalence: 10.00%)

Tool(s):
 (none)

Model parameters:
 - Recovery rate     : 0.3000
 - Transmission rate : 0.9000

Distribution of the population at time 50:
  - (0) Susceptible :  900 -> 0
  - (1) Infected    :  100 -> 0
  - (2) Recovered   :    0 -> 1000

Transition Probabilities:
 - Susceptible  0.60  0.40  0.00
 - Infected     0.00  0.70  0.30
 - Recovered    0.00  0.00  1.00

___________________________________________

LIKELIHOOD-FREE MARKOV CHAIN MONTE CARLO

N Samples : 1000
Elapsed t : 663.00ms

Parameters:
  -Immune recovery :  0.58 [ 0.13,  0.96] (initial :  0.50)
  -Infectiousness  :  0.87 [ 0.58,  0.99] (initial :  0.50)

Statistics:
  -Susceptible :     0.32 [    0.00,     2.00] (Observed:     0.00)
  -Infected    :     0.08 [    0.00,     1.00] (Observed:     0.00)
  -Recovered   :   998.61 [  998.00,  1000.00] (Observed:  1000.00)
___________________________________________

```
