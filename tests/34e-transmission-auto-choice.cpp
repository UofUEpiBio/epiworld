#include "tests.hpp"

using namespace epiworld;

// The automatic choice between pushing and pulling, on a large outbreak in a
// heterogeneous network (household cliques plus a heavy-tailed layer), the
// kind of network where a pull costs more than it looks: at low degrees the
// fixed cost of visiting each susceptible agent is comparable to scanning its
// ties, and a push visits far fewer agents.
//
// The choice is a function of the model's state only, so the assertions are on
// the modes used, never on timings:
//
// 1. The first steps push (few agents carry a virus).
// 2. Around the peak, when a third of the population carries the virus, the
//    rule still pushes: pulling every susceptible is dearer there.
// 3. Since it only ever pushes, the run is exactly the run of "push".
// 4. A tighter threshold (`kappa`) makes the rule pull sooner, and a looser one
//    later, so the constant is doing what its documentation says.

namespace {

struct Modes {
    std::string trace;                       // 'P' or 'L' for each day
    std::vector< int > counts, date, target; // the run itself
    int n_pull() const { return static_cast< int >(std::count(trace.begin(), trace.end(), 'L')); }
    int n_push() const { return static_cast< int >(std::count(trace.begin(), trace.end(), 'P')); }
};

Modes simulate(const std::string & mode, double kappa = -1.0)
{

    const size_t n = 20000u;

    epimodels::ModelSEIR<> model("virus", 100.0 / static_cast< double >(n), 0.1, 3.0, 1.0 / 7.0);
    model.set_state_function(0u, sampler::make_update_susceptible<>({1u}));
    model.seed(8);
    tests_heterogeneous_network(model, n);

    if (kappa >= 0.0)
        model.set_transmission_mode(mode, kappa);
    else
        model.set_transmission_mode(mode);

    model.verbose_off();

    Modes out;
    std::string * trace = &out.trace;
    model.add_globalevent(
        [trace](Model<> * m) -> void {
            trace->push_back(
                m->get_last_transmission_mode() == TransmissionMode::push ? 'P' : 'L'
            );
        },
        "trace modes"
    );

    model.run(100, 2026);

    std::vector< int > source, virus, sexp;
    model.get_db().get_hist_total(nullptr, nullptr, &out.counts);
    model.get_db().get_transmissions(out.date, source, out.target, virus, sexp);
    return out;

}

} // namespace

EPIWORLD_TEST_CASE("Transmission - automatic choice", "[transmission]") {

    Modes autom = simulate("auto");
    Modes push = simulate("push");

    // A large outbreak
    REQUIRE(autom.date.size() > 8000u);

    // 1. Push while few carry the virus
    REQUIRE(autom.trace.substr(0u, 10u) == std::string(10u, 'P'));

    // 2. And through the peak
    INFO("modes by day: " << autom.trace);
    REQUIRE(autom.n_pull() == 0);

    // 3. So it is the push run
    REQUIRE(autom.counts == push.counts);
    REQUIRE(autom.date == push.date);
    REQUIRE(autom.target == push.target);

    // 4. The threshold moves the switch
    Modes tight = simulate("auto", 0.05);
    REQUIRE(tight.n_pull() > 0);
    REQUIRE(tight.n_pull() > autom.n_pull());

    Modes loose = simulate("auto", 100.0);
    REQUIRE(loose.n_pull() == 0);

}
