## Example: `16-sbm`

Output from the program:

```
_________________________________________________________________________
|Running the model...
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
|________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : Susceptible-Infected-Recovered (SIR)
Population size     : 10000
Agents' data        : (none)
Number of entities  : 0
Days (duration)     : 50 (of 50)
Number of viruses   : 1
Last run elapsed t  : 151.00ms
Last run speed      : 3.29 million agents x day / second
Rewiring            : off
Last seed used      : 123

Global events:
 (none)

Virus(es):
 - a virus

Tool(s):
 (none)

Model parameters:
 - Recovery rate     : 0.5000
 - Transmission rate : 0.3000

Distribution of the population at time 50:
  - (0) Susceptible :  9900 -> 180
  - (1) Infected    :   100 -> 0
  - (2) Recovered   :     0 -> 9820

Transition Probabilities:
 - Susceptible  0.83  0.17     -
 - Infected        -  0.50  0.50
 - Recovered       -     -  1.00

```
