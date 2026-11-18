#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE(
    "SEIRMixingQuarantine transitions 2",
    "[SEIR-mixing-quarantine-transitions-2]"
) {

    std::vector< double > contact_matrix(9, 8.0/3.0);

    int n = 2000;
    size_t n_seeds = 10;
    size_t nsims = 400;

    epimodels::ModelSEIRQuarantine model(
        "Flu", // std::string vname,
        n, // epiworld_fast_uint n,
        0.01,  // epiworld_double prevalence,
        0.5,   // epiworld_double transmission_prob,
        4.0,   // epiworld_double latent_period,
        1.0/3.5,
        .2, // Hospitalization rate
        4, // Hospitalization period
        1.5, // Days undetected (negative means no quarantine)
        15, // Quarantine period
        .9, // Quarantine willingness
        1.0, // Isolation willingness
        4, // Isolation period
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
    epiworld_double R0_expected =
        effective_contact_rate * model("Transmission probability") / 
        (model("Recovery rate") + model("Hospitalization rate"));

    epiworld_double R0 = 0.0;
    for (size_t i = 0; i < R0s.size(); ++i)
    {
        R0 += R0s[i];
    }
    R0 /= static_cast<epiworld_double>(R0s.size());
    printf_epiworld(
        "R0 observed (lower): %.2f (expected %.2f)\n", R0, R0_expected
    );

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
    const double q_to_i = to_stage(C(MQ::EXPOSED, MQ::QUARANTINED), MQ::INFECTED);

    // The share of the agents infected while free who are ever detected. A
    // free infected agent leaves Infected each day with probability e (the
    // competing recovery and severity, one roulette() draw) and, from the day
    // after its onset, is detected with probability d: (1 - e) d /
    // (1 - (1 - d)(1 - e)). Counted over runs from the declared transitions,
    // without tracing.
    const double ro = model("Recovery rate") / (1.0 - model("Recovery rate"));
    const double ho = model("Hospitalization rate") / (1.0 - model("Hospitalization rate"));
    const double e = (ro + ho) / (1.0 + ro + ho);
    const double d = 1.0 / model("Detection delay");
    const double detected_expected = (1.0 - e) * d / (1.0 - (1.0 - d) * (1.0 - e));

    // Without tracing: an agent isolated at its onset in quarantine can be
    // detected after its isolation ends, which the formula does not count
    model.set_param("Contact tracing probability", 0.0);
    double n_detected = 0.0, n_free_onsets = 0.0;
    model.run_multiple(
        60, 100, 321,
        [&](size_t, Model * m) -> void {
            auto * mq = static_cast< MQ * >(m);
            for (auto x : mq->get_agent_quarantine_triggered())
                n_detected += (x != 0u) ? 1.0 : 0.0;
            std::vector< int > date, counts;
            std::vector< std::string > name;
            m->get_db().get_hist_transitions(date, name, counts);
            for (size_t k = 0u; k < date.size(); ++k)
            {
                if (name[k] == "Exposed -> Infected")
                    n_free_onsets += counts[k];
                else if (name[k] == "Onset in quarantine")
                    n_free_onsets -= counts[k];
            }
        },
        true, false, 1
    );
    const double detected = n_detected / n_free_onsets;
    model.set_param("Contact tracing probability", 1.0);

    std::cout << "Transition to infected (exposed): " << to_i << " (expected: " <<
        1.0/model("Latent period") << ")" << std::endl;
    std::cout << "Transition to recovered (infected): " << to_r << " (expected: " <<
        ro / (1.0 + ro + ho) << ")" << std::endl;
    std::cout << "Share detected (infected while free): " << detected <<
        " (expected: " << detected_expected << ")" << std::endl;
    std::cout << "Transition to infected (q. exposed): " << q_to_i << " (expected: " <<
        1.0/model("Latent period") << ")" << std::endl;

    REQUIRE_FALSE(moreless(to_i, 1.0/model("Latent period"), 0.05));
    REQUIRE_FALSE(moreless(to_r, ro / (1.0 + ro + ho), 0.02));
    REQUIRE_FALSE(moreless(detected, detected_expected, 0.03));
    REQUIRE_FALSE(moreless(q_to_i, 1.0/model("Latent period"), 0.05));


}
