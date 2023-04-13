## Example: 01-sir

Output from the program:

```
Running the model...
_________________________________________________________________________
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.

________________________________________________________________________________
SIMULATION STUDY

Population size     : 10000
Number of entities  : 0
Days (duration)     : 100 (of 100)
Number of variants  : 1
Last run elapsed t  : 17.00ms
Rewiring            : off

Virus(es):
 - a virus (baseline prevalence: 1.00%)

Tool(s):
 (none)

Model parameters:
 - Infectiousness    : 0.9000
 - Prob. of Recovery : 0.5000

Distribution of the population at time 100:
 - (0) Susceptible :  9900 -> 0
 - (1) Infected    :   100 -> 0
 - (2) Recovered   :     0 -> 10000

Transition Probabilities:
 - Susceptible  0.89  0.11  0.00
 - Infected     0.00  0.50  0.50
 - Recovered    0.00  0.00  1.00

```
