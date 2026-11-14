#include "tests.hpp"

using namespace epiworld;

// A model with one disease, and the same model with a second disease never
// seeded, give the same histories for the first (several-diseases.md, test
// 2): the engine's state of each agent combines its stages, and each
// disease's records are read from it. With statuses and a tool, so the cells
// combine a status too and the tool is counted in both diseases.
static Virus seir(const std::string & name)
{
    Virus v(name);
    const int S = v.add_stage("Susceptible");
    const int E = v.add_stage("Exposed", stage::carrier);
    const int I = v.add_stage("Infected", stage::infectious);
    const int R = v.add_stage("Recovered");
    v.infection(S, E, name + " transmission");
    v.after(E, I, Duration::geometric(3.0));
    v.rate(I, R, name + " recovery");
    return v;
}

static Model build(bool second)
{

    Model model(3000u);
    model.verbose_off();
    model.add_param(0.1, "Flu transmission");
    model.add_param(0.15, "Flu recovery");
    model.add_param(0.1, "RSV transmission");
    model.add_param(0.15, "RSV recovery");
    model.add_param(0.05, "Isolation rate");
    model.add_layer(layer::network("contacts", graph::smallworld(8, 0.1)));

    model.add_virus(seir("Flu"), assign::count(20));
    if (second)
        model.add_virus(seir("RSV"));

    Statuses st;
    const int free_ = st.add_status("Free");
    const int iso = st.add_status("Isolated");
    st.set_contact(iso, 0.0);
    st.rate(free_, iso, "Isolation rate").when({2}).of("Flu");
    st.after(iso, free_, Duration::fixed(5.0));
    model.set_statuses(st);

    Tool mask("Mask");
    mask.set_susceptibility_reduction(0.3);
    if (second)
        mask.set_targets({"Flu", "RSV"});
    model.add_tool(mask, assign::share(0.3));

    return model;

}

EPIWORLD_TEST_CASE("A second disease never seeded", "[two-diseases]") {

    Model one = build(false);
    Model two = build(true);
    REQUIRE(two.get_n_diseases() == 2u);
    REQUIRE(two.get_n_states() == one.get_n_states() * 4u);

    // The model with two diseases pulls; so does the one it is compared with
    one.set_transmission_mode("pull");

    for (int seed : {1, 2})
    {

        one.run(80, seed);
        two.run(80, seed);

        INFO("seed " << seed);
        std::vector< int > d1, c1, d2, c2;
        std::vector< std::string > k1, s1, k2, s2;
        one.get_db().get_hist_total(&d1, &k1, &s1, &c1);
        two.get_db().get_hist_total(&d2, &k2, &s2, &c2, 0);
        REQUIRE(c1 == c2);
        REQUIRE(s1 == s2);
        REQUIRE(k1 == k2);

        std::vector< int > a1, b1, t1, v1, e1, l1, a2, b2, t2, v2, e2, l2;
        one.get_db().get_transmissions(a1, b1, t1, v1, e1, l1);
        two.get_db().get_transmissions(a2, b2, t2, v2, e2, l2);
        REQUIRE(t1 == t2);
        REQUIRE(a1 == a2);

        std::vector< int > id1, id2;
        one.get_db().get_hist_tool(d1, id1, k1, s1, c1);
        two.get_db().get_hist_tool(d2, id2, k2, s2, c2, 0);
        REQUIRE(c1 == c2);

        // The disease's transitions (with several diseases, the statuses'
        // rows come once, with every disease)
        const auto * nh = one.get_natural_history();
        auto stage_rows = [nh](
            const std::vector< std::string > & names,
            const std::vector< int > & counts
        ) {
            std::vector< int > out;
            for (size_t i = 0u; i < names.size(); ++i)
                for (size_t t = 0u; t < nh->get_n_transitions(); ++t)
                    if (names[i] == nh->get_transition(static_cast< int >(t)).name)
                        out.push_back(counts[i]);
            return out;
        };
        one.get_db().get_hist_transitions(d1, s1, c1);
        two.get_db().get_hist_transitions(d2, s2, c2, 0);
        REQUIRE(stage_rows(s1, c1) == c2);
        REQUIRE(std::accumulate(c2.begin(), c2.end(), 0) > 0);

        // The second disease: everyone susceptible, every day
        two.get_db().get_hist_total(&d2, &k2, &s2, &c2, 1);
        std::map< int, int > susceptible;
        for (size_t i = 0u; i < c2.size(); ++i)
            if (s2[i] != "Susceptible")
                REQUIRE(c2[i] == 0);
            else
                susceptible[d2[i]] += c2[i];
        REQUIRE(susceptible.size() == 81u);
        for (const auto & x : susceptible)
            REQUIRE(x.second == static_cast< int >(two.size()));

    }

}
