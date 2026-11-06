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

        // Three groups of n / 3
        auto layer = layer::mixing("mixing");
        layer->set_groups(assign::labels(blocks({0u, n / 3u, 2u * n / 3u, n})), 3u);
        layer->set_contact_matrix(cm);
        add_layer(std::move(layer));

        // Any sampled contact is infectious: it transmits for sure
        Virus<> v("Virus");
        v.add_stage("Susceptible");
        v.add_stage("Infected", stage::infectious);
        v.infection(0, 1, 1.0).sampler(
            [](Agent<> & p, Model<> & m) -> Virus<> * {
                auto & mixing = m.get_layer_as< layer::Mixing<> >(0u);
                std::vector< size_t > contacts;
                mixing.sample(p.get_id(), m, nullptr, contacts);
                if (contacts.empty())
                    return nullptr;
                return m.get_agent(contacts[0]).get_virus().get();
            }
        );
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

    model.assign_virus(0, dist_virus<>(0));

    model.run(30, 331);

    size_t inf[3] = {0u, 0u, 0u};
    for (auto & a : model.get_agents())
    {
        if (model.get_stage(a) == 1)
            inf[model.get_layer_as< layer::Mixing<> >(0u).get_group(a.get_id())]++;
    }

    // Infection travels 0 -> 0 and 0 -> 1, never into group 2
    REQUIRE(inf[0] > 90u);
    REQUIRE(inf[1] > 90u);
    REQUIRE(inf[2] == 0u);

    // The infectious agent is its own pool member but is never sampled
    REQUIRE(model.get_layer_as< layer::Mixing<> >(0u).get_n_infectious(2u) == 0u);
    REQUIRE(model.get_layer_as< layer::Mixing<> >(0u).get_n_infectious(0u) == inf[0]);

}
