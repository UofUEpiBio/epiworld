## Example: 02b-sir_multiple_runs

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
Last run elapsed t  : 0.00ms
Total elapsed t     : 32.00ms (100 runs)
Last run speed      : 1796.41 million agents x day / second
Average run speed   : 1824.87 million agents x day / second
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
  - (0) Susceptible :  9900 -> 9900
  - (1) Infected    :   100 -> 0
  - (2) Recovered   :     0 -> 100

Transition Probabilities:
 - Susceptible  1.00  0.00     -
 - Infected        -  0.72  0.28
 - Recovered       -     -  1.00

last run elapsed time : 0.00ms
total elapsed time    : 32.00ms
total runs            : 100
mean run elapsed time : 0.32ms
```
