#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("State update transition factory", "[state-update-transition]") {

    // ----------------------------------------------------------------
    // Test 1: Validate input checks
    // ----------------------------------------------------------------
    REQUIRE_THROWS_AS(
        new_state_update_transition<int>(
            {"A->B rate"},
            {1, 2}  // mismatch: 1 param name vs 2 target states
        ),
        std::logic_error
    );

    REQUIRE_THROWS_AS(
        new_state_update_transition<int>({}, {}),
        std::logic_error
    );

    // ----------------------------------------------------------------
    // Test 2: Build an E-I-R-D model with the factory and verify
    //         transition probabilities converge to the expected values.
    //
    //   E --rate--> I --rate--> R
    //                \--rate--> D
    //
    // All agents start in E (state 0). The transition matrix records
    // an initial day 0 where all agents "stay" (no update fires), so
    // the empirical E->I probability converges to
    //     p / (1 + p)
    // rather than the raw rate p for absorbing chains.
    //
    // For state I (which starts empty), there is no such bias and the
    // observed probabilities match the roulette expectations directly.
    // ----------------------------------------------------------------

    Model<int> model;

    constexpr epiworld_fast_uint I = 1u;
    constexpr epiworld_fast_uint R = 2u;
    constexpr epiworld_fast_uint D = 3u;

    epiworld_double ei_rate = 0.3;
    epiworld_double ir_rate = 0.5;
    epiworld_double id_rate = 0.1;

    auto update_exposed = new_state_update_transition<int>(
        {"E->I transition rate"},
        {I}
    );

    auto update_infected = new_state_update_transition<int>(
        {"I->R transition rate", "I->D transition rate"},
        {R, D}
    );

    model.add_state("Exposed",   update_exposed);
    model.add_state("Infected",  update_infected);
    model.add_state("Recovered");
    model.add_state("Deceased");

    model.add_param(ei_rate, "E->I transition rate");
    model.add_param(ir_rate, "I->R transition rate");
    model.add_param(id_rate, "I->D transition rate");

    // Large population so the transition probabilities converge
    model.agents_empty_graph(10000);
    model.queuing_off();

    // Run long enough to get stable transition probabilities
    model.run(5000, 123);

    auto tprobs = model.get_db().get_transition_probability(false, true);

    // The transition matrix uses column-major layout:
    //   index = from + to * n_state
    size_t n = 4u;

    // E->I: all agents start in E, so day 0 records N stays with 0
    // transitions. The converged empirical probability is p/(1+p).
    epiworld_double exp_ei = ei_rate / (1.0 + ei_rate);

    REQUIRE_THAT(
        tprobs[0 + I * n],   // E->I
        Catch::WithinAbs(static_cast<double>(exp_ei), 0.02)
    );

    REQUIRE_THAT(
        tprobs[0 + 0 * n],   // E->E (stay)
        Catch::WithinAbs(static_cast<double>(1.0 - exp_ei), 0.02)
    );

    // I->R and I->D: state I starts empty, so no day-0 bias.
    // The roulette conditional probabilities apply directly:
    //   p_none = (1 - ir) * (1 - id)
    //   P(I->R) = ir * p_none / (1-ir) / Z
    //   P(I->D) = id * p_none / (1-id) / Z
    //   Z = p_none + ir * p_none/(1-ir) + id * p_none/(1-id)
    epiworld_double p_none_i = (1.0 - ir_rate) * (1.0 - id_rate);
    epiworld_double ir_adj   = ir_rate * p_none_i / (1.0 - ir_rate);
    epiworld_double id_adj   = id_rate * p_none_i / (1.0 - id_rate);
    epiworld_double p_total  = p_none_i + ir_adj + id_adj;

    epiworld_double exp_stay_i = p_none_i / p_total;
    epiworld_double exp_ir     = ir_adj / p_total;
    epiworld_double exp_id     = id_adj / p_total;

    REQUIRE_THAT(
        tprobs[I + I * n],   // I->I (stay)
        Catch::WithinAbs(static_cast<double>(exp_stay_i), 0.02)
    );

    REQUIRE_THAT(
        tprobs[I + R * n],   // I->R
        Catch::WithinAbs(static_cast<double>(exp_ir), 0.02)
    );

    REQUIRE_THAT(
        tprobs[I + D * n],   // I->D
        Catch::WithinAbs(static_cast<double>(exp_id), 0.02)
    );

    // R and D are absorbing states
    REQUIRE_THAT(
        tprobs[R + R * n],   // R->R
        Catch::WithinAbs(1.0, 0.001)
    );

    REQUIRE_THAT(
        tprobs[D + D * n],   // D->D
        Catch::WithinAbs(1.0, 0.001)
    );

}
