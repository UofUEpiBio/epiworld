#include "tests.hpp"

using namespace epiworld;

// The built-in models that had no test, on their natural histories, end to
// end on a fixed network:
// - ModelSURV: nobody out of the infected stages holds a virus (the removed
//   used to keep theirs, and transmit); the surveillance program isolates,
//   and the isolated stay isolated until they recover or die; a branch splits
//   the latent agents between symptomatic and asymptomatic about as
//   "Prob of symptoms" says.
// - ModelDiffNet: adoption spreads, and an adopter never stops adopting.
// - ModelSIRLogit: with a large recovery coefficient, the infected recover
//   the next day.

namespace {

constexpr size_t N = 3000u;

template<typename TModel>
void add_net(TModel & model, const AdjList & net)
{
    model.add_layer(layer::network("net", net));
    model.verbose_off();
}

} // namespace

EPIWORLD_TEST_CASE("Natural history - surveillance and diffnet and logit models", "[natural-history]") {

    Model<> scratch(N);
    scratch.seed(5);
    const AdjList net = rgraph_smallworld(N, 6, 0.05, false, scratch);

    // ModelSURV -------------------------------------------------------------
    {
        using M = epimodels::ModelSURV<>;
        M model(
            "covid", N, 30u, 0.9, 3.0, 6.0, 0.6, 0.25, 0.5, 0.5,
            0.05, 0.5, 0.01, 0.9
        );
        add_net(model, net);
        model.run(60, 11);

        const auto * nh = model.get_natural_history();
        for (auto & a : model.get_agents())
            REQUIRE(
                (a.get_virus() != nullptr) ==
                nh->is_carrier(static_cast< int >(a.get_state()))
            );

        std::vector< int > date, counts;
        std::vector< std::string > name;
        model.get_db().get_hist_transitions(date, name, counts);

        int detected = 0, to_sym = 0, to_asym = 0, latent_out = 0;
        for (size_t k = 0u; k < date.size(); ++k)
        {
            if ((name[k] == "Detected") || (name[k] == "Detected asymptomatic"))
                detected += counts[k];
            if (name[k] == "Latent -> Symptomatic or Asymptomatic")
                latent_out += counts[k];
        }
        REQUIRE(detected > 0);

        // The branch, from the agents' entry days
        for (size_t i = 0u; i < N; ++i)
        {
            if (model.get_stage_entered(i, M::SYMPTOMATIC) >= 0)
                ++to_sym;
            else if (model.get_stage_entered(i, M::ASYMPTOMATIC) >= 0)
                ++to_asym;
        }
        REQUIRE(latent_out > 300);
        const double share = static_cast< double >(to_sym) /
            static_cast< double >(to_sym + to_asym);
        REQUIRE(std::abs(share - 0.6) < 0.06);
    }

    // ModelDiffNet ----------------------------------------------------------
    {
        epimodels::ModelDiffNet<> model("idea", N, 0.02, 0.02);
        add_net(model, net);
        model.run(40, 2);

        std::vector< int > date, counts;
        std::vector< std::string > state;
        model.get_db().get_hist_total(&date, &state, &counts);

        int last = -1;
        for (size_t k = 0u; k < date.size(); ++k)
            if (state[k] == "Adopters")
            {
                REQUIRE(counts[k] >= last);
                last = counts[k];
            }
        REQUIRE(last > 5 * 60);  // from 60 adopters
    }

    // ModelSIRLogit ---------------------------------------------------------
    {
        std::vector< double > data(N * 2u, 1.0);
        epimodels::ModelSIRLogit<> model(
            "v", N, data.data(), 2u,
            {2.0, 0.0, 0.0},  // infection: exposure coefficient, then features
            {20.0, 20.0},     // recovery: plogis(40), about 1
            {0u, 1u}, {0u, 1u},
            0.5, 0.1, 0.01
        );
        add_net(model, net);
        model.run(20, 8);

        int n = 0;
        for (size_t i = 0u; i < N; ++i)
        {
            const int in = model.get_stage_entered(i, 1);
            const int out = model.get_stage_entered(i, 2);
            if ((in >= 0) && (out >= 0))
            {
                REQUIRE(out - in == 1);
                ++n;
            }
        }
        REQUIRE(n > 30);
    }

}
