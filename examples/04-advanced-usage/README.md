## Example: 04-advanced-usage

Output from the program:

```
DEBUGGING ON (compiled with EPI_DEBUG defined)
________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : (none)
Population size     : 1000
Agents' data        : (none)
Number of entities  : 0
Days (duration)     : 0 (of 0)
Number of viruses   : 1
Last run elapsed t  : -
Rewiring            : on (0.10)

Global events:
 (none)

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
Running the model...
DEBUGGING ON (compiled with EPI_DEBUG defined)
________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : (none)
Population size     : 1000
Agents' data        : (none)
Number of entities  : 0
Days (duration)     : 60 (of 60)
Number of viruses   : 1
Last run elapsed t  : 35.00ms
Last run speed      : 1.68 million agents x day / second
Rewiring            : on (0.10)

Global events:
 (none)

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

Distribution of the population at time 60:
  - (0) Susceptible :  990 -> 901
  - (1) Exposed     :   10 -> 0
  - (2) Recovered   :    0 -> 97
  - (3) Removed     :    0 -> 2

Transition Probabilities:
 - Susceptible  1.00  0.00  0.00  0.00
 - Exposed      0.00  0.75  0.24  0.00
 - Recovered    0.00  0.00  1.00  0.00
 - Removed      0.00  0.00  0.00  1.00

```
