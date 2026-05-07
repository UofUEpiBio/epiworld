
#include "tests.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "Measles model with mixing R0s",
    "[ModelMeaslesMixingOff]"
) {
    
    // Queuing doesn't matter and get results that are meaningful
    size_t nsims = 400; // Reduced for faster testing
    int n_seeds = 1;
    
    // Simple contact matrix (single group, all mixing)
    std::vector<double> contact_matrix = {4.0};
    double n_agents = 2000.0;
    size_t n_agents_sz = static_cast<size_t>(n_agents);
    
    measles::ModelMeaslesMixing<> model_0(
        n_agents_sz, // Number of agents
        n_seeds / n_agents, // Initial prevalence
        0.1,         // Transmission rate
        0.9,         // Vaccination efficacy
        0.3,         // Vaccination reduction recovery rate
        7.0,         // Incubation period
        4.0,         // Prodromal period
        5.0,         // Rash period
        contact_matrix, // Contact matrix
        0.1,         // Hospitalization rate
        7.0,         // Hospitalization duration
        3.0,         // Days undetected
        21,          // Quarantine period
        .8,          // Quarantine willingness
        .8,          // Isolation willingness
        4,           // Isolation period
        0.0,         // Proportion vaccinated
        1.0,         // Contact tracing success rate
        4u           // Contact tracing days prior
    );

    // Shutting off the quarantine and isola
    model_0.set_param("Quarantine period", -1.0);
    model_0.set_param("Isolation period", -1.0);

    // Adding a single entity (population group)
    model_0.add_entity(Entity<>("Population", dist_factory<>(0, n_agents_sz)));

    // Setting the distribution function of the initial cases
    model_0.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(*m, v);
        return;
    });

    std::vector<std::vector<epiworld_double>> transitions(nsims);
    std::vector<epiworld_double> R0s(nsims * n_seeds, -1.0);
        
    auto saver = tests_create_saver(transitions, R0s, n_seeds, nullptr, nullptr, nullptr);

    // Average R0
    auto calc_avg_r0 = [&R0s, nsims, n_seeds]() -> double {
        double sum = 0.0;
        for (auto & r0 : R0s)
        {
            if (r0 >= 0.0)
                sum += r0;
            else
                throw std::range_error(
                    "The R0 value is negative. This should not happen."
                );
        }
        return sum / static_cast<epiworld_double>(nsims * n_seeds);
    };

    auto calc_r0_theoretical = [&contact_matrix, &model_0]() -> double {
        
        // Agents exit the rash state via recovery (1/rash_period) OR
        // hospitalization (hospitalization_rate), so the expected infectious
        // days during rash is 1 / (1/rash_period + hospitalization_rate),
        // not rash_period itself.
        double effective_rash_period = 1.0 / (
            1.0 / model_0("Rash period") + model_0("Hospitalization rate")
        );

        return contact_matrix[0] * model_0("Transmission rate") * (
            model_0("Prodromal period") +
            effective_rash_period * (1.0 - model_0("Rash reduction contact rate"))
        );
    };

    /*
    E(days rash) = sum_i=0 [ (1 - h_r - 1/rash_period)^i ]
                 = 1/(1 - (1 - h_r - 1/rash_period))
                 = 1 / (1/rash_period + h_r)
    */

    auto assert_and_print_r0 = [&R0s, nsims, n_seeds, calc_avg_r0, calc_r0_theoretical](
        double delta = 0.3
    ) -> void {
        double R0_observed = calc_avg_r0();
        double R0_theoretical = calc_r0_theoretical();

        std::cout << "Reproductive number: "
                  << R0_observed << " (expected ~" << R0_theoretical << ")"
                  << std::endl;

        REQUIRE_FALSE(moreless(R0_observed, R0_theoretical, delta));
    };

    // Increasing the size of the transmission rate
    std::cout << "=============================================" << std::endl;
    std::cout << "MeaslesMixing without hospitalization R0 test" << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "R0s with no Rash transmission:" << std::endl;
    auto baseline_transmission_rate = model_0("Transmission rate");
    for (size_t i = 0; i < 4; ++i)
    {
        model_0.
            run_multiple(200, nsims, 1231, saver, true, false, 4);

        assert_and_print_r0();

        model_0.set_param("Transmission rate", model_0("Transmission rate") * 1.25);

    }

    model_0.set_param("Transmission rate", baseline_transmission_rate);
    std::cout << "\nR0s with Rash transmission:" << std::endl;
    
    model_0.set_param("Rash reduction contact rate", 1.0);
    for (size_t i = 0; i < 4; ++i)
    {
        model_0.
            run_multiple(200, nsims, 1231, saver, true, false, 4);

        std::cout << "Rash reduction: " << model_0("Rash reduction contact rate") << "; ";
        assert_and_print_r0();

        model_0.set_param("Rash reduction contact rate", 0.9 * model_0("Rash reduction contact rate"));

    }
    
    
}
