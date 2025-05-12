## Example: 06b-sir-omp

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
Last run elapsed t  : 5.00ms
Total elapsed t     : 144.00ms (100 runs)
Last run speed      : 1718.51 million agents x day / second
Average run speed   : 6939.63 million agents x day / second
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
  - (0) Susceptible :  99000 -> 99000
  - (1) Infected    :   1000 -> 0
  - (2) Recovered   :      0 -> 1000

Transition Probabilities:
 - Susceptible  1.00  0.00     -
 - Infected        -  0.69  0.31
 - Recovered       -     -  1.00

```
