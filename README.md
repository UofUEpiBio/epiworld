[![Tests and coverage](https://github.com/UofUEpi/epiworld/actions/workflows/ci.yml/badge.svg)](https://github.com/UofUEpi/epiworld/actions/workflows/ci.yml) [![C++ Documentation](https://img.shields.io/badge/Docs-Doxygen-brightgreen)](https://UofUEpi.github.io/epiworld)

# epiworld

This C++ template-header-only library provides a general framework for epidemiologic simulation. The main features of the library are:

  1. Four key classes: `Model`, `Person`, `Tool`, and `Virus`.
  2. The model features a social networks of `Persons`.
  3. `Persons` can have multiple `Tools` as a defense system.
  4. `Tools` can reduce contagion rate, transmissibility, death rates, and improve recovery rates.
  5. `Viruses` can mutate (generating new variants).
  6. `Models` can feature multiple states, e.g., `HEALTHY`, `SUSCEPTIBLE`, etc.
  7. `Models` can have an arbitrary number of parameters.
  8. **REALLY FAST** About 6.5 Million person/day simulations per second.

<img src="contents.svg">

# Hello world

Here is a simple SIRS model implemented with 

```cpp
#include "../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main()
{

        // Creating a model
    Model<> model;

    // Adding the tool and virus
    Virus<> virus("covid 19");
    virus.set_post_immunity(1.0);
    model.add_virus_n(virus, 5);
    
    Tool<> tool("vaccine");
    model.add_tool(tool, .5);

    // Generating a random pop 
    model.population_smallworld(100000);

    // Initializing setting days and seed
    model.init(100, 123);

    // Running the model
    model.run();
    model.print();
  
}
```

And you should get something like the following:

```bash
Running the model...
_________________________________________________________________________
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.

________________________________________________________________________________
SIMULATION STUDY

Population size    : 100000
Days (duration)    : 100 (of 100)
Number of variants : 1
Last run elapsed t : 280.00ms
Rewiring           : off

Virus(es):
 - covid 19 (baseline prevalence: 5 seeds)
Tool(s):
 - vaccine (baseline prevalence: 50.00%)

Model parameters:

Distribution of the population at time 100:
 - Total healthy (S)   :   99995 -> 97390
 - Total recovered (S) :       0 -> 2554
 - Total infected (I)  :       5 -> 56
 - Total removed (R)   :       0 -> 0

(S): Susceptible, (I): Infected, (R): Recovered
________________________________________________________________________________
```

Which took about 0.280 seconds.

## Tools

## Contagion

Susceptible individuals can acquire a virus from any of their infected connections. The probability that susceptible individual `i` gets the virus `v` from individual `j` depends on how three things:

1. The transmissibility of the virus, <!-- $P_v \in [0,1]$ --> <img style="transform: translateY(0.1em); background: white;" src="https://render.githubusercontent.com/render/math?math=P_v%20%5Cin%20%5B0%2C1%5D">,
2. The contagion reduction factor of `i`, <!-- $C_r \in [0,1]$ --> <img style="transform: translateY(0.1em); background: white;" src="https://render.githubusercontent.com/render/math?math=C_r%20%5Cin%20%5B0%2C1%5D">, and
3. The host's transmission reduction factor, <!-- $T_r \in [0,1]$ --> <img style="transform: translateY(0.1em); background: white;" src="https://render.githubusercontent.com/render/math?math=T_r%20%5Cin%20%5B0%2C1%5D">.

The last two are computed from <!-- $i$ --> <img style="transform: translateY(0.1em); background: white;" src="https://render.githubusercontent.com/render/math?math=i"> and <!-- $j$ --> <img style="transform: translateY(0.1em); background: white;" src="https://render.githubusercontent.com/render/math?math=j">'s tools. Ultimately, the probability of <!-- $i$ --> <img style="transform: translateY(0.1em); background: white;" src="https://render.githubusercontent.com/render/math?math=i"> getting virus $v$ from <!-- $j$ --> <img style="transform: translateY(0.1em); background: white;" src="https://render.githubusercontent.com/render/math?math=j"> equals:

<!-- $$
P(Virus v) = P_v * (1 - C_r) *(1 - T_r) 
$$ --> 

<div align="center"><img style="background: white;" src="https://render.githubusercontent.com/render/math?math=P(Virus%20v)%20%3D%20P_v%20*%20(1%20-%20C_r)%20*(1%20-%20T_r)%20"></div>

Nonetheless, the default behavior of the simulation model is to assume that individuals can acquire only one disease at a time, if any. This way, the actual probability is:

<!-- $$
P(Virus v|\mbox{at most one virus}) = P_{ivj}
$$ --> 

<div align="center"><img style="background: white;" src="https://render.githubusercontent.com/render/math?math=P(Virus%20v%7C%5Cmbox%7Bat%20most%20one%20virus%7D)%20%3D%20P_%7Bivj%7D"></div>

The latter is calculated using Bayes' rule

<!-- $$
\begin{align*}
P_{ivj} & = P(at most one virus|Virus v) * P_v / P(at most one virus) \\
        & = P(Only Virus v)/P_v * P(Virus v) / P(at most one virus) \\
        & = P(Only Virus v)/P(at most one virus)
\end{align*}
$$ --> 

<div align="center"><img style="background: white;" src="https://render.githubusercontent.com/render/math?math=%5Cbegin%7Balign*%7D%0AP_%7Bivj%7D%20%26%20%3D%20P(at%20most%20one%20virus%7CVirus%20v)%20*%20P_v%20%2F%20P(at%20most%20one%20virus)%20%5C%5C%0A%20%20%20%20%20%20%20%20%26%20%3D%20P(Only%20Virus%20v)%2FP_v%20*%20P(Virus%20v)%20%2F%20P(at%20most%20one%20virus)%20%5C%5C%0A%20%20%20%20%20%20%20%20%26%20%3D%20P(Only%20Virus%20v)%2FP(at%20most%20one%20virus)%0A%5Cend%7Balign*%7D"></div>


Where

<!-- $$
\begin{align*}
P(Only Virus V)      & = P_v * \prod_{m!=V} (1 - P_m) \\
P(at most one virus) & = P(None) + \sum_{k \in viruses} P_k * \prod_{m != k} (1 - P_m) \\
P(None)              & = \prod_{k \in Viruses} (1 - P_k)
\end{align*}
$$ --> 

<div align="center"><img style="background: white;" src="https://render.githubusercontent.com/render/math?math=%5Cbegin%7Balign*%7D%0AP(Only%20Virus%20V)%20%20%20%20%20%20%26%20%3D%20P_v%20*%20%5Cprod_%7Bm!%3DV%7D%20(1%20-%20P_m)%20%5C%5C%0AP(at%20most%20one%20virus)%20%26%20%3D%20P(None)%20%2B%20%5Csum_%7Bk%20%5Cin%20viruses%7D%20P_k%20*%20%5Cprod_%7Bm%20!%3D%20k%7D%20(1%20-%20P_m)%20%5C%5C%0AP(None)%20%20%20%20%20%20%20%20%20%20%20%20%20%20%26%20%3D%20%5Cprod_%7Bk%20%5Cin%20Viruses%7D%20(1%20-%20P_k)%0A%5Cend%7Balign*%7D"></div>

This way, viruses with higher transmissibility will be more likely to be acquired when competing with other variants.
