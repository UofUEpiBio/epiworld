#include "tests.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

// Who skips quarantine is set by tool name: by default the MMR vaccine
// (routine or as PEP), not any tool. Here everyone holds a tool with no
// effect ("Other"), as an immunoglobulin recipient would: traced contacts are
// quarantined, until "Other" joins the exempt tools.
EPIWORLD_TEST_CASE(
    "Measles model with mixing: quarantine exemption by tool name",
    "[ModelMeaslesMixingExempt]"
) {

    size_t n = 2000;
    measles::ModelMeaslesMixing model(
        n,           // Number of agents
        0.005,       // Initial prevalence (10 cases)
        0.2,         // Transmission rate
        0.9,         // Vaccination efficacy
        0.3,         // Vaccination reduction recovery rate
        7.0,         // Incubation period
        4.0,         // Prodromal period
        5.0,         // Rash period
        0.0,         // Hospitalization rate
        7.0,         // Hospitalization duration
        1.0,         // Days undetected
        21,          // Quarantine period
        1.0,         // Quarantine willingness
        1.0,         // Isolation willingness
        4,           // Isolation period
        0.0,         // Proportion vaccinated
        1.0,         // Contact tracing success rate
        4u           // Contact tracing days prior
    );

    add_group_mixing(model, {10.0}, blocks({0u, n}));
    model.verbose_off();

    Tool other("Other");
    model.add_tool(other, assign::share(1.0));

    REQUIRE(
        model.get_quarantine_exempt_tools() ==
        std::vector< std::string >({"MMR", "PEP MMR"})
    );

    // Agent-days spent in quarantine over five runs
    auto quarantined = [&]() {
        int total = 0;
        for (int seed = 1; seed <= 5; ++seed)
        {
            model.run(60, seed);
            std::vector< int > date, counts;
            std::vector< std::string > status, stage;
            model.get_db().get_hist_total(&date, &status, &stage, &counts);
            for (size_t i = 0u; i < counts.size(); ++i)
                if (status[i] == "Quarantined")
                    total += counts[i];
        }
        return total;
    };

    REQUIRE(quarantined() > 0);

    model.set_quarantine_exempt_tools({"MMR", "PEP MMR", "Other"});
    REQUIRE(quarantined() == 0);

}
