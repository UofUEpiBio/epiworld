## Example: 00-hello-world

Output from the program:

```
Running the model...
_________________________________________________________________________
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
[epiworld-debug] DEBUGGING ON (compiled with EPI_DEBUG defined)

________________________________________________________________________________
SIMULATION STUDY

Population size     : 10000
Number of entities  : 0
Days (duration)     : 100 (of 100)
Number of variants  : 1
Last run elapsed t  : 40.00ms
Rewiring            : off

Virus(es):
 - covid 19 (baseline prevalence: 50 seeds)

Tool(s):
 - vaccine (baseline prevalence: 50.00%)
 - Immunity (covid 19) (originated in the model...)

Model parameters:
 (none)

Distribution of the population at time 100:
 - (0) Susceptible :  9950 -> 70
 - (1) Exposed     :    50 -> 70
 - (2) Recovered   :     0 -> 9271
 - (3) Removed     :     0 -> 589

Transition Probabilities:
 - Susceptible  0.95  0.05  0.00  0.00
 - Exposed      0.00  0.85  0.14  0.01
 - Recovered    0.00  0.00  1.00  0.00
 - Removed      0.00  0.00  0.00  1.00

```
