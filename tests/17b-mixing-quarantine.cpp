#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "SEIRMixingQuarantine transitions",
    "[SEIR-mixing-quarantine-transitions]"
) {

    std::vector< double > contact_matrix(9, 8.0/3.0);

    int n = 1000;
    size_t n_seeds = 5;
    size_t nsims = 400;

    epimodels::ModelSEIRQuarantine<> model(
        "Flu", // std::string vname,
        n, // epiworld_fast_uint n,
        0.01,  // epiworld_double prevalence,
        0.08,   // epiworld_double transmission_prob,
        4.0,   // epiworld_double latent_period,
        1.0/3.5,
        .1, // Hospitalization rate
        4, // Hospitalization period
        -1, // Days undetected (negative means no quarantine)
        4, // Quarantine period
        .9, // Quarantine willingness
        1.0, // Isolation willingness
        10, // Isolation period
        1.0, // Contact tracing success rate
        4 // Contact tracing days prior
    );
    const size_t third = size_t(n) / 3u;
    add_group_mixing(
        model, contact_matrix, blocks({0u, third, 2u * third, size_t(n)})
    );

    // Copy the original virus
    Virus<> v1 = model.get_virus(0);
    model.rm_virus(0);
    v1.set_distribution(dist_virus<>(0));

    model.add_virus(v1);

    // Creating three groups


    // Setting the distribution function of the initial cases
    model.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < static_cast<int>(n_seeds); ++i)
            m->get_agents()[i].set_virus(*m, v);
        return;
    });

    std::vector< std::vector<epiworld_double> > transitions(nsims);
    std::vector< epiworld_double > R0s(nsims * n_seeds, -1.0);
    
    // Generating the saver function
    auto saver = tests_create_saver(transitions, R0s, n_seeds);

    // Running and checking the results
    model.run_multiple(60, nsims, 123, saver, true, true, 4);
    model.print();

    // Calculate average transitions
    auto avg_transitions = tests_calculate_avg_transitions(
        transitions, model
    );

    // Checking the average transitions
    tests_print_avg_transitions(avg_transitions, model);

    // Checking the expected and effective R0
    epiworld_double effective_contact_rate =
        std::accumulate(contact_matrix.begin(), contact_matrix.end(), 0.0) / 3.0;
    epiworld_double R0_expected = 
        effective_contact_rate * model("Transmission probability") /
            (model("Recovery rate") + model("Hospitalization rate"));

    epiworld_double R0 = 0.0;
    for (size_t i = 0; i < R0s.size(); ++i)
    {
        R0 += R0s[i];
    }
    R0 /= static_cast<epiworld_double>(R0s.size());
    printf_epiworld("R0 observed: %.2f (expected %.2f)\n", R0, R0_expected);

    // Transition to Infected
    using MQ = epimodels::ModelSEIRQuarantine<>;
    #define mat(i, j) avg_transitions[j * model.get_n_states() + i]
    // Detected or not, a hospitalization
    #define hosp(i) (mat(i, MQ::HOSPITALIZED) + mat(i, MQ::DETECTED_HOSPITALIZED))

    std::cout << "Transition to infected: " <<
        mat(MQ::EXPOSED, MQ::INFECTED) << " (expected: " << 
        1.0/model("Latent period") << ")" << std::endl;

    // Transition to recovered
    std::cout << "Transition to recovered: " <<
        mat(MQ::INFECTED, MQ::RECOVERED) << " (expected: " << 
        model("Recovery rate") << ")" << std::endl;

    // Transition to hospitalized
    std::cout << "Transition to hospitalized: " <<
        hosp(MQ::INFECTED) << " (expected: " << 
        model("Hospitalization rate") << ")" << std::endl;

    // Transition from hospitalized
    std::cout << "Transition from hospitalized: " <<
        mat(MQ::HOSPITALIZED, MQ::RECOVERED) << " (expected: " << 
        (1.0 / model("Hospitalization period")) << ")" << std::endl;

    // R0
    REQUIRE_FALSE(moreless(R0, R0_expected, 0.1));

    // Transition to infected
    REQUIRE_FALSE(
        moreless(mat(MQ::EXPOSED, MQ::INFECTED), 1.0/model("Latent period"), 0.05)
    );

    // Transition to recovered
    REQUIRE_FALSE(
        moreless(mat(MQ::INFECTED, MQ::RECOVERED), model("Recovery rate"), 0.05)
    );

    // Transition to hospitalized
    REQUIRE_FALSE(
        moreless(hosp(MQ::INFECTED), model("Hospitalization rate"), 0.05)
    );

    // Transition from hospitalized
    REQUIRE_FALSE(
        moreless(mat(MQ::HOSPITALIZED, MQ::RECOVERED), 1.0 / model("Hospitalization period"), 0.05)
    );
    
    #undef mat
    #undef hosp


}
