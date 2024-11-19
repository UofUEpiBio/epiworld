# Community-hospital model

In this model, we have three states:

- Susceptible individuals (in the community),
- Infected individuals (in the community), and
- Infected hospitalized individuals.

Susceptible individuals may become hospitalized or not (so they have two possible transitions), and infected individuals may recover or (if hospitalized) stay infected but be discharged (so they go back as infected but in the community).

The model has the following parameters:

- Prob hospitalization: 0.1
- Prob recovery: 0.33
- Discharge infected: 0.1

Here is an example of the run

```bash
root ➜ /workspaces/epiworld/examples/14-community-hosp (example-karim) $ make
g++ -std=c++17 -O3 -g main.cpp -o main.o
root ➜ /workspaces/epiworld/examples/14-community-hosp (example-karim) $ ./main.o
_________________________________________________________________________
Running the model...
||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
 done.
________________________________________________________________________________
________________________________________________________________________________
SIMULATION STUDY

Name of the model   : (none)
Population size     : 1000
Agents' data        : (none)
Number of entities  : 0
Days (duration)     : 100 (of 100)
Number of viruses   : 1
Last run elapsed t  : 4.00ms
Last run speed      : 20.81 million agents x day / second
Rewiring            : off

Global events:
 (none)

Virus(es):
 - MRSA

Tool(s):
 (none)

Model parameters:
 - Discharge infected   : 0.1000
 - Prob hospitalization : 0.1000
 - Prob recovery        : 0.3300

Distribution of the population at time 100:
  - (0) Susceptible             :  990 -> 937
  - (1) Infected                :   10 -> 49
  - (2) Infected (hospitalized) :    0 -> 14

Transition Probabilities:
 - Susceptible              0.98  0.02  0.00
 - Infected                 0.32  0.61  0.07
 - Infected (hospitalized)  0.35  0.07  0.58
```

## Notes

An few key observations from this example.

1. **We have a sampling function that exclude population**. These two functions are used in the update functions so, when susceptible (in the community) sample, the sampling excludes individuals who are hospitalized. Likewise, hospitalized 


    ```cpp
    // A sampler that excludes infected from the hospital
    auto sampler_suscept = sampler::make_sample_virus_neighbors<>(
        {States::Infected_Hospitalized}
        );
    ```

2. **All update functions are built from scratch**. For instance, susceptible individuals are updated according to the following function:

    ```cpp
    inline void update_susceptible(Agent<int> * p, Model<int> * m)
    {

        auto virus = sampler_suscept(p, m);
        if (virus != nullptr)
        {
            if (m->par("Prob hospitalization") > m->runif())
                p->set_virus(*virus, m, States::Infected_Hospitalized);
            else
                p->set_virus(*virus, m, States::Infected);
        }


        return;

    }
    ```
