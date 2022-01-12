## Example: 04-advanced-usage

Output from the program:

```

________________________________________________________________________________
SIMULATION STUDY

Population size    : 1000
Days (duration)    : 0 (of 60)
Number of variants : 1
Last run elapsed t : -
Rewiring           : on (0.10)

Virus(es):
 - COVID19 (baseline prevalence: 1.00%)
Tool(s):
 - Vaccine (baseline prevalence: 50.00%)
 - Face masks (baseline prevalence: 50.00%)
 - Immune system (baseline prevalence: 100.00%)

Model parameters:
 - Mutation rate : 0.0010
 - imm death     : 0.0010
 - imm efficacy  : 0.1000
 - imm recovery  : 0.1000
 - imm trans     : 0.9000
 - vax death     : 1.0e-04
 - vax efficacy  : 0.9000

Distribution of the population at time 0:
 - Total healthy (S)   : 990
 - Total recovered (S) : 0
 - Total infected (I)  : 10
 - Total removed (R)   : 0

(S): Susceptible, (I): Infected, (R): Recovered
________________________________________________________________________________

Running the model...
_________________________________________________________________________
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.

________________________________________________________________________________
SIMULATION STUDY

Population size    : 1000
Days (duration)    : 60 (of 60)
Number of variants : 1
Last run elapsed t : 61.00ms
Rewiring           : on (0.10)

Virus(es):
 - COVID19 (baseline prevalence: 1.00%)
Tool(s):
 - Vaccine (baseline prevalence: 50.00%)
 - Face masks (baseline prevalence: 50.00%)
 - Immune system (baseline prevalence: 100.00%)

Model parameters:
 - Mutation rate : 0.0010
 - imm death     : 0.0010
 - imm efficacy  : 0.1000
 - imm recovery  : 0.1000
 - imm trans     : 0.9000
 - vax death     : 1.0e-04
 - vax efficacy  : 0.9000

Distribution of the population at time 60:
 - Total healthy (S)   :     990 -> 966
 - Total recovered (S) :       0 -> 34
 - Total infected (I)  :      10 -> 0
 - Total removed (R)   :       0 -> 0

(S): Susceptible, (I): Infected, (R): Recovered
________________________________________________________________________________

```
