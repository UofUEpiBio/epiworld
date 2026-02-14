## Example: `07-surveillance`

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

Global events:
 - Surveilance program (runs daily)

Virus(es):
 - Covid19

Tool(s):
 - Vaccine

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
Last run elapsed t  : 87.00ms
Last run speed      : 11.39 million agents x day / second
Rewiring            : off

Global events:
 - Surveilance program (runs daily)

Virus(es):
 - Covid19

Tool(s):
 - Vaccine

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
  - (0) Susceptible           :  9990 -> 8210
  - (1) Latent                :    10 -> 72
  - (2) Symptomatic           :     0 -> 140
  - (3) Symptomatic isolated  :     0 -> 2
  - (4) Asymptomatic          :     0 -> 78
  - (5) Asymptomatic isolated :     0 -> 0
  - (6) Recovered             :     0 -> 1481
  - (7) Removed               :     0 -> 17

Transition Probabilities:
 - Susceptible            1.00  0.00     -  0.00     -     -     -     -
 - Latent                    -  0.64  0.25  0.00  0.10  0.00  0.00     -
 - Symptomatic               -     -  0.91  0.00     -     -  0.09  0.00
 - Symptomatic isolated      -     -     -  0.89     -     -  0.11     -
 - Asymptomatic              -     -     -     -  0.90  0.00  0.09  0.00
 - Asymptomatic isolated     -     -     -     -     -  0.89  0.11     -
 - Recovered                 -     -     -     -     -     -  1.00     -
 - Removed                   -     -     -     -     -     -     -  1.00

```
