#include "tests.hpp"

using namespace epiworld;

// Infected agents in the SEIR model recover through the virus, so tools that
// enhance recovery work, as in every other model. With a recovery rate of 0
// nobody recovers on their own; a tool with a recovery enhancer of 1 makes
// every carrier recover. No transmission and an incubation of 1 day make both
// runs deterministic.
EPIWORLD_TEST_CASE("SEIR recovery follows the virus and tools", "[seir-recovery]") {

    const int n = 1000;
    const int n_seeds = 50;

    for (const bool with_tool : {false, true}) {

        epimodels::ModelSEIR<> model("virus", 0.05, 0.0, 1.0, 0.0);
        model.agents_smallworld(n, 10, false, 0.1);

        if (with_tool)
        {
            Tool<> cure("cure", 1.0, true);
            cure.set_recovery_enhancer(1.0);
            model.add_tool(cure);
        }

        model.verbose_off();
        model.run(10, 2201);

        const auto & db = model.get_db();
        CHECK(db.get_today_total("Susceptible") == n - n_seeds);
        CHECK(db.get_today_total("Exposed") == 0);
        CHECK(db.get_today_total("Infected") == (with_tool ? 0 : n_seeds));
        CHECK(db.get_today_total("Removed") == (with_tool ? n_seeds : 0));

    }

}
