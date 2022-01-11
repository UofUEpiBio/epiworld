# epiworld

This C++ template-header-only library provides a general framework for epidemiologic simulation. The main features of the library are:

  1. Four key classes: `Model`, `Person`, `Tool`, and `Virus`.
  2. The model features a social networks of `Persons`.
  3. `Persons` can have multiple `Tools` as a defense system.
  4. `Tools` can reduce contagion rate, transmissibility, death rates, and improve recovery rates.
  5. `Viruses` can mutate (generating new variants).
  6. `Models` can feature multiple states, e.g., `HEALTHY`, `SUSCEPTIBLE`, etc.
  7. `Models` can have an arbitrary number of parameters.

# Hello world

```{cpp}
#include "include/epiworld/epiworld.hpp"

int main()
{

  // Creating a virus
  epiworld::Virus<> covid19("covid 19");
  covid19.set_prob_infecting(.8);
  
  // Creating a tool
  epiworld::Tool<> vax("vaccine");
  vax.set_contagion_reduction(.95);

  // Creating a model
  epiworld::Model<> model;

  // Adding the tool and virus
  model.add_virus(covid19, 10);
  model.add_tool(vax, .5);

  // Generating a random pop
  model.pop_from_adjlist(
    epiworld::rgraph_smallworld(1000, 5, .2)
  );

  // Initializing setting days and seed
  model.init(60, 123123);

  // Running the model
  model.run();

  model.print();

  return;
}

```

Possible output:

```
Running the model...
_________________________________________________________________________
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.

________________________________________________________________________________
SIMULATION STUDY

Population size    : 1000
Days (duration)    : 60 (of 60)
Number of variants : 1
Last run elapsed t : 6.00ms
Rewiring           : off

Virus(es):
 - unknown virus (baseline prevalence: 5.00)
Tool(s):
 - unknown tool (baseline prevalence: 0.50)

Model parameters:

Distribution of the population at time 60:
 - Total healthy (S)   : 118
 - Total recovered (S) : 312
 - Total infected (I)  : 570
 - Total removed (R)   : 0

(S): Susceptible, (I): Infected, (R): Recovered
________________________________________________________________________________
```

## Tools

## Contagion

Susceptible individuals can acquire a virus from any of their infected connections. The probability that susceptible individual `i` gets the virus `v` from individual `j` depends on how three things:

1. The transmissibility of the virus, Pv in [0,1],
2. The contagion reduction factor of `i`, Cr in [0,1], and
3. The host's transmission reduction factor, Tr [0,1].

The last two are computed from `i` and `j`'s tools. Ultimately, the probability of `i` getting virus `v` from `j` equals:

```
P(Virus v) = Pv * (1 - Cr) *(1 - Tr) 
```

Nonetheless, the default behavior of the simulation model is to assume that individuals can acquire only one disease at a time, if any. This way, the actual probability is:

```
P(Virus v| at most one virus) = Prcond(i,v,j)
```

The latter is calculated using Bayes' rule

```
Prcond(i,v,j) = P(at most one virus|Virus v) * P(Variant v) / P(at most one virus)
              = P(Only Virus v)/P(Virus v) * P(Virus v) / P(at most one virus)
              = P(Only Virus v)/P(at most one virus)
```

Where

```
P(Vnly Virus V)      = P(Virus V) * Prod(m!=V) (1 - P(Virus m))
P(at most one virus) = P(None) + Sum(k in viruses) P(Virus k) * Prod(m != k) (1 - P(Virus m))
P(None)              = Prod(k in Viruses) (1 - P(Virus k))
```

This way, viruses with higher transmissibility will be more likely to be acquired when competing with other variants.