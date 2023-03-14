## Example: 07-surveillance

Output from the program:

```
[epiworld-debug] DEBUGGING ON (compiled with EPI_DEBUG defined)

________________________________________________________________________________
SIMULATION STUDY

Population size     : 10000
Number of entitites : 0
Days (duration)     : 0 (of 100)
Number of variants  : 1
Last run elapsed t  : -
Rewiring            : off

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

Distribution of the population at time 0:
 - (0) Susceptible           : 9990
 - (1) Latent                : 10
 - (2) Symptomatic           : 0
 - (3) Symptomatic isolated  : 0
 - (4) Asymptomatic          : 0
 - (5) Asymptomatic isolated : 0
 - (6) Recovered             : 0
 - (7) Removed               : 0
Running the model...
_________________________________________________________________________
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
[epiworld-debug] DEBUGGING ON (compiled with EPI_DEBUG defined)

________________________________________________________________________________
SIMULATION STUDY

Population size     : 10000
Number of entitites : 0
Days (duration)     : 100 (of 100)
Number of variants  : 1
Last run elapsed t  : 12.00ms
Rewiring            : off

Virus(es):
 - Covid19 (baseline prevalence: 10 seeds)

Tool(s):
 - Vaccine (baseline prevalence: 25.00%)
 - Immunity (Covid19) (originated in the model...)

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
 - (0) Susceptible           :  9990 -> 8068
 - (1) Latent                :    10 -> 46
 - (2) Symptomatic           :     0 -> 158
 - (3) Symptomatic isolated  :     0 -> 2
 - (4) Asymptomatic          :     0 -> 70
 - (5) Asymptomatic isolated :     0 -> 1
 - (6) Recovered             :     0 -> 1635
 - (7) Removed               :     0 -> 20

Transition Probabilities:
 - Susceptible            1.00  0.00  0.00  0.00  0.00  0.00  0.00  0.00
 - Latent                 0.00  0.68  0.23  0.00  0.09  0.00  0.00  0.00
 - Symptomatic            0.00  0.00  0.94  0.00  0.00  0.00  0.06  0.00
 - Symptomatic isolated   0.00  0.00  0.00  0.92  0.00  0.00  0.08  0.00
 - Asymptomatic           0.00  0.00  0.00  0.00  0.94  0.00  0.06  0.00
 - Asymptomatic isolated  0.00  0.00  0.00  0.00  0.00  0.84  0.16  0.00
 - Recovered              0.00  0.00  0.00  0.00  0.00  0.00  1.00  0.00
 - Removed                0.00  0.00  0.00  0.00  0.00  0.00  0.00  1.00

```
