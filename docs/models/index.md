# Models

Epiworld ships with a broad set of built-in epidemiological model templates. They
all build on the same core `Model<TSeq>` interface, so you can use them directly
for simulations or treat them as examples when designing your own variants.

For the full C++ declarations and member-level reference, jump from here into the
[generated API reference](../api/reference/index.html).

## Included Model Families

- **Classical compartment models** such as `ModelSIS`, `ModelSIR`, `ModelSEIR`,
  `ModelSIRD`, and `ModelSEIRD`.
- **Connected-population models** such as `ModelSIRCONN`, `ModelSEIRCONN`,
  `ModelSIRDCONN`, and `ModelSEIRDCONN`.
- **Mixing models** such as `ModelSIRMixing`, `ModelSEIRMixing`, and
  `ModelSEIRMixingQuarantine`.
- **Disease-specific or policy-oriented models** such as `ModelMeaslesSchool`,
  `ModelMeaslesMixing`, `ModelMeaslesMixingRiskQuarantine`, and `ModelSURV`.
- **Specialized variants** such as `ModelSIRLogit` and `ModelDiffNet`.

## High-Level Structure

The `epiworld::epimodels` namespace includes factory-style model templates for
common epidemiological workflows. In practice, each model exposes the same
simulation primitives you use elsewhere in the library:

- define a pathogen and its parameters,
- create or connect a population,
- run the simulation, and
- inspect or export the resulting time series and events.

## Where To Go Next

- Visit the [API overview](../api/index.html) for the generated C++ reference.
- Read [Core Simulation Models](../impl/core-simulation-models.html) for a
  conceptual guide to the built-in families.
- Read [Extending the Library with Models](../impl/extending-the-library-with-models.html)
  to build your own derived models.
