epiworld c++ template library
================

## Main features

This C++ template-header-only library provides a general framework for
epidemiologic simulation. The main features of the library are:

1.  Four key classes: `Model`, `Person`, `Tool`, and `Virus`.
2.  The model features a social networks of `Persons`.
3.  `Persons` can have multiple `Tools` as a defense system.
4.  `Tools` can reduce contagion rate, transmissibility, death rates,
    and improve recovery rates.
5.  `Viruses` can mutate (generating new variants).
6.  `Models` can feature multiple states, e.g., `HEALTHY`,
    `SUSCEPTIBLE`, etc.
7.  `Models` can have an arbitrary number of parameters.
8.  **REALLY FAST** About 6.5 Million person/day simulations per second.

<img src="/home/george/Documents/research/world-epi/contents.svg">

## Algorithm

Setup

-   Create viruses.
-   Create tools (arbitrary).
-   Set model parameters (arbitrary).
-   Create global events (e.g., surveillance).
-   Set up the population: small world network (default).
-   Set up rewiring (optional).
-   Set statuses (arbitrary number of them).

Run

1.  Distribute the tool(s) and virus(es)

2.  For each t in 1 -> Duration:

    -   Update status for susceptible/infected/removed(?)
    -   Mutate virus(es) (each individual)
    -   Run global actions (e.g., surveillance)
    -   Run rewiring algorithm

Along update:

-   Contagion events are applied recorded.
-   New variants are recorded.
-   Optional user data is recorded.

## Hello world (C++)

``` cpp
#include "include/epiworld/epiworld.hpp"

int main()
{

  // Creating a virus
  epiworld::Virus<> covid19("covid 19");
  covid19.set_infectiousness(.8);
  
  // Creating a tool
  epiworld::Tool<> vax("vaccine");
  vax.set_contagion_reduction(.95);

  // Creating a model
  epiworld::Model<> model;

  // Adding the tool and virus
  model.add_virus(covid19, .01);
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

# Surveillance simulation

-   Incubation time of the disease `~ Gamma(3, 1)`
-   Duration of the disease `~Gamma(12, 1)`
-   Probability of becoming symptomatic: 0.9
-   Prob. of transmission: 1.0.
-   Vaccinated population: 25%
-   Vaccine efficacy: .9.
-   Vaccine reduction on transmission: 0.5.
-   Surveillance program of x% of the population at random.
-   Individuals who test positive become isolated.

## Preliminary results

``` r
# With low surveillance
pop_size <- 20e3
pop_seed <- pop_size * .01
s_levels <- c(0.0001, 0.002)
system(sprintf("./07-surveillance.o %i %i 100 %.04f 2>&1", pop_seed, pop_size, s_levels[1]), intern = TRUE) |>
  cat(sep = "\n")
```

    ## Running the model...
    ## _________________________________________________________________________
    ## ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
    ## 
    ## ________________________________________________________________________________
    ## SIMULATION STUDY
    ## 
    ## Population size    : 20000
    ## Days (duration)    : 200 (of 200)
    ## Number of variants : 1
    ## Last run elapsed t : 505.00ms
    ## Rewiring           : off
    ## 
    ## Virus(es):
    ##  - Covid19 (baseline prevalence: 100 seeds)
    ## Tool(s):
    ##  - Vaccine (baseline prevalence: 25.00%)
    ## 
    ## Model parameters:
    ##  - Infect period         : 12.0000
    ##  - Latent period         : 3.0000
    ##  - Prob of symptoms      : 0.7000
    ##  - Prob of transmission  : 1.0000
    ##  - Prob. death           : 0.0010
    ##  - Prob. reinfect        : 0.1000
    ##  - Surveilance prob.     : 1.0e-04
    ##  - Vax efficacy          : 0.9000
    ##  - Vax redux transmision : 0.5000
    ## 
    ## Distribution of the population at time 200:
    ##  - Total susceptible (S)           :   19900 -> 2106
    ##  - Total recovered (S)             :       0 -> 17369
    ##  - Total latent (I)                :     100 -> 109
    ##  - Total symptomatic (I)           :       0 -> 155
    ##  - Total symptomatic isolated (I)  :       0 -> 2
    ##  - Total asymptomatic (I)          :       0 -> 72
    ##  - Total asymptomatic isolated (I) :       0 -> 0
    ##  - Total removed (R)               :       0 -> 187
    ## 
    ## (S): Susceptible, (I): Infected, (R): Recovered
    ## ________________________________________________________________________________

``` r
hist1 <- read.csv("07-surveillance_hist.txt", sep = " ")
surv1 <- read.csv("07-surveillance_user_data.txt", sep = " ")

# With high surveillance
system(sprintf("./07-surveillance.o %i %i 100 %.04f 2>&1", pop_seed, pop_size, s_levels[2]), intern = TRUE) |>
  cat(sep = "\n")
```

    ## Running the model...
    ## _________________________________________________________________________
    ## ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| done.
    ## 
    ## ________________________________________________________________________________
    ## SIMULATION STUDY
    ## 
    ## Population size    : 20000
    ## Days (duration)    : 200 (of 200)
    ## Number of variants : 1
    ## Last run elapsed t : 530.00ms
    ## Rewiring           : off
    ## 
    ## Virus(es):
    ##  - Covid19 (baseline prevalence: 100 seeds)
    ## Tool(s):
    ##  - Vaccine (baseline prevalence: 25.00%)
    ## 
    ## Model parameters:
    ##  - Infect period         : 12.0000
    ##  - Latent period         : 3.0000
    ##  - Prob of symptoms      : 0.7000
    ##  - Prob of transmission  : 1.0000
    ##  - Prob. death           : 0.0010
    ##  - Prob. reinfect        : 0.1000
    ##  - Surveilance prob.     : 0.0020
    ##  - Vax efficacy          : 0.9000
    ##  - Vax redux transmision : 0.5000
    ## 
    ## Distribution of the population at time 200:
    ##  - Total susceptible (S)           :   19900 -> 2125
    ##  - Total recovered (S)             :       0 -> 17325
    ##  - Total latent (I)                :     100 -> 109
    ##  - Total symptomatic (I)           :       0 -> 155
    ##  - Total symptomatic isolated (I)  :       0 -> 8
    ##  - Total asymptomatic (I)          :       0 -> 76
    ##  - Total asymptomatic isolated (I) :       0 -> 1
    ##  - Total removed (R)               :       0 -> 201
    ## 
    ## (S): Susceptible, (I): Infected, (R): Recovered
    ## ________________________________________________________________________________

``` r
hist2 <- read.csv("07-surveillance_hist.txt", sep = " ")
surv2 <- read.csv("07-surveillance_user_data.txt", sep = " ")
hist_comb <- rbind(
  cbind(sim = as.character(s_levels[1]), hist1),
  cbind(sim = as.character(s_levels[2]), hist2)
  )
```

``` r
ggplot(hist_comb, aes(x = date, y = counts + 1, colour = status, linetype=sim)) +
  geom_line() +
  # scale_y_log10() +
  labs(y = "Counts (log)")
```

![](07-surveillance_files/figure-gfm/unnamed-chunk-2-1.png)<!-- -->

## Cases detected

``` r
survdat <- rbind(
  with(surv1, rbind(
    data.frame(Id = as.character(s_levels[1]), Date = date, Type = "N Sampled",  n = nsampled),
    data.frame(Id = as.character(s_levels[1]), Date = date, Type = "N detected",  n = ndetected),
    data.frame(Id = as.character(s_levels[1]), Date = date, Type = "N detected Asymp",  n = ndetected_asympt),
    data.frame(Id = as.character(s_levels[1]), Date = date, Type = "N Asymp",  n = nasymptomatic)
  )),
  with(surv2, rbind(
    data.frame(Id = as.character(s_levels[2]), Date = date, Type = "N Sampled",  n = nsampled),
    data.frame(Id = as.character(s_levels[2]), Date = date, Type = "N detected",  n = ndetected),
    data.frame(Id = as.character(s_levels[2]), Date = date, Type = "N detected Asymp",  n = ndetected_asympt),
    data.frame(Id = as.character(s_levels[2]), Date = date, Type = "N Asymp",  n = nasymptomatic)
  ))
)


ggplot(survdat, aes(x = Date, y = n + 1, colour = Type)) +
  geom_line() +
  facet_wrap(~Id) +
  scale_y_log10() +
  labs(y = "Counts (log)")
```

![](07-surveillance_files/figure-gfm/unnamed-chunk-3-1.png)<!-- -->
