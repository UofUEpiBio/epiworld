#include "tests.hpp"

using namespace epiworld;

// A network layer's graph can come from vectors or from a file. Both must give
// the same ties, with neighbors in the same order, so the same seed gives the
// same epidemic -- history, transmissions, and ties. So must rebuilding from
// the layer's own edge list. Checked for an undirected and a directed network
// with repeated, reversed, and self-loop edges and a hub.

struct Run {
    std::vector< int > hist, tdate, tsource, ttarget, s, t;
};

static Run run_model(Model<> & model)
{
    model.run(50, 1231);

    Run r;
    std::vector< int > date, virus, sexp;
    model.get_db().get_hist_total(&date, nullptr, &r.hist);
    model.get_db().get_transmissions(
        r.tdate, r.tsource, r.ttarget, virus, sexp
    );
    tests_net(model).get_edgelist(r.s, r.t);

    return r;
}

static epimodels::ModelSIR<> make_model(int n)
{
    epimodels::ModelSIR<> model("a virus", n, 0.02, 0.4, 0.2);
    model.verbose_off();
    return model;
}

EPIWORLD_TEST_CASE("Edge list - all build paths agree", "[edgelist]") {

    std::mt19937 gen(36u);
    const int n = 1500;
    std::vector< int > source, target;
    for (int m = 0; m < 6000; ++m)
    {
        int a = static_cast< int >(gen() % n);
        int b = static_cast< int >(gen() % n);
        source.push_back(a);
        target.push_back(b);
        if (m % 11 == 0) { source.push_back(b); target.push_back(a); }
        if (m % 97 == 0) { source.push_back(a); target.push_back(a); }
    }
    for (int k = 0; k < 100; ++k)
    {
        source.push_back(3);
        target.push_back(static_cast< int >(gen() % n));
    }

    std::string fn = "36b-edgelist.txt";
    {
        std::ofstream f(fn);
        for (size_t m = 0u; m < source.size(); ++m)
            f << source[m] << " " << target[m] << "\n";
    }

    for (bool directed : {false, true})
    {

        auto m_el = make_model(n);
        m_el.add_layer(std::make_unique< layer::Network<> >(
            "network", AdjList(source, target, n, directed)
        ));

        auto m_fn = make_model(n);
        AdjList from_file;
        from_file.read_edgelist(fn, n, 0, directed);
        m_fn.add_layer(std::make_unique< layer::Network<> >("network", from_file));

        REQUIRE(tests_net(m_el).is_directed() == directed);
        REQUIRE(tests_net(m_fn).is_directed() == directed);

        Run r_el = run_model(m_el);
        Run r_fn = run_model(m_fn);

        // The epidemic did spread, so the comparison means something.
        REQUIRE(r_el.tdate.size() > 50u);

        REQUIRE(r_el.hist == r_fn.hist);
        REQUIRE(r_el.tsource == r_fn.tsource);
        REQUIRE(r_el.ttarget == r_fn.ttarget);
        REQUIRE(r_el.s == r_fn.s);
        REQUIRE(r_el.t == r_fn.t);

        // Rebuilding from the model's own edge list reproduces the run.
        auto m_rt = make_model(n);
        m_rt.add_layer(std::make_unique< layer::Network<> >(
            "network", AdjList(r_el.s, r_el.t, n, directed)
        ));
        Run r_rt = run_model(m_rt);
        REQUIRE(r_el.hist == r_rt.hist);
        REQUIRE(r_el.tsource == r_rt.tsource);
        REQUIRE(r_el.ttarget == r_rt.ttarget);

    }

    std::remove(fn.c_str());

}
