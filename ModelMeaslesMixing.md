# ModelMeaslesMixing

A comprehensive epidemiological model that combines measles-specific disease progression with population mixing and quarantine measures.

## Overview

`ModelMeaslesMixing` implements a measles transmission model that extends the SEIR framework with:

- **Measles-specific disease progression**: Susceptible → Exposed → Prodromal → Rash → Recovered
- **Population mixing**: Uses contact matrices to model heterogeneous mixing between population groups
- **Infectious period**: Agents are infectious during the Prodromal state
- **Detection and isolation**: Detection occurs during the Rash state when symptoms become visible
- **Contact tracing**: Comprehensive contact tracing with configurable success rates
- **Quarantine measures**: Multiple quarantine states for exposed contacts
- **Vaccination**: Vaccine distribution with configurable efficacy and recovery enhancement

## Disease States

The model includes 13 distinct states:

1. **Susceptible** - Can become infected
2. **Exposed** - Infected but not yet infectious (incubation period)  
3. **Prodromal** - Infectious individuals (replaces "Infected" in standard SEIR)
4. **Rash** - Non-infectious with visible symptoms (detection occurs here)
5. **Isolated** - Detected individuals in self-isolation
6. **Isolated Recovered** - Recovered individuals still in isolation
7. **Detected Hospitalized** - Hospitalized individuals who were contact-traced
8. **Quarantined Exposed** - Exposed individuals in quarantine
9. **Quarantined Susceptible** - Susceptible individuals in quarantine
10. **Quarantined Prodromal** - Prodromal individuals in quarantine
11. **Quarantined Recovered** - Recovered individuals in quarantine
12. **Hospitalized** - Individuals requiring hospital care
13. **Recovered** - Immune individuals

## Key Features

### Disease Progression
- **Incubation Period**: Time from exposure to becoming infectious
- **Prodromal Period**: Duration of infectiousness before rash appears
- **Rash Period**: Duration of visible symptoms (detection window)
- **Detection**: Probabilistic detection during rash period
- **Hospitalization**: Some individuals require hospitalization

### Population Mixing
- **Contact Matrices**: Define mixing patterns between population groups
- **Heterogeneous Mixing**: Different contact rates between groups
- **Scalable**: Supports multiple population entities

### Public Health Measures
- **Contact Tracing**: Trace contacts of detected individuals
- **Quarantine**: Quarantine exposed contacts with configurable compliance
- **Isolation**: Isolate detected cases with configurable willingness
- **Vaccination**: Reduce susceptibility and enhance recovery

## Parameters

### Disease Parameters
- `contact_rate`: Average number of contacts per day
- `transmission_rate`: Probability of transmission per contact
- `incubation_period`: Average incubation period (days)
- `prodromal_period`: Average prodromal period (days)
- `rash_period`: Average rash period (days)
- `hospitalization_rate`: Probability of hospitalization
- `hospitalization_period`: Average hospitalization duration (days)

### Detection and Isolation
- `days_undetected`: Average time until detection during rash period
- `isolation_period`: Duration of isolation for detected cases
- `isolation_willingness`: Proportion willing to self-isolate

### Contact Tracing and Quarantine
- `contact_tracing_success_rate`: Probability of successfully tracing a contact
- `contact_tracing_days_prior`: Number of days to trace back
- `quarantine_period`: Duration of quarantine for contacts
- `quarantine_willingness`: Proportion willing to quarantine

### Vaccination
- `prop_vaccinated`: Proportion of population vaccinated
- `vax_efficacy`: Vaccine efficacy (reduction in susceptibility)
- `vax_reduction_recovery_rate`: Recovery enhancement from vaccination

## Usage Example

```cpp
#include "epiworld.hpp"

// Contact matrix for single homogeneous population
std::vector<double> contact_matrix = {1.0};

// Create the model
epimodels::ModelMeaslesMixing<> model(
    "Measles",        // Virus name
    1000,             // Population size
    0.005,            // Initial prevalence
    4.0,              // Contact rate
    0.9,              // Transmission rate
    0.95,             // Vaccine efficacy
    0.5,              // Vaccine recovery enhancement
    10.0,             // Incubation period
    4.0,              // Prodromal period
    5.0,              // Rash period
    contact_matrix,   // Contact matrix
    0.1,              // Hospitalization rate
    7.0,              // Hospitalization period
    2.0,              // Days undetected
    14,               // Quarantine period
    0.8,              // Quarantine willingness
    0.9,              // Isolation willingness
    10,               // Isolation period
    0.7,              // Vaccination rate
    0.8,              // Contact tracing success rate
    3u                // Contact tracing days prior
);

// Add population entity
model.add_entity(Entity<>("Population", dist_factory<>(0, 1000)));

// Run simulation
model.run(60, 123);
model.print();
```

## Differences from Other Models

### vs. ModelMeaslesSchool
- **Population Mixing**: Adds contact matrices for heterogeneous mixing
- **Contact Tracing**: Enhanced contact tracing with configurable success rates
- **Scalability**: Supports multiple population entities

### vs. ModelSEIRMixingQuarantine
- **Disease-Specific States**: Uses Prodromal/Rash instead of generic Infected
- **Detection Timing**: Detection occurs during Rash state, not Infected state
- **Infectious Period**: Only Prodromal individuals are infectious
- **Vaccination**: Includes vaccine distribution and efficacy

## Testing

The model includes comprehensive tests that verify:
- Correct transition probabilities between states
- Proper detection and quarantine mechanics
- Population mixing functionality
- Reproductive number calculations
- All quarantine states function correctly

See `tests/19-measles-mixing.cpp` for test examples.