## Example: `01-seir`

Output from the program:

```
_________________________________________________________________________
|Running the model...
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
|________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : Susceptible-Exposed-Infected-Removed (SEIR)
Population size     : 1000000
Agents' data        : (none)
Number of entities  : 0
Days (duration)     : 50 (of 50)
Number of viruses   : 1
Last run elapsed t  : 6.00s
Last run speed      : 8.06 million agents x day / second
Rewiring            : off

Global events:
 (none)

Virus(es):
 - a virus

Tool(s):
 (none)

Model parameters:
 - Incubation days   : 7.0000
 - Recovery rate     : 0.5000
 - Transmission rate : 0.9000

Distribution of the population at time 50:
  - (0) Susceptible :  990000 -> 118167
  - (1) Exposed     :   10000 -> 51285
  - (2) Infected    :       0 -> 15830
  - (3) Removed     :       0 -> 814718

Transition Probabilities:
 - Susceptible  0.96  0.04     -     -
 - Exposed         -  0.86  0.14     -
 - Infected        -     -  0.50  0.50
 - Removed         -     -     -  1.00

```
