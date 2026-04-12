## Example: `03-simple-sir`

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
Last run elapsed t  : 829.00µs
Last run speed      : 120.63 million agents x day / second
Rewiring            : off
Last seed used      : 123123

Global events:
 (none)

Virus(es):
 - covid 19

Tool(s):
 - vaccine

Model parameters:
 (none)

Distribution of the population at time 100:
  - (0) Susceptible :  950 -> 588
  - (1) Infected    :   50 -> 0
  - (2) Removed     :    0 -> 412

Transition Probabilities:
 - Susceptible  0.99  0.01     -
 - Infected        -  0.85  0.15
 - Removed         -     -  1.00

```
