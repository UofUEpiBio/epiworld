#include "tests.hpp"

using namespace epiworld;

// The rules of several diseases (several-diseases.md, test 3): a death in one
// disease is "Removed (other disease)" in the others; what a model with one
// disease infers must be named with several (a stage referred to without its
// disease, a tool without targets) and pushing is refused, each with its
// message; code cannot give an agent a second infection; a tool acts on the
// diseases it targets only.
static Virus sird(const std::string & name, bool deadly)
{
    Virus v(name);
    const int S = v.add_stage("Susceptible");
    const int I = v.add_stage("Infected", stage::infectious);
    const int R = v.add_stage("Recovered");
    v.infection(S, I, name + " transmission");
    v.rate(I, R, name + " recovery");
    if (deadly)
    {
        const int D = v.add_stage("Dead");
        v.set_contact(D, 0.0);
        v.rate(I, D, name + " death");
    }
    v.external(R, S, "Waning " + name).name("Waning " + name);
    return v;
}

static Model build(size_t n = 3000u)
{
    Model model(n);
    model.verbose_off();
    model.add_param(0.1, "Ebola transmission");
    model.add_param(0.05, "Ebola recovery");
    model.add_param(0.05, "Ebola death");
    model.add_param(0.1, "Flu transmission");
    model.add_param(0.1, "Flu recovery");
    model.add_layer(layer::network("contacts", graph::smallworld(10, 0.1)));
    model.add_virus(sird("Ebola", true), assign::count(20));
    model.add_virus(sird("Flu", false), assign::count(20));
    return model;
}

EPIWORLD_TEST_CASE("Rules of several diseases", "[two-diseases]") {

    Model model = build();

    // Ebola kills, so Flu has "Removed (other disease)"; Ebola does not
    const auto & flu_stages = model.get_stages(1);
    REQUIRE(flu_stages.back() == "Removed (other disease)");
    REQUIRE(model.get_stages(0).back() == "Dead");

    model.run(150, 3);
    const auto & db = model.get_db();

    // Everyone dead of Ebola is "Removed (other disease)" in Flu, and nobody
    // else is
    const int dead = db.get_today_total_stage("Dead", 0);
    REQUIRE(dead > 50);
    REQUIRE(db.get_today_total_stage("Removed (other disease)", 1) == dead);
    for (auto & a : model.get_agents())
        REQUIRE(
            (model.get_stage(a, 0) == 3) ==
            (model.get_stage(a, 1) == static_cast< int >(flu_stages.size()) - 1)
        );

    // A stage referred to without its disease, with several
    {
        Model m = build(500u);
        m.react(Trigger::entered(2), m.transition_id("Waning Flu"));
        REQUIRE_THROWS_WITH(
            m.run(5, 1),
            Catch::Matchers::Contains("add .of(\"<disease>\")")
        );
    }
    {
        Model m = build(500u);
        m.react(Trigger::entered(2).of("Flu"), m.transition_id("Waning Flu"));
        REQUIRE_NOTHROW(m.run(5, 1));
    }

    // A tool without targets, with several
    {
        Model m = build(500u);
        Tool t("Mask");
        t.set_susceptibility_reduction(0.5);
        m.add_tool(t, assign::share(0.5));
        REQUIRE_THROWS_WITH(
            m.run(5, 1),
            Catch::Matchers::Contains("add them with add_target()")
        );
        m.get_tool(0u).add_target("Flu");
        REQUIRE_NOTHROW(m.run(5, 1));
        m.get_tool(0u).add_target("Measles");
        REQUIRE_THROWS_AS(m.run(5, 1), std::range_error);
    }

    // Pushing, with several
    {
        Model m = build(500u);
        m.set_transmission_mode("push");
        REQUIRE_THROWS_WITH(
            m.run(5, 1),
            Catch::Matchers::Contains("pulls infections")
        );
    }

    // Code cannot give an agent a second infection: a Flu virus given to an
    // agent carrying Ebola is refused when the events apply (a declared
    // transition cannot give the virus either: only an infection or seeding
    // enters a carrier stage from a stage without it)
    {
        Model m = build(200u);
        m.set_param("Ebola recovery", 0.0);
        m.set_param("Ebola death", 0.0);
        m.run(0, 1);

        Agent * carrier = nullptr;
        Agent * other = nullptr;
        for (auto & x : m.get_agents())
        {
            if (x.get_virus() && (x.get_virus()->get_disease() == 0))
                carrier = &x;
            else if (!x.get_virus())
                other = &x;
        }
        REQUIRE(carrier != nullptr);
        REQUIRE(other != nullptr);

        carrier->set_virus(m, m.get_virus(1u));
        other->set_virus(m, m.get_virus(1u));
        m.events_run();
        REQUIRE(carrier->get_virus()->get_disease() == 0);
        REQUIRE(m.get_stage(*carrier, 1) == 0);
        REQUIRE(other->get_virus()->get_disease() == 1);
        REQUIRE(m.get_stage(*other, 1) == 1);
    }

    // A tool acts on the diseases it targets only: a perfect Flu vaccine for
    // everyone stops Flu, not Ebola
    {
        Model m = build(2000u);
        Tool vax("Flu vaccine");
        vax.set_susceptibility_reduction(1.0);
        vax.add_target("Flu");
        m.add_tool(vax, assign::share(1.0));
        m.run(60, 5);
        std::vector< int > date, source, target, virus, expo, layer;
        m.get_db().get_transmissions(date, source, target, virus, expo, layer, 1);
        REQUIRE(std::count_if(source.begin(), source.end(), [](int s) { return s >= 0; }) == 0);
        m.get_db().get_transmissions(date, source, target, virus, expo, layer, 0);
        REQUIRE(std::count_if(source.begin(), source.end(), [](int s) { return s >= 0; }) > 0);

        // Its history is in Flu only
        std::vector< int > tdate, tid, tcounts;
        std::vector< std::string > tstatus, tstage;
        m.get_db().get_hist_tool(tdate, tid, tstatus, tstage, tcounts, 0);
        REQUIRE(tdate.empty());
        m.get_db().get_hist_tool(tdate, tid, tstatus, tstage, tcounts, 1);
        REQUIRE(std::accumulate(tcounts.begin(), tcounts.end(), 0) == 2000 * 61);
    }

}
