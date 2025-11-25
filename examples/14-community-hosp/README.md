## Example: `14-community-hosp`

Output from the program:

```
_________________________________________________________________________
Running the model...
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : (none)
Population size     : 1000
Agents' data        : (none)
Number of entities  : 0
Days (duration)     : 100 (of 100)
Number of viruses   : 1
Last run elapsed t  : 19.00ms
Last run speed      : 5.19 million agents x day / second
Rewiring            : off

Global events:
 (none)

Virus(es):
 - MRSA

Tool(s):
 (none)

Model parameters:
 - Discharge infected   : 0.1000
 - Prob hospitalization : 0.1000
 - Prob recovery        : 0.3300

Distribution of the population at time 100:
  - (0) Susceptible             :  990 -> 937
  - (1) Infected                :   10 -> 49
  - (2) Infected (hospitalized) :    0 -> 14

Transition Probabilities:
 - Susceptible              0.98  0.02  0.00
 - Infected                 0.32  0.62  0.07
 - Infected (hospitalized)  0.35  0.07  0.58

```
