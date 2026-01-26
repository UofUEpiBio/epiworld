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
Last run elapsed t  : 4.00ms
Last run speed      : 21.75 million agents x day / second
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
  - (0) Susceptible             :  990 -> 1000
  - (1) Infected                :   10 -> 0
  - (2) Infected (hospitalized) :    0 -> 0

Transition Probabilities:
 - Susceptible              1.00  0.00  0.00
 - Infected                 0.26  0.69  0.05
 - Infected (hospitalized)  0.39     -  0.61

```
