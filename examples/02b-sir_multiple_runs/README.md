## Example: `02b-sir_multiple_runs`

Output from the program:

```
Starting multiple runs (100) using 1 thread(s)
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
Last run elapsed t  : 12.00ms
Total elapsed t     : 954.00ms (100 runs)
Last run speed      : 49.84 million agents x day / second
Average run speed   : 62.84 million agents x day / second
Rewiring            : off
Last seed used      : 2103614207

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
  - (0) Susceptible :  9900 -> 2
  - (1) Infected    :   100 -> 2
  - (2) Recovered   :     0 -> 9996

Transition Probabilities:
 - Susceptible  0.94  0.06     -
 - Infected        -  0.70  0.30
 - Recovered       -     -  1.00

last run elapsed time : 12.00ms
total elapsed time    : 954.00ms
total runs            : 100
mean run elapsed time : 9.54ms
```
