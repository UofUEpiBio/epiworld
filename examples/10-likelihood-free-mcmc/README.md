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
Last run elapsed t  : 203.00µs
Last run speed      : 246.31 million agents x day / second
Rewiring            : off

Global events:
 (none)

Virus(es):
 - covid

Tool(s):
 (none)

Model parameters:
 - Recovery rate     : 0.3000
 - Transmission rate : 0.1000

Distribution of the population at time 50:
  - (0) Susceptible :  900 -> 706
  - (1) Infected    :  100 -> 0
  - (2) Recovered   :    0 -> 294

Transition Probabilities:
 - Susceptible  0.99  0.01     -
 - Infected        -  0.70  0.30
 - Recovered       -     -  1.00

_________________________________________________________________________
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||___________________________________________

LIKELIHOOD-FREE MARKOV CHAIN MONTE CARLO

N Samples (total) : 2000
N Samples (after burn-in period) : 2000
Elapsed t : 699.00ms

Parameters:
  -Immune recovery :  0.74 [ 0.22,  0.97] (initial :  0.50)
  -Infectiousness  :  0.22 [ 0.09,  0.40] (initial :  0.50)

Statistics:
  -Susceptible :  687.06 [ 219.00,  708.00] (Observed:  706.00)
  -Infected    :    1.20 [   0.00,    0.00] (Observed:    0.00)
  -Recovered   :  311.26 [ 292.00,  753.00] (Observed:  294.00)
___________________________________________

```
