# SEIRD Transitions

This example demonstrates how to build a SEIRD
(Susceptible-Exposed-Infected-Recovered-Deceased) model using the
`new_state_update_transition` factory function instead of hand-writing
lambda state-update functions.

## Model Structure

States: **S → E → I → R** with a branching **I → D** death pathway.

The `new_state_update_transition` factory generates the update functions for
the Exposed and Infected states from a simple transition-rate table, making the
model definition concise and easy to modify.

## Source

See [main.cpp](main.cpp) for the full source code.
