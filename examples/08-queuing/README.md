## Example: 08-queuing

Output from the program:

```

________________________________________________________________________________
DEFAULT STATUS CODES

  0 = susceptible (S)             *
  6 = recovered (S)               *
  1 = latent (I)                  *
  2 = symptomatic (I)            
  3 = symptomatic isolated (I)   
  4 = asymptomatic (I)           
  5 = asymptomatic isolated (I)  
  7 = removed (R)                 *

(S): Susceptible, (I): Infected, (R): Recovered
 * : Baseline status (default)
________________________________________________________________________________

Running the model...
_________________________________________________________________________
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.

________________________________________________________________________________
SIMULATION STUDY

Population size    : 20000
Days (duration)    : 100 (of 100)
Number of variants : 1
Last run elapsed t : 174.00ms
Rewiring           : off

Virus(es):
 - Covid19 (baseline prevalence: 50 seeds)
Tool(s):
 - Vaccine (baseline prevalence: 25.00%)

Model parameters:
 - Infect period         : 6.0000
 - Latent period         : 3.0000
 - Prob of symptoms      : 0.6000
 - Prob of transmission  : 1.0000
 - Prob. death           : 0.0010
 - Prob. reinfect        : 0.1000
 - Surveilance prob.     : 0.0010
 - Vax efficacy          : 0.9000
 - Vax redux transmision : 0.5000

Distribution of the population at time 100:
 - Total susceptible (S)           :   19950 -> 14779
 - Total recovered (S)             :       0 -> 823
 - Total latent (I)                :      50 -> 1343
 - Total symptomatic (I)           :       0 -> 1711
 - Total symptomatic isolated (I)  :       0 -> 16
 - Total asymptomatic (I)          :       0 -> 1176
 - Total asymptomatic isolated (I) :       0 -> 10
 - Total removed (R)               :       0 -> 142

(S): Susceptible, (I): Infected, (R): Recovered
________________________________________________________________________________

Running the model...
_________________________________________________________________________
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.

________________________________________________________________________________
SIMULATION STUDY

Population size    : 20000
Days (duration)    : 100 (of 100)
Number of variants : 1
Last run elapsed t : 87.00ms
Rewiring           : off

Virus(es):
 - Covid19 (baseline prevalence: 50 seeds)
Tool(s):
 - Vaccine (baseline prevalence: 25.00%)

Model parameters:
 - Infect period         : 6.0000
 - Latent period         : 3.0000
 - Prob of symptoms      : 0.6000
 - Prob of transmission  : 1.0000
 - Prob. death           : 0.0010
 - Prob. reinfect        : 0.1000
 - Surveilance prob.     : 0.0010
 - Vax efficacy          : 0.9000
 - Vax redux transmision : 0.5000

Distribution of the population at time 100:
 - Total susceptible (S)           :   19950 -> 14779
 - Total recovered (S)             :       0 -> 823
 - Total latent (I)                :      50 -> 1343
 - Total symptomatic (I)           :       0 -> 1711
 - Total symptomatic isolated (I)  :       0 -> 16
 - Total asymptomatic (I)          :       0 -> 1176
 - Total asymptomatic isolated (I) :       0 -> 10
 - Total removed (R)               :       0 -> 142

(S): Susceptible, (I): Infected, (R): Recovered
________________________________________________________________________________

```
