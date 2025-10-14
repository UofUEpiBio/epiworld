# ModelMeaslesMixingRiskQuarantine

A comprehensive epidemiological model that extends measles transmission modeling with risk-stratified quarantine strategies.

## Overview

`ModelMeaslesMixingRiskQuarantine` implements a measles transmission model based on the `ModelMeaslesMixing` framework, with enhanced quarantine policies that vary based on exposure risk levels. This allows for targeted public health interventions that can optimize resource allocation and epidemic control.

### Key Features

- **Measles-specific disease progression**: Susceptible → Exposed → Prodromal → Rash → Recovered
- **Population mixing**: Uses contact matrices to model heterogeneous mixing between population groups  
- **Risk-stratified quarantine**: Three-tier system (high/medium/low risk) with customizable durations
- **Enhanced detection**: Detection rate parameter that activates during quarantine periods
- **Contact tracing**: Comprehensive contact tracing with configurable success rates
- **Entity-based risk assessment**: Risk levels determined by shared entity membership
- **Vaccination effects**: Vaccine distribution with configurable efficacy

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
13. **Recovered** - Individuals who have recovered and gained immunity

## Risk Classification System

When the quarantine process is triggered by a detected case, contacts are classified into three risk levels:

### High Risk
- **Definition**: Unvaccinated agents who share entity membership with the case that triggered quarantine
- **Rationale**: Highest transmission risk due to close, prolonged contact within same household/workplace/school
- **Default quarantine duration**: 21 days (configurable)

### Medium Risk  
- **Definition**: Unvaccinated agents who had contact with infected individuals but don't share entity membership
- **Rationale**: Moderate transmission risk from community contact
- **Default quarantine duration**: 14 days (configurable)

### Low Risk
- **Definition**: All other unvaccinated agents
- **Rationale**: Lowest transmission risk, potential for community spread
- **Default quarantine duration**: 7 days (configurable)

**Note**: Vaccinated agents (those with tools) are not subject to quarantine regardless of contact patterns.

## Enhanced Detection

The model includes an enhanced detection mechanism that operates during active quarantine periods:

- **Detection rate quarantine**: Additional parameter specifying detection probability for prodromal individuals during active quarantine
- **Mechanism**: When any quarantine process is active, prodromal individuals have an additional chance of being detected and moved to quarantined prodromal state
- **Purpose**: Models increased surveillance and testing during outbreak response

## Parameters

### Disease Parameters
- **Contact rate**: Average number of contacts per step
- **Transmission rate**: Probability of transmission per contact
- **Incubation period**: Average time from exposure to becoming infectious
- **Prodromal period**: Average duration of infectious period
- **Rash period**: Average duration of rash symptoms
- **Hospitalization rate**: Probability of requiring hospitalization
- **Hospitalization period**: Average duration of hospital stay

### Detection and Isolation
- **Days undetected**: Average time before rash cases are detected
- **Isolation period**: Duration of isolation for detected cases
- **Isolation willingness**: Proportion willing to self-isolate when detected
- **Detection rate quarantine**: Detection rate during active quarantine periods

### Risk-based Quarantine
- **Quarantine period high**: Duration for high-risk contacts (days)
- **Quarantine period medium**: Duration for medium-risk contacts (days)  
- **Quarantine period low**: Duration for low-risk contacts (days)
- **Quarantine willingness**: Proportion willing to comply with quarantine

### Contact Tracing
- **Contact tracing success rate**: Probability of successfully identifying contacts
- **Contact tracing days prior**: Number of days prior to detection for tracing

### Vaccination
- **Vaccination rate**: Proportion of agents initially vaccinated
- **Vax efficacy**: Vaccine effectiveness in preventing infection
- **Vax improved recovery**: Enhanced recovery rate for vaccinated individuals

## Usage Example

```cpp
#include <epiworld/epiworld.hpp>

// Contact matrix for 3 population groups
std::vector<double> contact_matrix = {
    0.8, 0.1, 0.1,  // Group 1 mixing
    0.1, 0.8, 0.1,  // Group 2 mixing  
    0.1, 0.1, 0.8   // Group 3 mixing
};

// Create model with risk-based quarantine
epimodels::ModelMeaslesMixingRiskQuarantine<> model(
    1000,        // Population size
    0.01,        // Initial prevalence
    2.0,         // Contact rate
    0.2,         // Transmission rate
    0.9,         // Vaccination efficacy
    0.3,         // Vaccine recovery enhancement
    7.0,         // Incubation period
    4.0,         // Prodromal period
    5.0,         // Rash period
    contact_matrix, // Contact matrix
    0.2,         // Hospitalization rate
    7.0,         // Hospitalization duration  
    3.0,         // Days undetected
    21,          // Quarantine period high risk
    14,          // Quarantine period medium risk
    7,           // Quarantine period low risk
    0.8,         // Quarantine willingness
    0.8,         // Isolation willingness
    4,           // Isolation period
    0.1,         // Proportion vaccinated
    0.15,        // Detection rate during quarantine
    1.0,         // Contact tracing success rate
    4u           // Contact tracing days prior
);

// Add population entities
model.add_entity(Entity<>("Households", dist_factory<>(0, 400)));
model.add_entity(Entity<>("Schools", dist_factory<>(400, 700)));  
model.add_entity(Entity<>("Workplaces", dist_factory<>(700, 1000)));

// Set initial conditions
model.initial_states({1.0, 0.0}); // All infected start as exposed

// Run simulation
model.run(60, 123);
model.print();
```

## Differences from Other Models

### vs. ModelMeaslesMixing
- **Risk-stratified quarantine**: Three different quarantine durations based on exposure risk
- **Enhanced detection**: Additional detection during active quarantine periods  
- **Entity-based risk assessment**: Risk levels determined by shared entity membership
- **Flexible quarantine policies**: Each risk level can have different quarantine duration or be disabled

### vs. ModelMeaslesSchool  
- **Population mixing**: Supports contact matrices for heterogeneous mixing
- **Risk stratification**: Multiple quarantine strategies rather than uniform approach
- **Enhanced contact tracing**: Risk-based contact management
- **Scalability**: Supports multiple population entities with different mixing patterns

## Testing

The model includes comprehensive tests that verify:
- Correct transition probabilities between all 13 states
- Proper risk level assignment (high/medium/low)
- Enhanced detection mechanism during quarantine periods
- Different quarantine durations for different risk levels
- Comparison of uniform vs. risk-stratified quarantine strategies
- Population mixing functionality with multiple entities

See `tests/20a-measles-mixing-risk-quarantine.cpp` for test examples.

## Applications

This model is particularly useful for:

1. **Outbreak response planning**: Comparing different quarantine strategies
2. **Resource optimization**: Allocating quarantine resources based on transmission risk
3. **Policy evaluation**: Assessing effectiveness of risk-stratified interventions
4. **Contact tracing optimization**: Understanding impact of enhanced detection during outbreaks
5. **Vaccination strategy**: Evaluating how vaccination coverage affects quarantine effectiveness

## Implementation Notes

- Risk levels are assigned dynamically when quarantine is triggered
- Vaccinated agents are excluded from quarantine regardless of contact patterns
- Enhanced detection only operates when at least one quarantine process is active
- Quarantine durations can be set to -1 to disable quarantine for specific risk levels
- The model maintains backward compatibility with uniform quarantine by setting all periods equal