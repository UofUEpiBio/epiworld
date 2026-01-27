## Example: `06b-sir-omp`

Output from the program:

```
Starting multiple runs (100) using 4 thread(s)
_________________________________________________________________________
_________________________________________________________________________
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : Susceptible-Infected-Recovered (SIR)
Population size     : 100000
Agents' data        : (none)
Number of entities  : 0
Days (duration)     : 100 (of 100)
Number of viruses   : 1
Last run elapsed t  : 0.00s
Total elapsed t     : 14.00s (100 runs)
Last run speed      : 17.84 million agents x day / second
Average run speed   : 71.11 million agents x day / second
Rewiring            : off

Global events:
 (none)

Virus(es):
 - a virus

Tool(s):
 (none)

Model parameters:
 - Recovery rate     : 0.3000
 - Transmission rate : 0.9000

Distribution of the population at time 100:
  - (0) Susceptible :  99000 -> 123
  - (1) Infected    :   1000 -> 14
  - (2) Recovered   :      0 -> 99863

Transition Probabilities:
 - Susceptible  0.94  0.06     -
 - Infected        -  0.70  0.30
 - Recovered       -     -  1.00

```
