## Example: 07-surveillance

Output from the program:

```
Running the model...
_________________________________________________________________________
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.

________________________________________________________________________________
SIMULATION STUDY

Population size    : 10000
Days (duration)    : 100 (of 100)
Number of variants : 1
Last run elapsed t : 83.00ms
Rewiring           : off

Virus(es):
 - Covid19 (baseline prevalence: 10 seeds)
Tool(s):
 - Vaccine (baseline prevalence: 25.00%)

Model parameters:
 - Infect period         : 12.0000
 - Latent period         : 3.0000
 - Prob of symptoms      : 0.7000
 - Prob of transmission  : 1.0000
 - Prob. death           : 0.0010
 - Prob. reinfect        : 0.1000
 - Surveilance prob.     : 0.0010
 - Vax efficacy          : 0.9000
 - Vax redux transmision : 0.5000

Distribution of the population at time 100:
 - Total susceptible (S)           :    9990 -> 8137
 - Total recovered (S)             :       0 -> 1529
 - Total latent (I)                :      10 -> 70
 - Total symptomatic (I)           :       0 -> 155
 - Total symptomatic isolated (I)  :       0 -> 1
 - Total asymptomatic (I)          :       0 -> 83
 - Total asymptomatic isolated (I) :       0 -> 0
 - Total removed (R)               :       0 -> 25

(S): Susceptible, (I): Infected, (R): Recovered
________________________________________________________________________________

```
