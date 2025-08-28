# ModelMeaslesEntityQuarantine

A comprehensive epidemiological model that implements measles transmission with entity-level quarantine measures, where quarantine duration varies based on vaccination prevalence within entities.

## Overview

`ModelMeaslesEntityQuarantine` extends the measles transmission framework with:

- **Measles-specific disease progression**: Susceptible → Exposed → Prodromal → Rash → Recovered
- **Population mixing**: Uses contact matrices to model heterogeneous mixing between population groups
- **Infectious period**: Agents are infectious during the Prodromal state
- **Detection and isolation**: Detection occurs during the Rash state when symptoms become visible
- **Entity-level quarantine**: When a case is detected, quarantine is applied to all eligible individuals within the same entity (no contact tracing)
- **Vaccination prevalence-based quarantine duration**: Quarantine duration varies based on vaccination prevalence within each entity
- **Isolation policies**: Individual isolation for detected cases
- **Hospitalization**: Severe cases requiring hospital care
- **Individual willingness**: Agents have individual willingness to comply with public health measures

## Key Differences from ModelMeaslesMixing

### vs. ModelMeaslesMixing
- **No Contact Tracing**: Removes complex contact tracing mechanisms
- **Entity-Level Quarantine**: Quarantine applies to entire entities rather than traced contacts
- **Dynamic Quarantine Duration**: Quarantine duration varies based on real-time vaccination prevalence calculations
- **Simplified Quarantine Logic**: More straightforward quarantine triggering and management

## Disease States

The model supports 13 distinct states:

1. **Susceptible**: Individuals who can become infected
2. **Exposed**: Infected but not yet infectious (incubation period)
3. **Prodromal**: Infectious individuals in the community (replaces "Infected" in SEIR)
4. **Rash**: Non-infectious individuals with visible symptoms (detection occurs here)
5. **Isolated**: Detected individuals in self-isolation
6. **Isolated Recovered**: Recovered individuals still in isolation
7. **Detected Hospitalized**: Hospitalized individuals who were detected
8. **Quarantined Exposed**: Exposed individuals in quarantine due to entity quarantine
9. **Quarantined Susceptible**: Susceptible individuals in quarantine due to entity quarantine
10. **Quarantined Prodromal**: Prodromal individuals in quarantine due to entity quarantine
11. **Quarantined Recovered**: Recovered individuals in quarantine due to entity quarantine
12. **Hospitalized**: Individuals requiring hospital care
13. **Recovered**: Individuals who have recovered and gained immunity

## Parameters

### Disease Parameters
- `contact_rate`: Average number of contacts per day
- `transmission_rate`: Probability of transmission per contact
- `incubation_period`: Average incubation period in days
- `prodromal_period`: Average prodromal period in days (infectious period)
- `rash_period`: Average rash period in days
- `vax_efficacy`: Vaccine efficacy for preventing infection
- `vax_improved_recovery`: Vaccine enhancement of recovery rate

### Detection and Isolation
- `days_undetected`: Average number of days an infected individual remains undetected
- `isolation_willingness`: Proportion of individuals willing to self-isolate when detected
- `isolation_period`: Duration of isolation in days for detected infected individuals

### Entity-Level Quarantine
- `quarantine_willingness`: Proportion of individuals willing to comply with quarantine measures
- `vaccination_prevalence_thresholds`: Vector of vaccination prevalence thresholds (0.0 to 1.0)
- `quarantine_durations`: Vector of quarantine durations (days) corresponding to thresholds

### Hospitalization
- `hospitalization_rate`: Rate at which infected individuals are hospitalized
- `hospitalization_period`: Average duration of hospitalization in days

### Vaccination
- `prop_vaccinated`: Initial proportion of vaccinated agents
- `vaccination_prevalence_thresholds`: Thresholds for determining quarantine duration
- `quarantine_durations`: Corresponding quarantine durations for each threshold

## Usage Example

```cpp
#include <epiworld/epiworld.hpp>

using namespace epiworld;

// Contact matrix for population mixing (3 entities example)
std::vector<double> contact_matrix = {
    1.0, 0.5, 0.2,  // Entity 0 contacts
    0.5, 1.0, 0.3,  // Entity 1 contacts
    0.2, 0.3, 1.0   // Entity 2 contacts
};

// Vaccination prevalence thresholds and corresponding quarantine durations
std::vector<double> vax_thresholds = {0.0, 0.5, 0.8};
std::vector<size_t> quarantine_durations = {21, 14, 7}; // Days

// Create the model
epimodels::ModelMeaslesEntityQuarantine<> model(
    1000,            // Number of agents
    0.01,            // Initial prevalence
    2.0,             // Contact rate
    0.2,             // Transmission rate
    0.9,             // Vaccination efficacy
    0.3,             // Vaccination reduction recovery rate
    7.0,             // Incubation period
    4.0,             // Prodromal period
    5.0,             // Rash period
    contact_matrix,  // Contact matrix
    0.2,             // Hospitalization rate
    7.0,             // Hospitalization duration
    3.0,             // Days undetected
    0.8,             // Quarantine willingness
    0.8,             // Isolation willingness
    4,               // Isolation period
    0.3,             // Proportion vaccinated
    vax_thresholds,      // Vaccination prevalence thresholds
    quarantine_durations // Quarantine durations
);

// Add population entities
model.add_entity(Entity<>("Community A", dist_factory<>(0, 300)));
model.add_entity(Entity<>("Community B", dist_factory<>(300, 700)));
model.add_entity(Entity<>("Community C", dist_factory<>(700, 1000)));

// Initialize entity data structures
model.initialize_entity_data();

// Run simulation
model.run(60, 123);
model.print();
```

## Quarantine Duration Logic

The quarantine duration for each entity is determined dynamically based on vaccination prevalence:

1. **Calculate vaccination prevalence** for the entity (proportion of agents with vaccination tools)
2. **Find matching threshold** in the `vaccination_prevalence_thresholds` vector
3. **Apply corresponding duration** from the `quarantine_durations` vector

For example, with thresholds `[0.0, 0.5, 0.8]` and durations `[21, 14, 7]`:
- Entity with 30% vaccination → 21 days quarantine
- Entity with 60% vaccination → 14 days quarantine  
- Entity with 90% vaccination → 7 days quarantine

## Entity-Level Quarantine Process

When a case is detected (agent reaches Rash state):

1. **Entity quarantine triggered** for the entity containing the detected case
2. **All eligible agents** in that entity are considered for quarantine:
   - Must be in Susceptible, Exposed, Prodromal, or Rash state
   - Vaccinated agents (those with tools) are exempt
   - Subject to individual quarantine willingness
3. **Quarantine duration** determined by current vaccination prevalence in the entity
4. **State transitions** to appropriate quarantined states

## Testing

The model includes comprehensive tests that verify:
- Correct entity-level quarantine behavior
- Vaccination prevalence calculation accuracy
- Dynamic quarantine duration determination
- Population mixing functionality across multiple entities
- All disease state transitions function correctly

See `tests/20a-measles-entity-quarantine.cpp`, `tests/20b-measles-entity-quarantine.cpp`, and `tests/20c-measles-entity-quarantine.cpp` for test examples.

## Implementation Notes

- **No Contact Tracing**: Unlike `ModelMeaslesMixing`, this model does not implement contact tracing
- **Real-time Calculations**: Vaccination prevalence and quarantine durations are calculated dynamically during simulation
- **Entity Initialization**: Call `initialize_entity_data()` after adding all entities to set up mixing infrastructure
- **Memory Efficient**: Removes tracking matrices and contact tracing data structures from the original model