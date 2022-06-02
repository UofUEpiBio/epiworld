## Example: 10-likelihood-free-mcmc

Output from the program:

```

________________________________________________________________________________
SIMULATION STUDY

Population size    : 500
Days (duration)    : 50 (of 50)
Number of variants : 1
Last run elapsed t : 2.00ms
Rewiring           : off

Virus(es):
 - covid (baseline prevalence: 10.00%)

Tool(s):
 - Immunity (covid) (baseline prevalence: 0 seeds)
 - Immune system (baseline prevalence: 100.00%)

Model parameters:
 - Immune recovery        : 0.5000
 - Immune suscept. redux. : 0.0e+00
 - Infectiousness         : 0.9000
 - Post immunity          : 1.0000

Distribution of the population at time 50:
- (0) Susceptible     : 450 -> 0
- (1) Infected        :  50 -> 13
- (2) Recovered       :   0 -> 240
- (3) Gained Immunity :   0 -> 247
___________________________________________

LIKELIHOOD-FREE MARKOV CHAIN MONTE CARLO

N Samples : 1000
Elapsed t : 1.00s

Parameters:
  -Immune recovery :  0.48 [ 0.40,  0.55] (initial :  0.50)
  -Infectiousness  :  0.65 [ 0.20,  0.98] (initial :  0.50)

Statistics:
  -Susceptible     :    0.02 [   0.00,    0.00] (Observed:  0.00)
  -Infected        :   13.43 [   9.00,   16.00] (Observed:  13.00)
  -Recovered       :  241.80 [ 226.00,  266.00] (Observed:  240.00)
  -Gained Immunity :  235.24 [ 218.00,  259.00] (Observed:  247.00)
___________________________________________

```
