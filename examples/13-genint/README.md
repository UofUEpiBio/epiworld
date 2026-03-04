## Example: `13-genint`

Output from the program:

```
_________________________________________________________________________
Running the model...
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : Susceptible-Infected-Removed (SIR) (connected)
Population size     : 100000
Agents' data        : (none)
Number of entities  : 0
Days (duration)     : 200 (of 200)
Number of viruses   : 1
Last run elapsed t  : 215.00ms
Last run speed      : 92.66 million agents x day / second
Rewiring            : off

Global events:
 - Update infected individuals (runs daily)

Virus(es):
 - avirus

Tool(s):
 (none)

Model parameters:
 - Contact rate      : 10.0000
 - Recovery rate     : 0.1429
 - Transmission rate : 0.1000

Distribution of the population at time 200:
  - (0) Susceptible :  99950 -> 219
  - (1) Infected    :     50 -> 0
  - (2) Recovered   :      0 -> 99781

Transition Probabilities:
 - Susceptible  0.93  0.07     -
 - Infected        -  0.86  0.14
 - Recovered       -     -  1.00

_________________________________________________________________________
Running the model...
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : Susceptible-Exposed-Infected-Removed (SEIR) (connected)
Population size     : 100000
Agents' data        : (none)
Number of entities  : 0
Days (duration)     : 200 (of 200)
Number of viruses   : 1
Last run elapsed t  : 182.00ms
Last run speed      : 109.39 million agents x day / second
Rewiring            : off

Global events:
 - Update infected individuals (runs daily)

Virus(es):
 - avirus

Tool(s):
 (none)

Model parameters:
 - Avg. Incubation days : 2.0000
 - Contact rate         : 10.0000
 - Prob. Recovery       : 0.1429
 - Prob. Transmission   : 0.1000

Distribution of the population at time 200:
  - (0) Susceptible :  99950 -> 174
  - (1) Exposed     :     50 -> 0
  - (2) Infected    :      0 -> 0
  - (3) Recovered   :      0 -> 99826

Transition Probabilities:
 - Susceptible  0.96  0.04     -     -
 - Exposed         -  0.50  0.50     -
 - Infected        -     -  0.86  0.14
 - Recovered       -     -     -  1.00

SIR Gen. Int. (obs)       : 1.83934
SIR Gen. Int. (expected)  : 2.56084
SEIR Gen. Int. (obs)      : 3.7322
SEIR Gen. Int. (expected) : 4.49663
```
