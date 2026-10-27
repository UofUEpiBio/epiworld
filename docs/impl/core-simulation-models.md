We provide a collection of pre-built epidemiological model templates that implement classical compartmental structures and a few specialized extensions. A model is a disease: its compartments (states), its parameters, and the rules that move agents between states. Where contacts come from is up to the model's **contact layers** (see `Model::add_layer()`): a network, mixing by group with a contact matrix, contacts within groups such as households, or several of them at once. So there is one SIR model, not one per kind of contact: the same `ModelSIR` runs on a network, on a contact matrix, or on both.

The models included in the core codebase are:

- **SIS, SIR, SIRD, SEIR, SEIRD, and SISD models**, which represent standard compartmental processes with increasing biological detail.
- **`SEIRQuarantine`**, an SEIR model with detection, isolation, contact tracing (from the logs of its layers), and quarantine.
- **Disease-specific and specialized variants**, including `MeaslesMixing`, `MeaslesMixingRiskQuarantine`, `SIRLogit`, and `Surveillance`.
- **Supporting modules**, including `init-functions.hpp`, which defines helper functions for initializing model states, and `globalevents.hpp`, which provides tools for defining time-dependent interventions.

Every number a user may set or calibrate is a model parameter. The names say what the code does, in the terms epidemiology uses; most are daily rates, the probability per day of leaving a state, so the mean time in it is 1 / rate days:

| Parameter | Meaning |
|---|---|
| "Transmission probability" | Probability that one contact with an infectious agent infects (per contact, not per day). |
| "Recovery rate" | Daily probability of recovering while infectious; mean infectious period 1 / rate. |
| "Latent period" | Mean days in Exposed: infected, not yet infectious (daily probability 1 / period). |
| "Death rate" | Daily probability of dying while infectious. |
| "Contact rate" | Expected contacts per day, when a mixing layer reads it (see below). |

Parameters can be changed between runs, or during one from a global event, with:

```cpp
model.set_param("Transmission probability", 0.2);
model.set_param("Recovery rate", 0.1);
```

## SIS Model

The Susceptible–Infected–Susceptible (SIS) model describes diseases for which infection does not confer immunity. Once an agent recovers, they immediately return to the susceptible pool, meaning reinfection is possible indefinitely.

**Constructor:**
```cpp
ModelSIS(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double recovery_rate
);
```

## SIR Model

The Susceptible–Infected–Recovered (SIR) model adds immunity following recovery. Once an agent recovers, they move into the recovered (R) compartment and cannot be reinfected.

**Constructor:**
```cpp
ModelSIR(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double recovery_rate
);
```

**Example:**
```cpp
epiworld::epimodels::ModelSIR<> model("Influenza", 2000, 0.01, 0.25, 0.1);
model.add_layer(epiworld::layer::network(
    "contacts", epiworld::graph::smallworld<>(8, 0.05)
));
model.run(200);
```

## SIRD and SISD Models

The SIRD model extends SIR with a Dead (D) compartment, and SISD does the same for SIS: infected individuals may recover or die ("Death rate").

**Constructors:**
```cpp
ModelSIRD(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double recovery_rate,
    epiworld_double death_rate
);

ModelSISD(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double recovery_rate,
    epiworld_double death_rate
);
```

## SEIR and SEIRD Models

The Susceptible–Exposed–Infected–Recovered (SEIR) model adds an Exposed compartment: agents who carry the virus but are not infectious yet. Only Infected agents transmit; the model says so with `set_infectious_states()`, and every layer follows it. SEIRD adds a Dead compartment.

**Constructors:**
```cpp
ModelSEIR(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double latent_period,
    epiworld_double recovery_rate
);

ModelSEIRD(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double latent_period,
    epiworld_double recovery_rate,
    epiworld_double death_rate
);
```

## Where contacts come from

Any of the models above takes its contacts from its layers. Three common setups:

**A network.** Ties between agents, drawn by a generator at every run or given as a fixed graph:

```cpp
model.add_layer(epiworld::layer::network(
    "contacts", epiworld::graph::smallworld<>(10, 0.05)
));
```

**Mixing by group, with a contact matrix.** Entry `(i, j)` of the column-major matrix is the expected number of contacts per day of an agent in group `i` with agents in group `j` (e.g., age groups):

```cpp
auto mixing = epiworld::layer::mixing<>("community");
mixing->set_groups(groups, 2u);          // A group per agent
mixing->set_contact_matrix({8.0, 1.0, 1.0, 8.0});
model.add_layer(mixing);
```

**Random mixing.** Everyone meets "Contact rate" others a day, at random (what the former connected models, e.g. `ModelSIRCONN`, did). The contacts are the parameter times the matrix, so a global event or a calibration can change them:

```cpp
model.add_param(4.0, "Contact rate");
auto mixing = epiworld::layer::mixing<>("random mixing");
mixing->set_groups(std::vector< size_t >(model.size(), 0u), 1u);
mixing->set_contact_matrix({1.0});
mixing->set_contact_rate("Contact rate");
model.add_layer(mixing);
```

A mixing layer draws distinct contacts: an agent meets each other agent at most once a day. Every agent of a group counts in its contacts (everyone can be met), and only agents in the model's infectious states transmit. Layers combine: a household layer and a community contact matrix in the same model are exposures of one draw.

## SEIRQuarantine Model

An SEIR model with detection, isolation, contact tracing, quarantine and hospitalization, on any layers. At reset, the model turns tracing on for each of its layers (`ContactLayer::enable_tracing()`); when an infected agent is detected, its contacts in every layer's log within the tracing window are traced, and those willing to comply are quarantined (or isolated, if infected). Isolated, quarantined and hospitalized agents are out of the mixing layers altogether: they are not met, and their group's contacts go to the others.

**Constructor:**
```cpp
ModelSEIRQuarantine(
    const std::string & vname,
    epiworld_fast_uint n,
    epiworld_double prevalence,
    epiworld_double transmission_prob,
    epiworld_double latent_period,
    epiworld_double recovery_rate,
    epiworld_double hospitalization_rate,
    epiworld_double hospitalization_period,
    epiworld_double detection_delay,
    epiworld_fast_int quarantine_period,
    epiworld_double quarantine_willingness,
    epiworld_double isolation_willingness,
    epiworld_fast_int isolation_period,
    epiworld_double contact_tracing_prob = 1.0,
    epiworld_fast_uint contact_tracing_window = 4u
);
```

**Policy parameters:**
- "Detection delay": mean days infected before detection (0 detects at once, a negative value never).
- "Quarantine period" and "Isolation period": days of quarantine (from tracing) and isolation (from onset); a negative value turns them off.
- "Quarantine willingness" and "Isolation willingness": the probability that an agent complies (drawn once per agent and run).
- "Contact tracing probability": the probability that a contact is traced.
- "Contact tracing window": days before detection whose contacts are traced.
- "Hospitalization rate" and "Hospitalization period": daily probability of hospitalization while infected; mean days in hospital.

## SIRLogit Model

The SIRLogit model uses logistic regression to determine infection and recovery probabilities as functions of covariates rather than fixed rates. This allows behavioral, demographic, or contextual effects to influence transitions.

**Constructor:**
```cpp
ModelSIRLogit(
    ModelSIRLogit<TSeq> & model,
    const std::string & vname,
    double * data,
    size_t ncols,
    std::vector< double > coefs_infect,
    std::vector< double > coefs_recover,
    std::vector< size_t > coef_infect_cols,
    std::vector< size_t > coef_recover_cols,
    epiworld_double transmission_rate,
    epiworld_double recovery_rate,
    epiworld_double prevalence
);
```

**Notes:**  
- `data` is a matrix of covariates for all agents.  
- `coefs_infect` and `coefs_recover` represent regression coefficients for infection and recovery probabilities.  
- Transition probabilities are computed using the logistic function:  
  `P(event) = 1 / (1 + exp(-Xβ))`.

## Surveillance Model

The SURV (Surveillance) model integrates vaccination, infection, immunity, and observation processes. It extends basic SEIR logic with vaccination efficacy and detection mechanisms, supporting testing and reporting simulations.

**Constructor:**
```cpp
ModelSURV(
    ModelSURV<TSeq> & model,
    const std::string & vname,
    epiworld_fast_uint prevalence               = 50,
    epiworld_double efficacy_vax          = 0.9,
    epiworld_double latent_period         = 3u,
    epiworld_double infect_period         = 6u,
    epiworld_double prob_symptoms         = 0.6,
    epiworld_double prop_vaccinated       = 0.25,
    epiworld_double prop_vax_redux_transm = 0.5,
    epiworld_double prop_vax_redux_infect = 0.5,
    epiworld_double surveillance_prob     = 0.001,
    epiworld_double prob_transmission     = 1.0,
    epiworld_double prob_death            = 0.001,
    epiworld_double prob_noreinfect       = 0.9
);
```

**Interpretation:**
- Combines vaccination, infection, recovery, and death within a surveillance system.  
- Tracks detected versus undetected infections.  
- Includes immunity reduction parameters and reinfection probabilities.

## See Also

- [Extending the Library with Models](extending-the-library-with-models.md) — how to create new models by inheriting from `Model`.
- [Mixing and Entity Distribution](mixing-and-entity-distribution.md) — contact matrices and the mixing layer.
- [Sampling Contacts](sampling-contacts.md) — how a mixing layer draws contacts using binomial sampling.
- [Virus and Tool Distribution Functions](virus-and-tool-distribution.md) — mechanisms for distributing viruses and tools to agents.
- [Quarantine, Isolation, and Contact Tracing](quarantine-isolation-and-contact-tracing.md) — quarantine and isolation dynamics used by `SEIRQuarantine` and the measles models.
