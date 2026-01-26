## Example: `01-sis`

Output from the program:

```
_________________________________________________________________________
Running the model...
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : Susceptible-Infected-Susceptible (SIS)
Population size     : 10000
Agents' data        : (none)
Number of entities  : 0
Days (duration)     : 100 (of 100)
Number of viruses   : 1
Last run elapsed t  : 976.00ms
Last run speed      : 1.02 million agents x day / second
Rewiring            : off

Global events:
 (none)

Virus(es):
 - a virus

Tool(s):
 (none)

Model parameters:
 - Recovery rate     : 0.5000
 - Transmission rate : 0.9000

Distribution of the population at time 100:
  - (0) Susceptible :  9900 -> 3641
  - (1) Infected    :   100 -> 6359

Transition Probabilities:
 - Susceptible  0.49  0.51
 - Infected     0.50  0.50

```
