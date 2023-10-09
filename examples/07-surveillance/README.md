## Example: 07-surveillance

Output from the program:

```
DEBUGGING ON (compiled with EPI_DEBUG defined)
________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : Surveillance
Population size     : 10000
Agents' data        : (none)
Number of entities  : 0
Days (duration)     : 0 (of 0)
Number of viruses   : 1
Last run elapsed t  : -
Rewiring            : off

Global actions:
 - Surveilance program (runs daily)

Virus(es):
 - Covid19 (baseline prevalence: 10 seeds)

Tool(s):
 - Vaccine (baseline prevalence: 25.00%)

Model parameters:
 - Infect period          : 12.0000
 - Latent period          : 3.0000
 - Prob of symptoms       : 0.7000
 - Prob of transmission   : 1.0000
 - Prob. death            : 0.0010
 - Prob. no reinfect      : 0.1000
 - Surveilance prob.      : 0.0010
 - Vax efficacy           : 0.9000
 - Vax redux transmission : 0.5000
Running the model...
DEBUGGING ON (compiled with EPI_DEBUG defined)
________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : Surveillance
Population size     : 10000
Agents' data        : (none)
Number of entities  : 0
Days (duration)     : 100 (of 100)
Number of viruses   : 1
Last run elapsed t  : 3.00ms
Last run speed      : 252.72 million agents x day / second
Rewiring            : off

Global actions:
 - Surveilance program (runs daily)

Virus(es):
 - Covid19 (baseline prevalence: 10 seeds)

Tool(s):
 - Vaccine (baseline prevalence: 25.00%)

Model parameters:
 - Infect period          : 12.0000
 - Latent period          : 3.0000
 - Prob of symptoms       : 0.7000
 - Prob of transmission   : 1.0000
 - Prob. death            : 0.0010
 - Prob. no reinfect      : 0.1000
 - Surveilance prob.      : 0.0010
 - Vax efficacy           : 0.9000
 - Vax redux transmission : 0.5000

Distribution of the population at time 100:
  - (0) Susceptible           :  9990 -> 9484
  - (1) Latent                :    10 -> 13
  - (2) Symptomatic           :     0 -> 23
  - (3) Symptomatic isolated  :     0 -> 1
  - (4) Asymptomatic          :     0 -> 14
  - (5) Asymptomatic isolated :     0 -> 0
  - (6) Recovered             :     0 -> 457
  - (7) Removed               :     0 -> 8

Transition Probabilities:
 - Susceptible            1.00  0.00  0.00  0.00  0.00  0.00  0.00  0.00
 - Latent                 0.00  0.74  0.17  0.00  0.09  0.00  0.00  0.00
 - Symptomatic            0.00  0.00  0.91  0.00  0.00  0.00  0.09  0.00
 - Symptomatic isolated   0.00  0.00  0.00  0.87  0.00  0.00  0.13  0.00
 - Asymptomatic           0.00  0.00  0.00  0.00  0.90  0.00  0.09  0.00
 - Asymptomatic isolated  0.00  0.00  0.00  0.00  0.00  0.88  0.12  0.00
 - Recovered              0.00  0.00  0.00  0.00  0.00  0.00  1.00  0.00
 - Removed                0.00  0.00  0.00  0.00  0.00  0.00  0.00  1.00

```
