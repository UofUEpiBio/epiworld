## Example: `02b-sir_multiple_runs`

Output from the program:

```
Starting multiple runs (100)
_________________________________________________________________________
_________________________________________________________________________
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : Susceptible-Infected-Recovered (SIR)
Population size     : 10000
Agents' data        : (none)
Number of entities  : 0
Days (duration)     : 60 (of 60)
Number of viruses   : 1
Last run elapsed t  : 0.00s
Total elapsed t     : 3.00s (100 runs)
Last run speed      : 18.84 million agents x day / second
Average run speed   : 18.81 million agents x day / second
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

Distribution of the population at time 60:
  - (0) Susceptible :  9900 -> 0
  - (1) Infected    :   100 -> 5
  - (2) Recovered   :     0 -> 9995

Transition Probabilities:
 - Susceptible  0.94  0.06     -
 - Infected        -  0.70  0.30
 - Recovered       -     -  1.00

last run elapsed time : 0.00s
total elapsed time    : 3.00s
total runs            : 100
mean run elapsed time : 0.03s
```
