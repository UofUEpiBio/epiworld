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

    epimodels::ModelSEIRQuarantine model(
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
    Virus v1 = model.get_virus(0);
    model.rm_virus(0);

    model.add_virus(v1, dist_virus(0));

    // Creating three groups


    // Setting the distribution function of the initial cases
    model.assign_virus(0, assign::agents(first_agents(n_seeds)));

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
    // Recovering and becoming severe compete (one roulette() draw): the daily
    // probabilities of each are their odds over 1 plus the sum of the odds
    const double ro = model("Recovery rate") / (1.0 - model("Recovery rate"));
    const double ho = model("Hospitalization rate") / (1.0 - model("Hospitalization rate"));
    const double p_rec = ro / (1.0 + ro + ho);
    const double p_sev = ho / (1.0 + ro + ho);
    epiworld_double R0_expected =
        effective_contact_rate * model("Transmission probability") /
            (p_rec + p_sev);

    epiworld_double R0 = 0.0;
    for (size_t i = 0; i < R0s.size(); ++i)
    {
        R0 += R0s[i];
    }
    R0 /= static_cast<epiworld_double>(R0s.size());
    printf_epiworld("R0 observed: %.2f (expected %.2f)\n", R0, R0_expected);

    // A cell is a (stage, status) pair: stage * 4 + status. The share of the
    // agents in a cell who are in a given stage the next day, whatever their
    // status then:
    using MQ = epimodels::ModelSEIRQuarantine;
    auto C = [](int s, int k) { return static_cast< size_t >(s * 4 + k); };
    const size_t ns = model.get_n_states();
    auto to_stage = [&](size_t from, int s) {
        double x = 0.0;
        for (int k = 0; k < 4; ++k)
            x += avg_transitions[C(s, k) * ns + from];
        return x;
    };

    const double to_i = to_stage(C(MQ::EXPOSED, MQ::FREE), MQ::INFECTED);
    const double to_r = to_stage(C(MQ::INFECTED, MQ::FREE), MQ::RECOVERED);
    const double to_sv = to_stage(C(MQ::INFECTED, MQ::FREE), MQ::SEVERE);
    const double from_sv = to_stage(C(MQ::SEVERE, MQ::HOSPITALIZED), MQ::RECOVERED);

    std::cout << "Transition to infected: " << to_i << " (expected: " <<
        1.0/model("Latent period") << ")" << std::endl;
    std::cout << "Transition to recovered: " << to_r << " (expected: " <<
        p_rec << ")" << std::endl;
    std::cout << "Transition to severe: " << to_sv << " (expected: " <<
        p_sev << ")" << std::endl;
    std::cout << "Transition from severe: " << from_sv << " (expected: " <<
        (1.0 / model("Hospitalization period")) << ")" << std::endl;

    REQUIRE_FALSE(moreless(R0, R0_expected, 0.1));
    REQUIRE_FALSE(moreless(to_i, 1.0/model("Latent period"), 0.05));
    REQUIRE_FALSE(moreless(to_r, p_rec, 0.02));
    REQUIRE_FALSE(moreless(to_sv, p_sev, 0.02));
    REQUIRE_FALSE(moreless(from_sv, 1.0 / model("Hospitalization period"), 0.05));


}
