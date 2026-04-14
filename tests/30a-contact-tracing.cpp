#include "tests.hpp"

using namespace epiworld;

/**
 * This test validates the generalized contact tracing available in all models.
 *
 * We create a custom SIR model where the susceptible agent's update function
 * actively records contacts using the model's built-in contact tracing.
 * The transmission rate is set to 1.0 (100%), so every contact with an
 * infected agent results in transmission.
 *
 * Because transmission is guaranteed, the set of contacts recorded in the
 * contact tracing object must match exactly the set of (source, target)
 * pairs in the transmission database.
 */
EPIWORLD_TEST_CASE("Contact tracing - SIR with 100% transmission", "[contact-tracing]") {

    // -----------------------------------------------------------------------
    // Build a custom SIR model with contact tracing enabled.
    // The susceptible update function records contacts and attempts infection.
    // -----------------------------------------------------------------------

    Model<> model;

    // Susceptible update: mirrors default_update_susceptible but also records contacts
    auto update_susceptible = [](Agent<> * p, Model<> * m) -> void {

        // Retrieve the model's contact tracing object
        auto & ct = m->get_contact_tracing();

        size_t nviruses_tmp = 0u;
        for (auto & neighbor : p->get_neighbors(*m))
        {
            if (neighbor->get_virus() == nullptr)
                continue;

            auto & v = neighbor->get_virus();

            // Record the contact: infected neighbor -> susceptible agent
            ct.add_contact(
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

        // Roulette: with transmission rate = 1.0 this always fires
        int which = roulette(nviruses_tmp, m);
        if (which < 0)
            return;

        p->set_virus(*m, *m->array_virus_tmp[which]);
    };

    // Add states
    model.add_state("Susceptible", update_susceptible);
    model.add_state("Infected",    default_update_exposed<>);
    model.add_state("Recovered");

    // Parameters
    model.add_param(1.0, "Transmission rate");  // 100% transmission
    model.add_param(0.3, "Recovery rate");

    // Virus
    Virus<> virus("MyVirus", 0.05, true);
    virus.set_state(1, 2, 2);
    virus.set_prob_infecting("Transmission rate");
    virus.set_prob_recovery("Recovery rate");
    model.add_virus(virus);

    model.set_name("SIR with contact tracing");

    // Enable contact tracing BEFORE calling run()
    model.contact_tracing_on();

    // Build a small-world network and run
    model.agents_smallworld(500, 5, false, 0.01);
    model.seed(2024);
    model.verbose_off();
    model.run(50);

    // -----------------------------------------------------------------------
    // Extract transmissions from the database
    // -----------------------------------------------------------------------
    std::vector<int> t_date, t_source, t_target, t_virus, t_expo_date;
    model.get_db().get_transmissions(t_date, t_source, t_target, t_virus, t_expo_date);

    // Build a set of (source, target) transmission pairs
    std::set<std::pair<int,int>> transmission_pairs;
    for (size_t i = 0u; i < t_source.size(); ++i)
        transmission_pairs.insert({t_source[i], t_target[i]});

    // -----------------------------------------------------------------------
    // Extract contacts from the contact tracing object
    // -----------------------------------------------------------------------
    auto & ct = model.get_contact_tracing();

    // contact_pairs: (source=infected, target=susceptible) as stored by add_contact
    std::set<std::pair<int,int>> contact_pairs;
    for (size_t agent = 0u; agent < model.size(); ++agent)
    {
        size_t n = ct.get_n_contacts(agent);
        if (n > EPI_MAX_TRACKING)
            n = EPI_MAX_TRACKING;

        for (size_t j = 0u; j < n; ++j)
        {
            auto [contact_id, contact_day] = ct.get_contact(agent, j);
            // agent is the source (infected), contact_id is the target (susceptible)
            contact_pairs.insert({
                static_cast<int>(agent),
                static_cast<int>(contact_id)
            });
        }
    }

    // -----------------------------------------------------------------------
    // With 100% transmission rate, every recorded contact between an
    // infected source and a susceptible target must appear as a transmission.
    // The transmission set should be a subset of the contact set.
    // -----------------------------------------------------------------------
    REQUIRE(transmission_pairs.size() > 0u);
    REQUIRE(contact_pairs.size() > 0u);

    // Every transmission must have been a recorded contact.
    // Skip initial seed infections (source == -1), which have no contact.
    int unmatched = 0;
    for (const auto & tp : transmission_pairs)
    {
        if (tp.first == -1)
            continue;  // Initial seed infection, no contact to trace
        if (contact_pairs.find(tp) == contact_pairs.end())
            ++unmatched;
    }

    REQUIRE(unmatched == 0);

    // -----------------------------------------------------------------------
    // Verify that contact_tracing_off() correctly releases the object
    // -----------------------------------------------------------------------
    REQUIRE(model.is_contact_tracing_on());
    model.contact_tracing_off();
    REQUIRE_FALSE(model.is_contact_tracing_on());
    REQUIRE_THROWS_AS(model.get_contact_tracing(), std::logic_error);

}
