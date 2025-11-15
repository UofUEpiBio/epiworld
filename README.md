[![Tests and coverage](https://github.com/UofUEpi/epiworld/actions/workflows/ci.yml/badge.svg)](https://github.com/UofUEpi/epiworld/actions/workflows/ci.yml)
[![C++ Documentation](https://img.shields.io/badge/Docs-Doxygen-brightgreen)](https://UofUEpi.github.io/epiworld)
![GitHub Release](https://img.shields.io/github/v/release/UofUEpiBio/epiworld)
[![codecov](https://codecov.io/gh/UofUEpiBio/epiworld/graph/badge.svg?token=CPDYG4HA0B)](https://codecov.io/gh/UofUEpiBio/epiworld)
[![ForeSITE Group](https://github.com/EpiForeSITE/software/raw/e82ed88f75e0fe5c0a1a3b38c2b94509f122019c/docs/assets/foresite-software-badge.svg)](https://github.com/EpiForeSITE)
[![DeepWiki](https://img.shields.io/badge/Wiki-DeepWiki-blue)](https://github.com/UofUEpiBio/epiworld/wiki)

# epiworld <img src="assets/branding/epiworld.png" width="200px" alt="epiworld logo" align="right">

This C++ library provides a general framework for epidemiologic simulation. The
core principle of `epiworld` is fast epidemiological prototyping for
building complex models quickly. Here are some of its main features:

  - It only depends on the standard library (C++11 required.)
  - It is a template library.
  - It is header-only ([single file](https://github.com/UofUEpiBio/epiworld/tree/master/epiworld.hpp)).
  - Models can have an arbitrary set of states.
  - Viruses and tools (e.g., vaccines, mask-wearing) can be designed to have arbitrary features.
  - Multiple tools and viruses can live in the same simulation.
  - It is *FAST*: Over 150 Million person/day simulations per second (see example below).

Various examples can be found in the [examples](https://github.com/UofUEpiBio/epiworld/tree/master/examples) folder.

## Hello world

Here is a simple SIR model implemented with `epiworld`. The source code
can be found [here](https://github.com/UofUEpiBio/epiworld/tree/master/helloworld.cpp), and you can compile the code as follows:

```bash
g++ -std=c++17 -O2 readme.cpp -o readme.o
```

As you can see in [`helloworld.cpp`](https://github.com/UofUEpiBio/epiworld/tree/master/helloworld.cpp), to use `epiworld` you only need to incorporate the single header file [`epiworld.hpp`](https://github.com/UofUEpiBio/epiworld/tree/master/epiworld.hpp):

```cpp
#include "epiworld.hpp"

using namespace epiworld;

int main()
{

    // epiworld already comes with a couple
    // of models, like the SIR
    epimodels::ModelSIR<> hello(
        "COVID-19", // Name of the virus
        0.01,        // Initial prevalence
        0.9,        // Transmission probability
        0.3         // Recovery probability
        );

    // We can simulate agents using a smallworld network
    // with 100,000 individuals, in this case
    hello.agents_smallworld(100000, 4L, false, .01);

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
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : Susceptible-Infected-Recovered (SIR)
Population size     : 100000
Agents' data        : (none)
Number of entities  : 0
Days (duration)     : 100 (of 100)
Number of viruses   : 1
Last run elapsed t  : 44.00ms
Last run speed      : 223.71 million agents x day / second
Rewiring            : off

Global events:
 (none)

Virus(es):
 - COVID-19

Tool(s):
 (none)

Model parameters:
 - Recovery rate     : 0.3000
 - Transmission rate : 0.9000

Distribution of the population at time 100:
  - (0) Susceptible :  99000 -> 2565
  - (1) Infected    :   1000 -> 366
  - (2) Recovered   :      0 -> 97069

Transition Probabilities:
 - Susceptible  0.96  0.04     -
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

    // Creating a model with three statuses:
    // - Susceptible: Status 0
    // - Infected: Status 1
    // - Recovered: Status 2
    Model<> model;
    model.add_status("Susceptible", default_update_susceptible<>);
    model.add_status("Infected", default_update_exposed<>);
    model.add_status("Recovered");

    // Desgining a virus: This virus will:
    // - Have a 90% transmission rate
    // - Have a 30% recovery rate
    // - Infected individuals become "Infected" (status 1)
    // - Recovered individuals become "Recovered" (status 2)
    // 100 individuals will have the virus from the beginning.
    Virus<> virus("covid 19");

    virus.set_prob_infecting(.90);
    virus.set_prob_recovery(.30);

    virus.set_status(1, 2);

    model.default_add_virus<TSeq>n(virus, 1000);

    // Generating a random pop from a smallworld network
    model.agents_smallworld(100000, 4L, false, .01);

    // Initializing setting days and seed
    model.init(100, 122);

    // Running the model
    model.run();
    model.print();

}
```

Which should print something like the following:

```bash
_________________________________________________________________________
Running the model...
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
_________________________________________________________________________
SIMULATION STUDY

Name of the model   : (none)
Population size     : 100000
Agents' data        : (none)
Number of entities  : 0
Days (duration)     : 100 (of 100)
Number of viruses   : 1
Last run elapsed t  : 50.00ms
Last run speed      : 196.85 million agents x day / second
Rewiring            : off

Global events:
 (none)

Virus(es):
 - covid 19

Tool(s):
 (none)

Model parameters:
 (none)

Distribution of the population at time 100:
  - (0) Susceptible :  99000 -> 2565
  - (1) Infected    :   1000 -> 366
  - (2) Recovered   :      0 -> 97069

Transition Probabilities:
 - Susceptible  0.96  0.04     -
 - Infected        -  0.70  0.30
 - Recovered       -     -  1.00
```

Which took about 0.209 seconds (~ 47 million ppl x day / second).

## Simulation Steps

The core logic of the model relies on user-defined statuses and their corresponding update functions. In particular, the model does not have a predefined set of statuses, e.g., susceptible, infected, recovered; the user establishes them. This provides flexibility as models in `epiworld` can have an arbitrary set of statuses.

Like most other ABMs, `epiworld` simulates the evolution of a system in discrete steps. Each step represents a day in the system, and changes are reflected at the beginning of the following day. Therefore, agents can recover and transmit a virus on the same day. A single step of `epiworld` features the following procedures:

**Status update**: Agents are updated according to their status.

2. (optional) **Execute Global events**: A call of user-defined functions affecting
the system. These can make any type of change in the system.

3. (optional) **Apply rewiring algorithm**: When specified, the network is rewired
according to a user-defined function.

4. **Lock the results**: The current date is incremented in one unit, and
  the changes (exposition, new infections, recoveries, etc.) are recorded
  in the database.

5. (optional) **Mutate Variants**: When defined, variants can mutate, with the new
variants appearing the next day.

To speed up computations, `epiworld` uses a queuing system that decides which agents will be active during each step and which will not. Agents are active when either they or at least one of their neighbors has a virus active. Agents' updates are triggered only for those in the queue, accelerating the completion of the current step.

## Agents

Agents carry two sets of important information: viruses and tools. Each agent
can have multiple instances of them, meaning multiple viruses and tools can
coexist in a model. At each step of the simulation, an agent can face the following
changes:

- **Acquire a virus (`add_virus()`)**: Become exposed to a particular virus+host.

- **Lose a virus (`rm_virus()`)**: Removing a virus from the agent. Losing a virus
triggers a call to the virus's `postrecovery()` function, which can, for example,
result in gaining immunity to that variant.

- **Acquire a tool (`add_tool()`)**: For example, mask-wearing, vaccines, etc.

- **Lose a tool (`rm_tool()`)**: For example, stop wearing masks, lose immunity, etc.

- **Change status (`change_status()`)**: An arbitrary change in the status of the
agent. Examples of this are moving from "exposed" to "infected," from "infected"
to "ICU," etc.

- **Become removed (`rm_agent_by_virus()`)**: An agent becomes inactive after its condition worsens. In such a case, all viruses attached to the agent are also removed.

Any action in the model can trigger a change in its queuing system. By default, becoming exposed makes the agent (and its neighbors) active in the queuing system. Likewise, losing all viruses could make the agent and its neighbors inactive.

<!--
## Tools -->

## Contagion

Susceptible individuals can acquire a virus from any of their infected connections. The probability that susceptible individual `i` gets the virus `v` from individual `j` depends on how three things:

1. The transmissibility of the virus, <!-- $P_v \in [0,1]$ --> <img style="transform: translateY(0.1em); background: white;" src="https://latex.codecogs.com/svg.latex?P_v%20%5Cin%20%5B0%2C1%5D">,
2. The contagion reduction factor of `i`, <!-- $C_r \in [0,1]$ --> <img style="transform: translateY(0.1em); background: white;" src="https://latex.codecogs.com/svg.latex?C_r%20%5Cin%20%5B0%2C1%5D">, and
3. The host's transmission reduction factor, <!-- $T_r \in [0,1]$ --> <img style="transform: translateY(0.1em); background: white;" src="https://latex.codecogs.com/svg.latex?T_r%20%5Cin%20%5B0%2C1%5D">.

The last two are computed from <!-- $i$ --> <img style="transform: translateY(0.1em); background: white;" src="https://latex.codecogs.com/svg.latex?i"> and <!-- $j$ --> <img style="transform: translateY(0.1em); background: white;" src="https://latex.codecogs.com/svg.latex?j">'s tools. Ultimately, the probability of <!-- $i$ --> <img style="transform: translateY(0.1em); background: white;" src="https://latex.codecogs.com/svg.latex?i"> getting virus $v$ from <!-- $j$ --> <img style="transform: translateY(0.1em); background: white;" src="https://latex.codecogs.com/svg.latex?j"> equals:

<!-- $$
P(Virus v) = P_v * (1 - C_r) *(1 - T_r)
$$ -->

<div align="center"><img style="background: white;" src="https://latex.codecogs.com/svg.latex?P(Virus%20v)%20%3D%20P_v%20*%20(1%20-%20C_r)%20*(1%20-%20T_r)%20"></div>

Nonetheless, the default behavior of the simulation model is to assume that individuals can acquire only one disease at a time, if any. This way, the actual probability is:

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
