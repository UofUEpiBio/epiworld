#include "tests.hpp"

using namespace epiworld;

// A model built directly on sampler::Mixing: a susceptible agent is infected
// only through the contacts the sampler hands it. The contact matrix is
// column-major, entry (i, j) = contacts of group i with group j.
namespace {

class SamplerModel :
    public Model<>,
    public ContactMatrix
{
public:

    sampler::Mixing<> mixing;

    SamplerModel(size_t n, std::vector< double > cm) :
        mixing({1}, {0, 1})
    {

        set_contact_matrix(cm, true);

        UpdateFun<> susceptible = [](Agent<> * p, Model<> * m) -> void
        {

            auto * mod = model_cast<SamplerModel, int>(m);
            const size_t n_contacts = mod->mixing.sample(p, *m, *mod);

            // Any sampled contact is infectious: it transmits for sure
            if (n_contacts > 0u)
                p->set_virus(*m, *m->get_agent(mod->mixing.sampled_ids()[0]).get_virus(), 1);

        };

        add_state("Susceptible", susceptible);
        add_state("Infected");

        add_globalevent(
            [](Model<> * m) -> void {
                model_cast<SamplerModel, int>(m)->mixing.update(*m);
            },
            "Update infectious pools"
        );

        Virus<> v("Virus", 0.0, true);
        v.set_state(1, 1, 1);
        v.set_prob_infecting(1.0);
        add_virus(v);

        queuing_off();
        agents_empty_graph(n);

    }

    void reset() override
    {
        Model<>::reset();
        validate_contact_matrix(entities.size());
        mixing.reset(*this);
    }

    std::unique_ptr< Model<> > clone_ptr() override
    {
        return std::make_unique< SamplerModel >(*this);
    }

};

}

EPIWORLD_TEST_CASE("Mixing sampler: group-specific transmission", "[mixing-sampler]") {

    // Group 0 mixes with itself, group 1 contacts group 0, group 2 only
    // contacts itself (and nobody there is infected).
    std::vector< double > cm = {
        5.0, 5.0, 0.0, // column 0: contacts with group 0
        0.0, 0.0, 0.0, // column 1
        0.0, 0.0, 5.0  // column 2: contacts with group 2
    };

    SamplerModel model(300, cm);

    Virus<> v = model.get_virus(0);
    model.rm_virus(0);
    v.set_distribution(dist_virus<>(0));
    v.set_state(1, 1, 1);
    model.add_virus(v);

    model.add_entity(Entity<>("G0", dist_factory<>(0, 100)));
    model.add_entity(Entity<>("G1", dist_factory<>(100, 200)));
    model.add_entity(Entity<>("G2", dist_factory<>(200, 300)));

    model.run(30, 331);

    size_t inf[3] = {0u, 0u, 0u};
    for (auto & a : model.get_agents())
    {
        if (a.get_state() == 1u)
            inf[a.get_entity(0, model).get_id()]++;
    }

    // Infection travels 0 -> 0 and 0 -> 1, never into group 2
    REQUIRE(inf[0] > 90u);
    REQUIRE(inf[1] > 90u);
    REQUIRE(inf[2] == 0u);

    // The infectious agent is its own pool member but is never sampled
    REQUIRE(model.mixing.get_n_infectious(2u) == 0u);
    REQUIRE(model.mixing.get_n_infectious(0u) == inf[0]);

}
