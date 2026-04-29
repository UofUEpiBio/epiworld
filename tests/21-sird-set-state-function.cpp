#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SIRD via set_state_function", "[set-state-function]") {

    // Build a standard SIR model (states: 0=Susceptible, 1=Infected, 2=Recovered)
    epimodels::ModelSIR<> model(
        "a virus",
        /* prevalence        */ 0.05,
        /* transmission_rate */ 0.5,
        /* recovery_rate     */ 0.3
    );

    // Extend to SIRD by adding a "Deceased" state (index 3)
    model.add_state("Deceased");

    // Add a death rate parameter used in the bifurcating update function
    model.add_param(0.05, "Death rate");

    // Replace the "Infected" update function so that infected agents
    // bifurcate between Recovered (state 2) and Deceased (state 3)
    // at each time step.
    EPI_NEW_UPDATEFUN_LAMBDA(update_infected_sird, int) {

        // slot 0: probability of dying
        m->array_double_tmp[0u] = m->par("Death rate");

        // slot 1: probability of recovering
        m->array_double_tmp[1u] = m->par("Recovery rate");

        int which = roulette(2u, m);
        if (which < 0)
            return;

        if (which == 0) {
            // Death: remove virus and transition to state 3 (Deceased)
            p->rm_virus(*m, m->state_of("Deceased"));
        } else {
            // Recovery: remove virus and transition to post_ state (2 = Recovered)
            p->rm_virus(*m);
        }

    };

    model.set_state_function("Infected", update_infected_sird);

    model.agents_smallworld(5000, 5, false, 0.02);
    model.verbose_off();
    model.seed(12345);
    model.run(100).print();

    // Tally final state counts
    auto agents_states = model.get_agents_states();
    int n_susceptible = 0, n_infected = 0, n_recovered = 0, n_deceased = 0;
    for (auto s : agents_states) {
        if      (s == 0u) ++n_susceptible;
        else if (s == 1u) ++n_infected;
        else if (s == 2u) ++n_recovered;
        else if (s == 3u) ++n_deceased;
    }

    int total = static_cast<int>(model.size());

    // Population must be conserved across all four compartments
    REQUIRE(n_susceptible + n_infected + n_recovered + n_deceased == total);

    // The bifurcation must have produced agents in both exit states
    REQUIRE(n_recovered > 0);
    REQUIRE(n_deceased  > 0);

    // At least some agents must still be susceptible (high prevalence but
    // not 100% attack rate with recovery & death draining the epidemic)
    REQUIRE(n_susceptible > 0);

}
