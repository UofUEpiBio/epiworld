#include "tests.hpp"

using namespace epiworld;

/**
 * This test validates ContactTracing::get_contacts(), which provides a
 * per-agent view of all unique contacts together with the set of days on
 * which each contact was recorded.
 *
 * Strategy:
 *  1. Build a ContactTracing object directly and add known contacts so we
 *     have full control over the expected results.
 *  2. Verify that get_contacts() aggregates contacts by contact_id and
 *     collects all days correctly.
 *  3. Verify that multi-day contacts (same pair, different days) are
 *     collected into a single ContactRecord with multiple days in its
 *     times set.
 *  4. Verify caching: calling get_contacts() twice without an intervening
 *     add_contact() returns the same object address.
 *  5. Verify cache invalidation: adding a new contact causes the next call
 *     to get_contacts() to rebuild the cache.
 *  6. Cross-check against the low-level get_contact() API on a model run.
 */
EPIWORLD_TEST_CASE("ContactTracing::get_contacts()", "[contact-tracing-get-contacts]") {

    // -----------------------------------------------------------------------
    // Unit test with a manually-constructed ContactTracing object.
    // n_agents = 5, max_contacts = 10 (plenty of room, no wraparound).
    // -----------------------------------------------------------------------
    ContactTracing ct(5u, 10u);

    // Agent 0 contacts agent 2 on days 1, 3, and 5.
    ct.add_contact(0u, 2u, 1u);
    ct.add_contact(0u, 2u, 3u);
    ct.add_contact(0u, 2u, 5u);

    // Agent 0 also contacts agent 4 on day 2.
    ct.add_contact(0u, 4u, 2u);

    // Agent 1 contacts agent 3 on day 0.
    ct.add_contact(1u, 3u, 0u);

    // Agent 2 has no contacts.

    // -----------------------------------------------------------------------
    // Verify agent 0: should have exactly 2 unique contacts (agent 2 and 4).
    // -----------------------------------------------------------------------
    const auto & contacts_0 = ct.get_contacts(0u);
    std::cout << "[Agent 0] Expected 2 unique contacts, observed: "
              << contacts_0.size() << "\n";
    REQUIRE(contacts_0.size() == 2u);

    // Build a map from contact_id -> times for easy lookup.
    std::map<size_t, std::set<int>> map_0;
    for (const auto & rec : contacts_0)
        map_0[rec.get_contact_id()] = rec.get_times();

    // Agent 2 should appear with days {1, 3, 5}.
    std::cout << "[Agent 0 -> Agent 2] Expected days {1, 3, 5}, observed: {";
    for (int d : map_0[2u]) std::cout << d << " ";
    std::cout << "}\n";
    REQUIRE(map_0.count(2u) == 1u);
    REQUIRE(map_0.at(2u) == (std::set<int>{1, 3, 5}));

    // Agent 4 should appear with day {2}.
    std::cout << "[Agent 0 -> Agent 4] Expected days {2}, observed: {";
    for (int d : map_0[4u]) std::cout << d << " ";
    std::cout << "}\n";
    REQUIRE(map_0.count(4u) == 1u);
    REQUIRE(map_0.at(4u) == (std::set<int>{2}));

    // -----------------------------------------------------------------------
    // Verify agent 1: one unique contact (agent 3, day 0).
    // -----------------------------------------------------------------------
    const auto & contacts_1 = ct.get_contacts(1u);
    std::cout << "[Agent 1] Expected 1 unique contact (agent 3, day 0), observed: "
              << contacts_1.size() << " contact(s)";
    if (!contacts_1.empty())
        std::cout << ", contact_id=" << contacts_1[0].get_contact_id()
                  << ", day=" << *contacts_1[0].get_times().begin();
    std::cout << "\n";
    REQUIRE(contacts_1.size() == 1u);
    REQUIRE(contacts_1[0].get_contact_id() == 3u);
    REQUIRE(contacts_1[0].get_times() == (std::set<int>{0}));

    // -----------------------------------------------------------------------
    // Verify agent 2: no contacts recorded.
    // -----------------------------------------------------------------------
    std::cout << "[Agent 2] Expected 0 contacts, observed: "
              << ct.get_contacts(2u).size() << "\n";
    REQUIRE(ct.get_contacts(2u).empty());

    // -----------------------------------------------------------------------
    // Caching: two consecutive calls must return the same address.
    // -----------------------------------------------------------------------
    const auto & first_call  = ct.get_contacts(0u);
    const auto & second_call = ct.get_contacts(0u);
    std::cout << "[Caching] Expected same address for consecutive get_contacts(0) calls: "
              << (&first_call == &second_call ? "PASS" : "FAIL") << "\n";
    REQUIRE(&first_call == &second_call);

    // -----------------------------------------------------------------------
    // Cache invalidation: adding a new contact for agent 0 must produce a
    // fresh result on the next get_contacts() call.
    // -----------------------------------------------------------------------
    ct.add_contact(0u, 3u, 7u);  // new contact: agent 0 -> agent 3, day 7
    const auto & after_add = ct.get_contacts(0u);

    // Now agent 0 should have 3 unique contacts.
    std::cout << "[Cache invalidation] Agent 0 expected 3 unique contacts after adding "
                 "agent 3 on day 7, observed: " << after_add.size() << "\n";
    REQUIRE(after_add.size() == 3u);

    // Day 7 must appear under contact_id 3.
    bool found = false;
    for (const auto & rec : after_add)
    {
        if (rec.get_contact_id() == 3u)
        {
            found = rec.get_times().count(7) > 0u;
            break;
        }
    }
    std::cout << "[Cache invalidation] Agent 0 -> Agent 3 should include day 7: "
              << (found ? "PASS" : "FAIL") << "\n";
    REQUIRE(found);

    // -----------------------------------------------------------------------
    // Cross-check: run a small SIR model and verify that get_contacts()
    // returns exactly the same (agent, contact_id, day) triples as the
    // low-level get_contact() API.
    // -----------------------------------------------------------------------
    Model<> model;

    auto update_susceptible = [](Agent<> * p, Model<> * m) -> void {

        auto & model_ct = m->get_contact_tracing();

        size_t nviruses_tmp = 0u;
        for (auto & neighbor : p->get_neighbors(*m))
        {
            if (neighbor->get_virus() == nullptr)
                continue;

            auto & v = neighbor->get_virus();

            model_ct.add_contact(
                neighbor->get_id(),
                p->get_id(),
                static_cast<size_t>(m->today())
            );

            m->array_double_tmp[nviruses_tmp] =
                (1.0 - p->get_susceptibility_reduction(v, *m)) *
                v->get_prob_infecting(m) *
                (1.0 - neighbor->get_transmission_reduction(v, *m));

            m->array_virus_tmp[nviruses_tmp++] = &(*v);
        }

        if (nviruses_tmp == 0u)
            return;

        int which = roulette(nviruses_tmp, m);
        if (which < 0)
            return;

        p->set_virus(*m, *m->array_virus_tmp[which]);
    };

    model.add_state("Susceptible", update_susceptible);
    model.add_state("Infected",    default_update_exposed<>);
    model.add_state("Recovered");

    model.add_param(1.0, "Transmission rate");
    model.add_param(0.3, "Recovery rate");

    Virus<> virus("MyVirus", 0.05, true);
    virus.set_state(1, 2, 2);
    virus.set_prob_infecting("Transmission rate");
    virus.set_prob_recovery("Recovery rate");
    model.add_virus(virus);

    model.set_name("SIR get_contacts test");
    model.contact_tracing_on();
    model.agents_smallworld(200, 5, false, 0.01);
    model.seed(42);
    model.verbose_off();
    model.run(30);

    auto & model_ct = model.get_contact_tracing();

    // Ground truth: triples from low-level API.
    using Triple = std::tuple<size_t, size_t, int>;
    std::set<Triple> ground_truth;

    for (size_t agent = 0u; agent < model.size(); ++agent)
    {
        size_t n = model_ct.get_n_contacts(agent);
        if (n > EPI_MAX_TRACKING)
            n = EPI_MAX_TRACKING;

        for (size_t j = 0u; j < n; ++j)
        {
            auto [cid, cday] = model_ct.get_contact(agent, j);
            ground_truth.emplace(agent, cid, cday);
        }
    }

    // From get_contacts(): unpack each ContactRecord into individual triples.
    std::set<Triple> from_get_contacts;

    for (size_t agent = 0u; agent < model.size(); ++agent)
    {
        for (const auto & rec : model_ct.get_contacts(agent))
        {
            for (int day : rec.get_times())
                from_get_contacts.emplace(agent, rec.get_contact_id(), day);
        }
    }

    std::cout << "[Cross-check] Low-level API triples: " << ground_truth.size()
              << ", get_contacts() triples: " << from_get_contacts.size()
              << " (expected equal and > 0)\n";
    REQUIRE(ground_truth == from_get_contacts);
    REQUIRE(ground_truth.size() > 0u);
}

