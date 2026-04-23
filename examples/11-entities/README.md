## Example: `11-entities`

Output from the program:

```
_________________________________________________________________________
|Running the model...
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
|________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : Susceptible-Exposed-Infected-Removed (SEIR) with Mixing
Population size     : 10000
Agents' data        : (none)
Number of entities  : 3
Days (duration)     : 50 (of 50)
Number of viruses   : 1
Last run elapsed t  : 152.00ms
Last run speed      : 3.28 million agents x day / second
Rewiring            : off
Last seed used      : 123

Global events:
 - Update infected individuals (runs daily)

Virus(es):
 - Flu

Tool(s):
 (none)

Model parameters:
 - Avg. Incubation days : 4.0000
 - Prob. Recovery       : 0.1429
 - Prob. Transmission   : 0.1000

Distribution of the population at time 50:
  - (0) Susceptible :  9900 -> 14
  - (1) Exposed     :   100 -> 4
  - (2) Infected    :     0 -> 260
  - (3) Recovered   :     0 -> 9722

Transition Probabilities:
 - Susceptible  0.95  0.05     -     -
 - Exposed         -  0.75  0.25     -
 - Infected        -     -  0.86  0.14
 - Recovered       -     -     -  1.00

```
