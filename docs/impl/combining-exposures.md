## Overview

When a susceptible agent has several infectious contacts in a step, `epiworld` needs one number, the probability that the agent becomes infected, and one answer to "by whom". How the contacts are *combined* decides both. This page explains the rule `epiworld` has used so far (the **roulette**, which conditions on at most one transmission), the rule that replaces it in version 1.0.0 (**independent exposures**, or hazard), and why the change is needed once a model has [contact layers](#why-layers-make-this-matter).

!!! note "Status"
    Through the 0.x releases, and during the layer refactor, `epiworld` uses the roulette described below. The switch to independent exposures is planned as its own step of the 1.0.0 release (Phase 6 of the contact-layers plan), so that the earlier steps can be checked against unchanged results. Until then this page describes the design, and [Push and Pull Transmission](transmission-sampling.md) describes what the code does.

## The two rules

Let $p_1, \dots, p_n$ be the transmission probabilities of the infectious contacts of a susceptible agent in one step, each computed as in [Sampling Contacts](sampling-contacts.md#probability-of-infection):

$$
p_k = \left(1 - \text{susceptibility reduction}\right) \times p_{v_k} \times \left(1 - \text{transmission reduction}_k\right).
$$

Both rules draw *at most one infector*. They differ in what happens to the probability of infection.

### Roulette (conditional on at most one)

The roulette enumerates the outcomes "no contact transmits" and "only contact $k$ transmits" and renormalizes them, which discards the outcome "two or more transmit". With the odds $r_k = p_k / (1 - p_k)$ and $R = \sum_k r_k$:

$$
P(\text{none}) = \frac{1}{1 + R},
\qquad
P(\text{infected by } k) = \frac{r_k}{1 + R}.
$$

The renormalization lowers the probability of infection: $P(\text{infection}) = R / (1 + R)$ is a saturating function of the number of contacts, and $p_k$ stops being the probability that contact $k$ infects the agent when other infectious contacts are present.

### Independent exposures (hazard)

Treat each infectious contact as an independent exposure with its own chance of infecting. The agent escapes infection only if every exposure fails:

$$
P(\text{none}) = \prod_k \left(1 - p_k\right) = e^{-\Lambda},
\qquad
\Lambda = \sum_k \lambda_k,
\quad
\lambda_k = -\log\left(1 - p_k\right).
$$

Infection no longer depends on how the infector is chosen. If it happens, a single infector is *attributed* with probability proportional to the hazards:

$$
P(\text{infected by } k) = \frac{\lambda_k}{\Lambda}\left(1 - e^{-\Lambda}\right).
$$

This is the competing-risks reading: each exposure has its own rate, and the one that fires first is the infector. Contacts with different probabilities (vaccinated infectors, different infectiousness, different susceptibility) enter through their own $\lambda_k$, so a contact with $p_k = 0.3$ is blamed far more often than one with $p_k = 0.05$. Only when several contacts have $p_k \geq 1$ (certain infection, infinite hazard) is the infector drawn uniformly among them, as the roulette does.

In both rules the contacts are summed, either as odds or as hazards. The sum can be accumulated from either end of a tie, so [pushing](transmission-sampling.md#pushing) works the same way with either rule.

## How far apart they are

They agree for one infectious contact and for small $p_k$, and diverge as contacts pile up:

| Infectious contacts | Roulette | Independent exposures |
|---|---|---|
| 1 × $p = 0.5$ | 0.500 | 0.500 |
| 2 × $p = 0.5$ | 0.667 | 0.750 |
| 3 × $p = 0.3$ | 0.563 | 0.657 |
| 10 × $p = 0.1$ | 0.526 | 0.651 |

For two contacts at $p = 0.5$ there are four equally likely outcomes: none, only the first, only the second, both. The roulette drops "both" and renormalizes, so none gets 1/3 and infection 2/3. Independent exposures keep all four: infection is 3/4, and when both contacts succeed one of them is named the infector.

## Why layers make this matter

With a single network, an agent rarely has many infectious contacts at once, and the roulette's saturation is mild. A model with [contact layers](https://github.com/UofUEpiBio/epiworld/pull/302) (a household, a workplace, public spaces) draws contacts from several independent sources and pools them. That has two consequences:

- **Bias.** The more layers contribute infectious contacts, the more the roulette caps the risk. Household secondary attack rates come out too low, and adding a layer helps less than it should. Layers are independent sources of contact, so independent exposures is the model that matches them.
- **Weights.** A layer weight meaning "how much time a contact in this layer represents" scales the hazard, $\lambda_k \to w \lambda_k$. A weight of 2 is twice the exposure, and weights above 1 are meaningful. Scaling odds has no comparable reading.

## Cost

The hazard needs a `log1p` per infectious contact and one `exp` per agent with any, against a division for the odds. A microbenchmark gave about 4 ns extra per contact. On real models (SEIR on small-world and heterogeneous household networks in pull and push, and SEIR with mixing; 100,000 to 1,000,000 agents, CPU time, variants interleaved), the cost per infectious contact was indistinguishable from the roulette's, with a median ratio of 1.01 and differences within the noise (below about 5–10%) of the benchmark machine. Nothing in the choice of rule is driven by speed.

## What changes for users

- Results of models with more than one infectious contact per agent change: infection is more likely, by the amount in the table above, and epidemics grow faster at the same parameters.
- Models whose agents have at most one infectious contact at a time, or very small $p_k$, are practically unchanged.
- The change is one of the reasons for the major version bump to 1.0.0. Parameters calibrated under the roulette should be recalibrated.

## Checking the implementation

A star graph with one susceptible center and several infectious leaves has a closed-form answer for both rules. The check used for the design (10 leaves, two viruses with $p = 0.3$ and $p = 0.05$, 40,000 replicates) reproduced the analytic values in pull and push:

| | Roulette | Independent exposures |
|---|---|---|
| $P(\text{infection})$, simulated vs analytic | 0.707 vs 0.706 | 0.870 vs 0.870 |
| Share of infections from the $p = 0.3$ virus | 0.891 vs 0.891 | 0.875 vs 0.874 |

## See Also

- [Push and Pull Transmission](transmission-sampling.md) — how the roulette is sampled from either end of a tie.
- [Sampling Contacts](sampling-contacts.md) — the probability of infection and how tools adjust it.
