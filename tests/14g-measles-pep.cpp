#include "tests.hpp"

using namespace epiworld;
using MS = epimodels::ModelMeaslesSchool<>;

namespace {

struct PepSummary {
    double avg_recipients = 0.0;
    double avg_outbreak_size = 0.0;
};

MS make_measles_model(int n_seeds, bool with_pep)
{
    MS model(
        1000,    // Number of agents
        n_seeds, // Number of initial cases
        12.0,    // Contact rate
        0.2,     // Transmission rate
        0.9,     // Vaccination efficacy
        0.3,     // Vaccination reduction recovery rate
        7.0,     // Incubation period
        4.0,     // Prodromal period
        5.0,     // Rash period
        3,       // Days undetected
        0.1,     // Hospitalization rate
        7.0,     // Hospitalization duration
        0.1,     // Proportion vaccinated
        21u,     // Quarantine period
        1.0,     // Quarantine willingness
        4u       // Isolation period
    );

    if (with_pep)
    {
        epimodels::InterventionMeaslesPEP<> pep(
            "Post-exposure prophylaxis for measles",
            1.0,       // PEP MMR efficacy
            1.0,       // PEP IG efficacy
            4.0 * 7.0, // PEP IG half-life (mean)
            7.0 / 2.0, // PEP IG half-life (sd)
            1.0,       // PEP MMR willingness
            1.0,       // PEP IG willingness
            300.0,     // PEP MMR window
            300.0,     // PEP IG window
            {
                MS::QUARANTINED_LATENT,
                MS::QUARANTINED_SUSCEPTIBLE,
                MS::QUARANTINED_PRODROMAL
            },
            {MS::RECOVERED, MS::SUSCEPTIBLE, MS::RECOVERED},
            {MS::LATENT, MS::SUSCEPTIBLE, MS::PRODROMAL}
        );

        model.add_globalevent(pep);
    }

    model.get_virus(0).set_distribution(
        [n_seeds](Virus<> & virus, Model<> * model_ptr) -> void {
            for (int i = 0; i < n_seeds; ++i)
                model_ptr->get_agents()[i].set_virus(*model_ptr, virus);
        }
    );

    return model;
}

PepSummary run_experiment(bool with_pep, size_t nsims)
{
    constexpr int n_days = 200;
    constexpr int n_seeds = 5;

    auto model = make_measles_model(n_seeds, with_pep);

    std::vector<double> recipients(nsims, 0.0);
    std::vector<double> outbreak_sizes(nsims, 0.0);

    auto saver = [&recipients, &outbreak_sizes](size_t n, Model<> * model_ptr) {
        for (auto & agent : model_ptr->get_agents())
        {
            if (agent.has_tool("PEP MMR") || agent.has_tool("PEP IG"))
                recipients[n] += 1.0;
        }

        std::vector<int> date, virus, outbreak;
        model_ptr->get_db().get_outbreak_size(date, virus, outbreak);

        REQUIRE_FALSE(outbreak.empty());

        outbreak_sizes[n] = outbreak.back();
    };

    model.run_multiple(n_days, nsims, 1231, saver, true, false, 2);

    PepSummary summary;
    summary.avg_recipients =
        std::accumulate(recipients.begin(), recipients.end(), 0.0) /
        static_cast<double>(nsims);
    summary.avg_outbreak_size =
        std::accumulate(outbreak_sizes.begin(), outbreak_sizes.end(), 0.0) /
        static_cast<double>(nsims);

    return summary;
}

} // namespace

EPIWORLD_TEST_CASE("Measles PEP reduces final outbreak size", "[ModelMeaslesPEP]") {

    constexpr size_t nsims = 300u;

    auto with_pep = run_experiment(true, nsims);
    auto without_pep = run_experiment(false, nsims);

    std::cout << "\n=== Measles PEP summary ===" << std::endl;
    std::cout << "Average PEP recipients:      "
              << with_pep.avg_recipients << std::endl;
    std::cout << "Average outbreak size w/PEP: "
              << with_pep.avg_outbreak_size << std::endl;
    std::cout << "Average outbreak size noPEP: "
              << without_pep.avg_outbreak_size << std::endl;

    REQUIRE(with_pep.avg_recipients > 0.0);
    REQUIRE(with_pep.avg_outbreak_size < without_pep.avg_outbreak_size);
}
