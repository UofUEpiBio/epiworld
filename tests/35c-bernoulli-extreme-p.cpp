#include "tests.hpp"

using namespace epiworld;

// rgraph_bernoulli() at the ends of [0, 1]. With p = 0 it used to return an
// almost complete graph (log(1 - p) = 0 made the first geometric skip
// infinite); now it returns no ties, so nobody is infected beyond the initial
// cases. With p = 1 every pair is tied, and a p outside [0, 1] is refused.
EPIWORLD_TEST_CASE("Bernoulli graphs - p = 0 and p = 1", "[rgraph_bernoulli]") {

    const size_t n = 200u;

    for (bool directed : {false, true})
    {

        epimodels::ModelSIR<> model("a virus", n, 0.05, 1.0, 0.0);
        model.verbose_off();
        auto & net = model.add_layer(layer::network(
            "network", graph::bernoulli(0.0, directed)
        ));

        model.run(0, 123);
        const int susceptible0 = model.get_db().get_today_total("Susceptible");
        REQUIRE(susceptible0 < static_cast< int >(n));

        std::vector< int > source, target;
        net.get_edgelist(source, target);
        REQUIRE(source.empty());

        model.run(30, 123);
        REQUIRE(model.get_db().get_today_total("Susceptible") == susceptible0);

        AdjList full = rgraph_bernoulli(n, 1.0, directed, model);
        REQUIRE(full.ecount() == (directed ? n * (n - 1u) : n * (n - 1u) / 2u));

    }

    epimodels::ModelSIR<> model("a virus", n, 0.05, 1.0, 0.0);
    REQUIRE_THROWS_AS(rgraph_bernoulli(n, -0.1, false, model), std::range_error);
    REQUIRE_THROWS_AS(rgraph_bernoulli(n, 1.5, true, model), std::range_error);

}
