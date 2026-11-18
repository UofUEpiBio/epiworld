[![Tests and coverage](https://github.com/UofUEpi/epiworld/actions/workflows/test.yml/badge.svg)](https://github.com/UofUEpi/epiworld/actions/workflows/test.yml)
[![C++ Documentation](https://img.shields.io/badge/Docs-Doxygen-brightgreen)](https://UofUEpi.github.io/epiworld)
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/UofUEpiBio/epiworld)
![GitHub Release](https://img.shields.io/github/v/release/UofUEpiBio/epiworld)
[![codecov](https://codecov.io/gh/UofUEpiBio/epiworld/graph/badge.svg?token=CPDYG4HA0B)](https://codecov.io/gh/UofUEpiBio/epiworld)
[![ForeSITE Group](https://github.com/EpiForeSITE/software/raw/e82ed88f75e0fe5c0a1a3b38c2b94509f122019c/docs/assets/foresite-software-badge.svg)](https://github.com/EpiForeSITE)

# epiworld <img src="assets/branding/epiworld.png" width="200px" alt="epiworld logo" align="right">

This C++ library provides a general framework for epidemiologic simulation. The
core principle of `epiworld` is fast epidemiological prototyping for
building complex models quickly. Here are some of its main features:

  - It only depends on the standard library (C++17 required.)
  - It is header-only ([single file](https://github.com/UofUEpiBio/epiworld/tree/master/epiworld.hpp)).
  - A disease is declared as a natural history: its stages and the
    transitions between them (by contact, at a daily rate, after a drawn
    duration, or fired by code). Agents also have a status (free,
    quarantined, isolated, ...) with transitions of its own.
  - Contacts come from layers: networks, mixing by group, and mixing within
    groups, any number of them in one model.
  - Viruses and tools (e.g., vaccines, mask-wearing) can be designed to have arbitrary features.
  - Multiple tools and viruses can live in the same simulation.
  - It is *FAST*: Over 150 Million person/day simulations per second (see example below).

An evolving [cross-framework benchmark](https://github.com/UofUEpiBio/epiworld-benchmark)
compares epiworld, its R and Python wrappers, and other epidemic ABM engines.
In the SEIRH network scenarios currently covered, the epiworld family is among
the fastest, and native epiworld is among the lower-memory implementations.
These results are a snapshot of the tested versions, workloads, and environment
rather than a universal ranking; additional scenarios, engines, corrections,
and implementation improvements are welcome through the benchmark's
[issues](https://github.com/UofUEpiBio/epiworld-benchmark/issues) and
[pull requests](https://github.com/UofUEpiBio/epiworld-benchmark/pulls).

Various examples can be found in the [examples](https://github.com/UofUEpiBio/epiworld/tree/master/examples) folder. This repository also hosts the `measles` library, which extends `epiworld` with Measles-specific models. The library is available under [`include/measles`](./include/measles/).

## Hello world

Here is a simple SIR model implemented with `epiworld`. The source code
can be found [here](https://github.com/UofUEpiBio/epiworld/tree/master/helloworld.cpp), and you can compile the code as follows:

```bash
g++ -std=c++20 -O3 helloworld.cpp -o helloworld.o
```

As you can see in [`helloworld.cpp`](https://github.com/UofUEpiBio/epiworld/tree/master/helloworld.cpp), to use `epiworld` you only need to incorporate the single header file [`epiworld.hpp`](https://github.com/UofUEpiBio/epiworld/tree/master/epiworld.hpp):

```cpp
#include "epiworld.hpp"

using namespace epiworld;

int main()
{

    // epiworld already comes with a couple
    // of models, like the SIR
    epimodels::ModelSIR hello(
        "COVID-19", // Name of the virus
        100000,     // Number of agents
        0.01,       // Initial prevalence
        0.1,        // Transmission probability
        0.3         // Recovery probability
        );

    // Contacts come from layers: here, a small-world network, drawn anew
    // for every run
    hello.add_layer(layer::network(
        "contacts", graph::smallworld(10, .01)
    ));

    // Running the model and printing the results
    // Setting the number of days (100) and seed (122)
    hello.run(100, 122);
    hello.print();

    return 0;

}
```

Compiling (with `make helloworld.o`) and running the problem yields the following result:

```bash
_________________________________________________________________________
Running the model...
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : Susceptible-Infected-Recovered (SIR)
Population size     : 100000
Agents' data        : (none)
Days (duration)     : 100 (of 100)
Number of viruses   : 1
Last run elapsed t  : 93.00ms
Last run speed      : 106.49 million agents x day / second
Contact layers      : 1
  - contacts (network)
Last seed used      : 122

Global events:
 (none)

Virus(es):
 - COVID-19

Tool(s):
 (none)

Model parameters:
 - Recovery rate            : 0.3000
 - Transmission probability : 0.1000

Distribution of the population at time 100:
  - (0) Susceptible :  99000 -> 40382
  - (1) Infected    :   1000 -> 154
  - (2) Recovered   :      0 -> 59464

Transition Probabilities:
 - Susceptible  0.99  0.01     -
 - Infected        -  0.70  0.30
 - Recovered       -     -  1.00
```


## Building from scratch

One of the best things about epiworld is the capability to build models from scratch. Here is one example ([readme.cpp](https://github.com/UofUEpiBio/epiworld/tree/master/readme.cpp)):

```cpp
#include "epiworld.hpp"

using namespace epiworld;

int main()
{

    Model model(100000);

    // Designing a virus and its natural history: three stages,
    // Susceptible, Infected and Recovered. A contact with an infected agent
    // transmits with probability 0.1, and infected agents recover with daily
    // probability 0.3. 100 individuals have the virus from the beginning.
    Virus virus("covid 19");
    virus.add_stage("Susceptible");
    virus.add_stage("Infected", stage::infectious);
    virus.add_stage("Recovered");
    virus.infection(0, 1, 0.10);
    virus.rate(1, 2, 0.30).effect(Effect::recovery);

    model.add_virus(virus, assign::count(100));
    
    // Contacts from a small-world network
    model.add_layer(layer::network(
        "contacts", graph::smallworld(10, .01)
    ));

    // Running the model
    model.run(100, 122);
    model.print();
  
}
```

Which should print something like the following:

```bash
_________________________________________________________________________
Running the model...
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : (none)
Population size     : 100000
Agents' data        : (none)
Days (duration)     : 100 (of 100)
Number of viruses   : 1
Last run elapsed t  : 26.00ms
Last run speed      : 371.13 million agents x day / second
Contact layers      : 1
  - contacts (network)
Last seed used      : 122

Global events:
 (none)

Virus(es):
 - covid 19

Tool(s):
 (none)

Model parameters:
 - covid 19: Infected -> Recovered   : 0.3000
 - covid 19: Susceptible -> Infected : 0.1000

Distribution of the population at time 100:
  - (0) Susceptible :  99900 -> 82894
  - (1) Infected    :    100 -> 519
  - (2) Recovered   :      0 -> 16587

Transition Probabilities:
 - Susceptible  1.00  0.00     -
 - Infected        -  0.70  0.30
 - Recovered       -     -  1.00
```

Which took about 0.012 seconds (~ 776 million agents x day / second; run in
a container, so take the speed as an order of magnitude). The model
parameters are named after the virus and the transition: a number given to
the natural history becomes a parameter of the model, which can be changed
before a run (`model.set_param("covid 19: Susceptible -> Infected", 0.2)`).
Each model's diagram is drawn from its declarations
(`model.draw_natural_history()`, a mermaid flowchart).

## Simulation Steps

A model's states come from its natural history: the stages a virus declares
(`Virus::add_stage()`) and the transitions between them, plus the agents'
statuses (`Model::set_statuses()`; a model without statuses has one,
"Free"). Each agent is in one stage and one status. The transitions are of
four kinds:

- **infection**: from a contact with an infectious agent, with a probability
  per contact (`Virus::infection()`);
- **rate**: with a daily probability (`Virus::rate()`);
- **after**: when a duration drawn on entering the stage ends
  (`Virus::after()`, with `Duration::fixed()`, `geometric()`, `lognormal()`,
  `gamma()`, or a function);
- **external**: fired by code, such as a global event or a reaction
  (`Model::fire()`).

Like most other ABMs, `epiworld` simulates the evolution of a system in
discrete steps. Each step represents a day, and the changes of a day take
effect at its end, so agents can recover and transmit a virus on the same
day. A single step features the following procedures:

1. **Update the agents**: first the natural history (infections from the
   day's contacts, rates, and the durations that end), then the statuses,
   then the reactions to what changed (`Model::react()`).

2. (optional) **Execute global events**: user-defined functions that can make
   any change in the system.

3. **Lock the results**: the layers are updated (e.g., rewired), the changes
   are applied, the date moves one day ahead, and the day is recorded in the
   database.

4. (optional) **Mutate variants**: a virus with a mutation function can
   become a new variant (a new id, a child of the one it was).

To speed up computations, `epiworld` uses a queuing system that decides
which agents are visited in each step. Only the agents that can change are:
those with something drawn in their stage or status, and those in contact
with a carrier.

## Agents

An agent has a stage and a status, at most one virus at a time (several
viruses can circulate in a model, with the same stages), and any number of
tools. During a run, an agent can face the following changes:

- **Acquire a virus**: by an infection, or placed at the start of a run
  (`Model::add_virus(virus, assign::share(0.01))`).

- **Lose a virus**: when it moves from a stage that carries the virus
  (declared with `stage::carrier` or `stage::infectious`) to one that does
  not (e.g., recovers).

- **Acquire or lose a tool** (`add_tool()`, `rm_tool()`): for example,
  mask-wearing, vaccines, etc.

- **Change stage or status**: through the declared transitions, or fired by
  code (`Model::fire()`). Examples are moving from "Exposed" to "Infected",
  or from "Free" to "Quarantined".

## Contagion

Susceptible individuals can acquire a virus from any of their infectious contacts, in any layer (see [combining exposures](docs/impl/combining-exposures.md)). The probability that susceptible individual `i` gets the virus `v` from individual `j` depends on three things:

1. The transmissibility of the virus, <!-- $P_v \in [0,1]$ --> <img style="transform: translateY(0.1em); background: white;" src="https://latex.codecogs.com/svg.latex?P_v%20%5Cin%20%5B0%2C1%5D">,
2. The contagion reduction factor of `i`, <!-- $C_r \in [0,1]$ --> <img style="transform: translateY(0.1em); background: white;" src="https://latex.codecogs.com/svg.latex?C_r%20%5Cin%20%5B0%2C1%5D">, and
3. The host's transmission reduction factor, <!-- $T_r \in [0,1]$ --> <img style="transform: translateY(0.1em); background: white;" src="https://latex.codecogs.com/svg.latex?T_r%20%5Cin%20%5B0%2C1%5D">.

The last two are computed from <!-- $i$ --> <img style="transform: translateY(0.1em); background: white;" src="https://latex.codecogs.com/svg.latex?i"> and <!-- $j$ --> <img style="transform: translateY(0.1em); background: white;" src="https://latex.codecogs.com/svg.latex?j">'s tools. Ultimately, the probability of <!-- $i$ --> <img style="transform: translateY(0.1em); background: white;" src="https://latex.codecogs.com/svg.latex?i"> getting virus $v$ from <!-- $j$ --> <img style="transform: translateY(0.1em); background: white;" src="https://latex.codecogs.com/svg.latex?j"> equals:

<!-- $$
P(Virus v) = P_v * (1 - C_r) *(1 - T_r)
$$ -->

<div align="center"><img style="background: white;" src="https://latex.codecogs.com/svg.latex?P(Virus%20v)%20%3D%20P_v%20*%20(1%20-%20C_r)%20*(1%20-%20T_r)%20"></div>

Individuals acquire at most one virus at a time, if any. This way, the actual probability is:

<!-- $$
P(Virus v|\mbox{\text{at most one virus}}) = P_{ivj}
$$ -->

<div align="center"><img style="background: white;" src="https://latex.codecogs.com/svg.latex?P(Virus%20v%20%7C%20%5Cmbox%7Bat%20most%20one%20virus%7D)%20%3D%20P_%7Bivj%7D"></div>

The latter is calculated using Bayes' rule

<!-- $$
\begin{align*}
P_{ivj} & = P(\text{at most one virus}|\text{Virus v}) * P_v / P(\text{at most one virus}) \\
        & = P(\text{Only Virus v})/P_v * P(\text{Virus v}) / P(\text{at most one virus}) \\
        & = P(\text{Only Virus v})/P(\text{at most one virus})
\end{align*}
$$ -->

<div align="center"><img style="background: white;" src="https://latex.codecogs.com/svg.latex?%5Cbegin%7Balign%2A%7D%0AP_%7Bivj%7D%20%26%20%3D%20P%28%5Ctext%7Bat%20most%20one%20virus%7D%7C%5Ctext%7BVirus%20v%7D%29%20%2A%20P_v%20%2F%20P%28%5Ctext%7Bat%20most%20one%20virus%7D%29%20%5C%5C%0A%20%20%20%20%20%20%20%20%26%20%3D%20P%28%5Ctext%7BOnly%20Virus%20v%7D%29%2FP_v%20%2A%20P%28%5Ctext%7BVirus%20v%7D%29%20%2F%20P%28%5Ctext%7Bat%20most%20one%20virus%7D%29%20%5C%5C%0A%20%20%20%20%20%20%20%20%26%20%3D%20P%28%5Ctext%7BOnly%20Virus%20v%7D%29%2FP%28%5Ctext%7Bat%20most%20one%20virus%7D%29%0A%5Cend%7Balign%2A%7D"></div>


Where

<!-- $$
\begin{align*}
P(\text{Only Virus V})      & = P_v * \prod_{m!=V} (1 - P_m) \\
P(\text{at most one virus}) & = P(None) + \sum_{k \in \text{viruses}} P_k * \prod_{m != k} (1 - P_m) \\
P(\text{None})              & = \prod_{k \in \text{viruses}} (1 - P_k)
\end{align*}
$$ -->

<div align="center"><img style="background: white;" src="https://latex.codecogs.com/svg.latex?%5Cbegin%7Balign%2A%7D%0AP%28%5Ctext%7BOnly%20Virus%20V%7D%29%20%20%20%20%20%20%26%20%3D%20P_v%20%2A%20%5Cprod_%7Bm%21%3DV%7D%20%281%20-%20P_m%29%20%5C%5C%0AP%28%5Ctext%7Bat%20most%20one%20virus%7D%29%20%26%20%3D%20P%28%5Ctext%7BNone%7D%29%20%2B%20%5Csum_%7Bk%20%5Cin%20%5Ctext%7Bviruses%7D%7D%20P_k%20%2A%20%5Cprod_%7Bm%20%21%3D%20k%7D%20%281%20-%20P_m%29%20%5C%5C%0AP%28%5Ctext%7BNone%7D%29%20%20%20%20%20%20%20%20%20%20%20%20%20%20%26%20%3D%20%5Cprod_%7Bk%20%5Cin%20%5Ctext%7Bviruses%7D%7D%20%281%20-%20P_k%29%0A%5Cend%7Balign%2A%7D"></div>

This way, viruses with higher transmissibility will be more likely to be acquired when competing with other variants.
