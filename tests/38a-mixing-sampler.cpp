#include "tests.hpp"

using namespace epiworld;

// A model built directly on layer::Mixing: a susceptible agent is infected
// only through the contacts the layer hands it. The contact matrix is
// column-major, entry (i, j) = contacts of group i with group j.
namespace {

class SamplerModel : public Model<>
{
public:

    SamplerModel(size_t n, std::vector< double > cm)
    {

        auto layer = layer::mixing(
            "mixing", std::vector< int >({1}), std::vector< int >({0, 1})
        );
        layer->set_contact_matrix(cm);
        add_layer(std::move(layer));

        UpdateFun<> susceptible = [](Agent<> * p, Model<> * m) -> void
        {

            auto & mixing = m->get_layer_as< layer::Mixing<> >(0u);
            const size_t n_contacts = mixing.sample(p, *m);

            // Any sampled contact is infectious: it transmits for sure
            if (n_contacts > 0u)
                p->set_virus(*m, *m->get_agent(mixing.sampled_ids()[0]).get_virus(), 1);

        };

        add_state("Susceptible", susceptible);
        add_state("Infected");

        Virus<> v("Virus", 0.0, true);
        v.set_state(1, 1, 1);
        v.set_prob_infecting(1.0);
        add_virus(v);

        queuing_off();
        set_population_size(n);

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
    REQUIRE(model.get_layer_as< layer::Mixing<> >(0u).get_n_infectious(2u) == 0u);
    REQUIRE(model.get_layer_as< layer::Mixing<> >(0u).get_n_infectious(0u) == inf[0]);

}
