## Example: `10-likelihood-free-mcmc`

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
Last run elapsed t  : 1.00ms
Last run speed      : 26.55 million agents x day / second
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
  - (0) Susceptible :  900 -> 697
  - (1) Infected    :  100 -> 0
  - (2) Recovered   :    0 -> 303

Transition Probabilities:
 - Susceptible  0.99  0.01     -
 - Infected        -  0.71  0.29
 - Recovered       -     -  1.00

_________________________________________________________________________
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||___________________________________________

LIKELIHOOD-FREE MARKOV CHAIN MONTE CARLO

N Samples (total) : 2000
N Samples (after burn-in period) : 2000
Elapsed t : 6.00s

Parameters:
  -Immune recovery :  0.56 [ 0.34,  0.77] (initial :  0.50)
  -Infectiousness  :  0.19 [ 0.15,  0.63] (initial :  0.50)

Statistics:
  -Susceptible :  664.87 [   4.00,  707.00] (Observed:  697.00)
  -Infected    :    2.26 [   0.00,    0.00] (Observed:    0.00)
  -Recovered   :  332.38 [ 293.00,  986.00] (Observed:  303.00)
___________________________________________

```
