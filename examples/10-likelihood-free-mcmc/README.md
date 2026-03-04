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
Last run elapsed t  : 148.00µs
Last run speed      : 337.84 million agents x day / second
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
  - (0) Susceptible :  900 -> 738
  - (1) Infected    :  100 -> 0
  - (2) Recovered   :    0 -> 262

Transition Probabilities:
 - Susceptible  0.99  0.01     -
 - Infected        -  0.70  0.30
 - Recovered       -     -  1.00

_________________________________________________________________________
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||___________________________________________

LIKELIHOOD-FREE MARKOV CHAIN MONTE CARLO

N Samples (total) : 2000
N Samples (after burn-in period) : 2000
Elapsed t : 614.00ms

Parameters:
  -Immune recovery :  0.49 [ 0.27,  0.97] (initial :  0.50)
  -Infectiousness  :  0.15 [ 0.09,  0.25] (initial :  0.50)

Statistics:
  -Susceptible :  725.97 [ 734.00,  746.00] (Observed:  738.00)
  -Infected    :    0.47 [   0.00,    0.00] (Observed:    0.00)
  -Recovered   :  273.08 [ 254.00,  266.00] (Observed:  262.00)
___________________________________________

```
