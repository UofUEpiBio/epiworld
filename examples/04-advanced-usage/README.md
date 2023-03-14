## Example: 04-advanced-usage

Output from the program:

```
[epiworld-debug] DEBUGGING ON (compiled with EPI_DEBUG defined)

________________________________________________________________________________
SIMULATION STUDY

Population size     : 1000
Number of entitites : 0
Days (duration)     : 0 (of 60)
Number of variants  : 1
Last run elapsed t  : -
Rewiring            : on (0.10)

Virus(es):
 - COVID19 (baseline prevalence: 1.00%)

Tool(s):
 - Immune system (baseline prevalence: 100.00%)
 - Vaccine (baseline prevalence: 50.00%)
 - Face masks (baseline prevalence: 50.00%)
 - Post Immune (baseline prevalence: 0 seeds)

Model parameters:
 - Mutation rate : 0.0010
 - imm death     : 0.0010
 - imm efficacy  : 0.1000
 - imm recovery  : 0.1000
 - imm trans     : 0.9000
 - vax death     : 1.0e-04
 - vax efficacy  : 0.9000
 - virus death   : 0.0100

Distribution of the population at time 0:
 - (0) Susceptible : 990
 - (1) Exposed     : 10
 - (2) Recovered   : 0
 - (3) Removed     : 0
Running the model...
_________________________________________________________________________
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
[epiworld-debug] DEBUGGING ON (compiled with EPI_DEBUG defined)

________________________________________________________________________________
SIMULATION STUDY

Population size     : 1000
Number of entitites : 0
Days (duration)     : 60 (of 60)
Number of variants  : 3
Last run elapsed t  : 63.00ms
Rewiring            : on (0.10)

Virus(es):
 - COVID19 (baseline prevalence: 1.00%)
 - COVID19 (originated in the model...)
 - COVID19 (originated in the model...)

Tool(s):
 - Immune system (baseline prevalence: 100.00%)
 - Vaccine (baseline prevalence: 50.00%)
 - Face masks (baseline prevalence: 50.00%)
 - Post Immune (baseline prevalence: 0 seeds)

Model parameters:
 - Mutation rate : 0.0010
 - imm death     : 0.0010
 - imm efficacy  : 0.1000
 - imm recovery  : 0.1000
 - imm trans     : 0.9000
 - vax death     : 1.0e-04
 - vax efficacy  : 0.9000
 - virus death   : 0.0100

Distribution of the population at time 60:
 - (0) Susceptible :  990 -> 751
 - (1) Exposed     :   10 -> 20
 - (2) Recovered   :    0 -> 220
 - (3) Removed     :    0 -> 9

Transition Probabilities:
 - Susceptible  1.00  0.00  0.00  0.00
 - Exposed      0.00  0.78  0.21  0.01
 - Recovered    0.00  0.00  1.00  0.00
 - Removed      0.00  0.00  0.00  1.00

```
