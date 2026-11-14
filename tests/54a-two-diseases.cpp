#include "tests.hpp"

using namespace epiworld;

// Two diseases on one population (several-diseases.md, test 1): each SIR,
// one infection per agent at a time. Agents recovered from one disease can
// catch the other; no agent ever carries two infections; each disease's
// counts add up to the population every day; the disease column splits the
// transmissions; a strain joins its virus's disease.
static Virus sir(const std::string & name)
{
    Virus v(name);
    const int S = v.add_stage("Susceptible");
    const int I = v.add_stage("Infected", stage::infectious);
    const int R = v.add_stage("Recovered");
    v.infection(S, I, name + " transmission");
    v.rate(I, R, name + " recovery");
    return v;
}

EPIWORLD_TEST_CASE("Two diseases", "[two-diseases]") {

    const size_t n = 4000u;
    Model model(n);
    model.verbose_off();
    model.add_param(0.08, "Flu transmission");
    model.add_param(0.08, "Flu B transmission");
    model.add_param(0.1, "Flu recovery");
    model.add_param(0.06, "Measles transmission");
    model.add_param(0.1, "Measles recovery");
    model.add_layer(layer::network("contacts", graph::smallworld(10, 0.1)));

    Virus flu = sir("Flu");
    model.add_virus(flu, assign::count(10));
    model.add_virus(sir("Measles"), assign::count(10));

    // A strain is the same disease; a virus from scratch with a taken name is
    // an error
    Virus flu_b = flu.strain("Flu B");
    flu_b.use_param("Flu transmission", "Flu B transmission");
    model.add_virus(flu_b, assign::count(5));
    REQUIRE_THROWS_AS(model.add_virus(sir("Flu")), std::logic_error);

    REQUIRE(model.get_n_diseases() == 2u);
    REQUIRE(model.get_diseases() == std::vector< std::string >{"Flu", "Measles"});
    REQUIRE(model.get_virus(0u).get_disease() == 0);
    REQUIRE(model.get_virus(1u).get_disease() == 1);
    REQUIRE(model.get_virus(2u).get_disease() == 0);

    // Every day: at most one infection per agent, in the disease of its virus
    int carriers_wrong = 0;
    model.add_globalevent([&carriers_wrong](Model * m) {
        for (auto & a : m->get_agents())
        {
            int carrying = 0;
            for (int d = 0; d < 2; ++d)
                if (m->get_natural_history(d)->is_carrier(m->get_stage(a, d)))
                {
                    ++carrying;
                    if (!a.get_virus() || (a.get_virus()->get_disease() != d))
                        ++carriers_wrong;
                }
            if ((carrying > 1) || ((carrying == 0) && a.get_virus()))
                ++carriers_wrong;
        }
    }, "one infection", -1);

    model.run(200, 77);
    const auto & db = model.get_db();
    REQUIRE(carriers_wrong == 0);

    // Each disease's counts add up to the population, every day
    for (int d = 0; d < 2; ++d)
    {
        std::vector< int > date, counts;
        std::vector< std::string > status, stage, names;
        db.get_hist_total(&date, &status, &stage, &counts, d, &names);
        std::map< int, int > by_day;
        for (size_t i = 0u; i < date.size(); ++i)
            by_day[date[i]] += counts[i];
        REQUIRE(by_day.size() == 201u);
        for (const auto & x : by_day)
            REQUIRE(x.second == static_cast< int >(n));
        REQUIRE(names == std::vector< std::string >(names.size(), model.get_diseases()[d]));
    }

    // Both outbreaks took off, and some agents had both diseases (one after
    // the other)
    int both = 0;
    for (size_t i = 0u; i < n; ++i)
        if ((model.get_episode(i, 0) > 0) && (model.get_episode(i, 1) > 0))
            ++both;
    REQUIRE(db.get_today_total_stage("Recovered", 0) > 1000);
    REQUIRE(db.get_today_total_stage("Recovered", 1) > 1000);
    REQUIRE(both > 100);

    // Two diseases have a "Recovered" stage: the total needs the disease
    REQUIRE_THROWS_AS(db.get_today_total_stage("Recovered"), std::logic_error);
    REQUIRE_THROWS_AS(model.get_stage(model.get_agent(0u)), std::logic_error);

    // The transmissions, split by the disease column: each row's virus is of
    // its disease, and asking for one disease gives its rows
    std::vector< int > date, source, target, virus, expo, layer;
    std::vector< std::string > names;
    db.get_transmissions(date, source, target, virus, expo, layer, all_diseases, &names);
    size_t n_flu = 0u;
    for (size_t i = 0u; i < virus.size(); ++i)
    {
        const int d = model.get_virus(static_cast< size_t >(virus[i])).get_disease();
        REQUIRE(names[i] == model.get_diseases()[static_cast< size_t >(d)]);
        n_flu += (d == 0) ? 1u : 0u;
    }
    db.get_transmissions(date, source, target, virus, expo, layer, 0);
    REQUIRE(date.size() == n_flu);
    REQUIRE(std::count(virus.begin(), virus.end(), 2) > 0); // Flu B spread

    // The output tables list each disease's rows under its name
    auto out = db.get_run_outputs({"total_hist", "virus_info"});
    const auto & dcol = std::get< std::vector< std::string > >(out["total_hist"].columns[0u]);
    REQUIRE(std::count(dcol.begin(), dcol.end(), "Flu") == std::count(dcol.begin(), dcol.end(), "Measles"));
    REQUIRE(
        std::get< std::vector< std::string > >(out["virus_info"].columns[0u]) ==
        std::vector< std::string >{"Flu", "Measles", "Flu"}
    );

}
