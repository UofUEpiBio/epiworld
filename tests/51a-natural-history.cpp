#include "tests.hpp"

using namespace epiworld;

// A disease built on its virus, run end to end, pulling and pushing.
//
// Stages: S, E (carrier, a fixed 3-day latent period), I (infectious; daily
// recovery and death compete, and a 6-day deadline ends the stage), R and D
// (contact weight 0). Twenty agents are seeded, half in E and half in I; a
// quarter of the others start in R. On day 5 a global event vaccinates
// (S -> R, fired by code) the susceptible agents whose id ends in 0.
//
// What must hold, whatever the random draws:
// 1. Day 0: 10 E, 10 I, 45 R, 135 S.
// 2. Every day, each stage's change equals its declared transitions in minus
//    out (the per-transition counts and the stage counts agree), and the
//    stages add up to the population.
// 3. Every agent that went from E to I spent exactly 3 days in E, and nobody
//    stayed in I more than 6 days.
// 4. The vaccination is counted on day 5 only.
// 5. Pushing gives the same bookkeeping as pulling.
// 6. Declarations that break the rules throw.

namespace {

constexpr int S = 0, E = 1, I = 2, R = 3, D = 4;
constexpr size_t N = 200u;

Virus<> make_disease()
{

    Virus<> v("Test disease");
    v.add_stage("Susceptible");
    v.add_stage("Exposed", stage::carrier);
    v.add_stage("Infected", stage::infectious);
    v.add_stage("Recovered");
    v.add_stage("Deceased");
    v.set_contact(D, 0.0);

    v.infection(S, E, "Transmission");
    v.after(E, I, Duration<>::fixed(3));
    v.rate(I, R, 0.2).effect(Effect::recovery);
    v.rate(I, D, "Death").effect(Effect::death);
    v.after(I, R, Duration<>::fixed(6)).name("End of infection");
    v.external(S, R, "Vaccinated");

    return v;

}

std::shared_ptr< Model<> > make_model(const std::string & mode)
{

    auto model = std::make_shared< Model<> >(N);
    model->add_param(0.4, "Transmission");
    model->add_param(0.05, "Death");
    model->add_virus(make_disease(), assign::counts({{E, 10u}, {I, 10u}}));
    model->set_initial(assign::shares({{R, 0.25}}));

    // A ring: each agent meets the two on each side
    std::vector< int > source, target;
    for (size_t i = 0u; i < N; ++i)
        for (size_t k = 1u; k <= 2u; ++k)
        {
            source.push_back(static_cast< int >(i));
            target.push_back(static_cast< int >((i + k) % N));
        }
    model->add_layer(layer::network("ring", AdjList(source, target, N, false)));

    const int vaccinated = model->get_natural_history()->transition_id("Vaccinated");
    model->add_globalevent(
        [vaccinated](Model<> * m) -> void {
            for (auto & a : m->get_agents())
                if ((a.get_id() % 10) == 0)
                    m->fire(vaccinated, a);
        },
        "Vaccination",
        5
    );

    model->set_transmission_mode(mode);
    model->verbose_off();
    return model;

}

} // namespace

EPIWORLD_TEST_CASE("Natural history - built and run end to end", "[natural-history]") {

    for (std::string mode : {"pull", "push"})
    {

        auto model = make_model(mode);
        const int ndays = 30;
        model->run(ndays, 123);

        const auto * nh = model->get_natural_history();
        const int ns = static_cast< int >(nh->get_n_stages());

        std::vector< int > tdate, tcount, sdate, scount;
        std::vector< std::string > tname, sname;
        model->get_db().get_hist_transitions(tdate, tname, tcount);
        model->get_db().get_hist_total(&sdate, nullptr, &sname, &scount);

        // Stage counts by day
        std::vector< std::vector< int > > count(ndays + 1, std::vector< int >(ns, 0));
        for (size_t k = 0u; k < sdate.size(); ++k)
            count[sdate[k]][nh->stage_id(sname[k])] = scount[k];

        // 1. Day 0
        REQUIRE(count[0][E] == 10);
        REQUIRE(count[0][I] == 10);
        REQUIRE(count[0][R] == 45);
        REQUIRE(count[0][S] == 135);
        REQUIRE(count[0][D] == 0);

        // 2. Changes are the transitions in minus out
        std::vector< std::vector< int > > change(ndays + 1, std::vector< int >(ns, 0));
        int n_vaccinated = 0;
        for (size_t k = 0u; k < tdate.size(); ++k)
        {
            const auto & t = nh->get_transition(nh->transition_id(tname[k]));
            change[tdate[k]][t.from[0]] -= tcount[k];
            change[tdate[k]][t.to] += tcount[k];

            // 4. The vaccination
            if (tname[k] == "Vaccinated")
            {
                REQUIRE(((tdate[k] == 5) || (tcount[k] == 0)));
                n_vaccinated += tcount[k];
            }
        }
        REQUIRE(n_vaccinated > 0);

        for (int d = 1; d <= ndays; ++d)
        {
            int total = 0;
            for (int s = 0; s < ns; ++s)
            {
                REQUIRE(count[d][s] - count[d - 1][s] == change[d][s]);
                total += count[d][s];
            }
            REQUIRE(total == static_cast< int >(N));
        }

        // 3. Durations
        int n_latent = 0;
        for (size_t i = 0u; i < N; ++i)
        {
            const int e = model->get_stage_entered(i, E);
            const int in = model->get_stage_entered(i, I);
            const int out = std::max(
                model->get_stage_entered(i, R), model->get_stage_entered(i, D)
            );

            if ((e >= 0) && (in >= 0))
            {
                REQUIRE(in - e == 3);
                ++n_latent;
            }

            if ((in >= 0) && (out >= in))
                REQUIRE(out - in <= 6);
        }
        REQUIRE(n_latent > 10);

        // The infection counts against the agents infected
        REQUIRE(model->get_db().get_n_viruses() == 1u);

    }

    // 6. Rules
    {
        Virus<> bad("Bad");
        int s = bad.add_stage("S");
        int c = bad.add_stage("C", stage::infectious);
        bad.infection(c, s, 0.1);                  // out of a carrier stage
        Model<> m(10);
        REQUIRE_THROWS(m.add_virus(bad));
    }
    {
        Virus<> bad("Bad");
        int s = bad.add_stage("S");
        int r = bad.add_stage("R");
        bad.after(s, r, Duration<>::fixed(1));
        bad.after(s, r, Duration<>::fixed(2)).name("Again");   // two durations
        Model<> m(10);
        REQUIRE_THROWS(m.add_virus(bad));
    }
    {
        Virus<> bad("Bad");
        int s = bad.add_stage("S");
        int c = bad.add_stage("C", stage::infectious);
        bad.rate(s, c, 0.1);                       // the virus without infection
        Model<> m(10);
        REQUIRE_THROWS(m.add_virus(bad));
    }
    {
        Virus<> bad("Bad");
        bad.add_stage("S");
        REQUIRE_THROWS(bad.set_contact(0, 0.5));
        REQUIRE_THROWS(bad.add_stage("S"));
    }
    {
        auto model = make_model("pull");
        REQUIRE_THROWS(model->fire(0, model->get_agent(0)));   // an infection

        // A strain: a copy that reads its own parameter is welcome; a virus
        // with other stages, or none, is not
        Virus<> strain = make_disease();
        strain.set_name("Strain");
        strain.use_param("Death", "Death (strain)");
        model->add_param(0.5, "Death (strain)");
        REQUIRE_NOTHROW(model->add_virus(strain));

        Virus<> other("Other");
        other.add_stage("Susceptible");
        REQUIRE_THROWS(model->add_virus(other));
        REQUIRE_THROWS(model->add_virus(Virus<>("Plain")));

        // Copies share the natural history until one is edited
        Virus<> a = make_disease();
        Virus<> b = a;
        REQUIRE(a.get_natural_history() == b.get_natural_history());
        b.add_stage("Extra");
        REQUIRE(a.get_natural_history() != b.get_natural_history());
        REQUIRE(a.get_natural_history()->get_n_stages() == 5u);
    }
    {
        // A status may share a stage's name: the totals ask for one or the
        // other. A transition name may not be both the virus's and the
        // statuses'.
        auto model = make_model("pull");
        Statuses<> st;
        st.add_status("Free");
        st.add_status("Recovered");
        st.external(0, 1, "Released");
        model->set_statuses(st);
        model->run(2, 123);
        const auto & db = model->get_db();
        REQUIRE(db.get_today_total_stage("Recovered") >= 45);
        REQUIRE(db.get_today_total_status("Recovered") == 0);
        REQUIRE(db.get_today_total_status("Free") == static_cast< int >(N));
        REQUIRE_THROWS(db.get_today_total_stage("Free"));
        REQUIRE_THROWS(db.get_today_total_status("Exposed"));

        Statuses<> clash;
        clash.add_status("Free");
        clash.add_status("Away");
        clash.external(0, 1, "Vaccinated");
        REQUIRE_THROWS(model->set_statuses(clash));
    }

}
