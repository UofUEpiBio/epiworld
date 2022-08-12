## Example: 01-sir

Output from the program:

```
Running the model...
_________________________________________________________________________
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.

________________________________________________________________________________
SIMULATION STUDY

Population size     : 10000
Number of entitites : 0
Days (duration)     : 100 (of 100)
Number of variants  : 1
Last run elapsed t  : 24.00ms
Rewiring            : off

Virus(es):
 - a virus (baseline prevalence: 1.00%)

Tool(s):
 - Immunity (a virus) (baseline prevalence: 0 seeds)
 - Immune system (baseline prevalence: 100.00%)

Model parameters:
 - Immune recovery        : 0.5000
 - Immune suscept. redux. : 0.5000
 - Infectiousness         : 1.0000
 - Post immunity          : 0.9500

Distribution of the population at time 100:
 - (0) Susceptible     :  9900 -> 18
 - (1) Infected        :   100 -> 0
 - (2) Recovered       :     0 -> 4927
 - (3) Gained Immunity :     0 -> 5055

Transition Probabilities:
 - Susceptible      0.94  0.06  0.00  0.00
 - Infected         0.00  0.43  0.29  0.28
 - Recovered        0.00  0.00  1.00  0.00
 - Gained Immunity  0.00  0.00  0.00  1.00

```
