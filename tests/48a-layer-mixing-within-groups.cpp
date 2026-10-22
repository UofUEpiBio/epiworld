#include "tests.hpp"

using namespace epiworld;

// A mixing layer with a rate per group mixes within groups only: here 60
// households of 10 agents added to an SIR model. The model's queue sees the
// layer, so a run with the queue is the same run, draw for draw, as one
// without it -- also when agents change households during the run. The layer
// gives the same epidemic, in distribution, as one built with the equivalent
// diagonal contact matrix. Rates set between runs are kept; set during a run,
// they last for that run.
namespace {

const size_t n = 600u;
const size_t n_groups = 60u;

std::vector< size_t > households()
{
    std::vector< size_t > g(n);
    for (size_t i = 0u; i < n; ++i)
        g[i] = i / 10u;
    return g;
}

// SIR model; S = 0, I = 1, R = 2
std::unique_ptr< epimodels::ModelSIR<> > make(
    std::function< void(layer::Mixing<> &) > contacts
)
{

    auto model = std::make_unique< epimodels::ModelSIR<> >(
        "flu", n, 0.02, 0.2, 0.1
    );
    model->verbose_off();

    auto hh = layer::mixing<>("households", {1}, {0, 1, 2});
    hh->set_groups(households(), n_groups);
    contacts(*hh);
    model->add_layer(hh);

    return model;

}

void rates(layer::Mixing<> & l) { l.set_rates(3.0); }

void matrix(layer::Mixing<> & l)
{
    std::vector< double > cm(n_groups * n_groups, 0.0);
    for (size_t g = 0u; g < n_groups; ++g)
        cm[g * n_groups + g] = 3.0;
    l.set_contact_matrix(cm);
}

}

EPIWORLD_TEST_CASE("Mixing layer - within groups", "[layer-mixing-within]") {

    // -- the queue sees the groups -----------------------------------------
    // Agents move to the next household on day 5 (an edit during the run),
    // so the queue must also follow the moves.
    auto move = [](Model<> * m) -> void {
        auto & l = m->get_layer_as< layer::Mixing<> >("households");
        for (size_t i = 0u; i < n; i += 7u)
            l.set_group(i, (l.get_group(i) + 1u) % n_groups);
    };

    std::vector< int > hist[2], src[2];
    for (bool queue : {true, false})
    {

        auto model = make(rates);
        if (!queue)
            model->queuing_off();

        model->add_globalevent(move, "move", 5);
        model->run(60, 46);

        model->get_db().get_hist_total(nullptr, nullptr, &hist[queue]);
        std::vector< int > d, t, v, se, lay;
        model->get_db().get_transmissions(d, src[queue], t, v, se, lay);

        // Every transmission came through the layer, between housemates of
        // the day (the move makes some cross the original households)
        REQUIRE(src[queue].size() > 50u);
        for (size_t k = 0u; k < lay.size(); ++k)
            REQUIRE(lay[k] == (src[queue][k] < 0 ? -1 : 0));

    }
    REQUIRE(hist[0] == hist[1]);
    REQUIRE(src[0] == src[1]);

    // The edit lasted for that run only
    auto check = make(rates);
    check->add_globalevent(move, "move", 5);
    check->run(10, 1);
    auto & hh = check->get_layer_as< layer::Mixing<> >("households");
    REQUIRE(hh.get_group(7u) == 1u);
    check->rm_globalevent("move");
    check->run(1, 1);
    REQUIRE(hh.get_group(7u) == 0u);
    REQUIRE(hh.members_of(0u).size() == 10u);

    // -- rates and the equivalent diagonal matrix agree in distribution -----
    auto attack = [](epimodels::ModelSIR<> & m) {
        std::vector< double > out;
        m.run_multiple(
            60, 200, 2026,
            [&out](size_t, Model<> * mm) -> void {
                std::vector< int > counts;
                mm->get_db().get_today_total(nullptr, &counts);
                out.push_back(static_cast< double >(counts[1] + counts[2]));
            },
            true, false, 1
        );
        double mean = 0.0, var = 0.0;
        for (double x : out) mean += x;
        mean /= out.size();
        for (double x : out) var += (x - mean) * (x - mean);
        var /= (out.size() - 1u);
        return std::make_pair(mean, var / out.size());
    };

    auto with_rates = make(rates);
    auto with_matrix = make(matrix);

    auto a = attack(*with_rates);
    auto b = attack(*with_matrix);
    REQUIRE(a.first > 50.0); // households do get infected
    REQUIRE(std::abs(a.first - b.first) < 4.0 * std::sqrt(a.second + b.second));

    // -- rates are edited like the contact matrix ---------------------------
    auto edit = make(rates);
    auto & layer = edit->get_layer_as< layer::Mixing<> >("households");

    // Between runs: the new baseline, one per group or one for all
    std::vector< double > r(n_groups, 1.0);
    r[0] = 9.0;
    layer.set_rates(r);
    edit->run(1, 1);
    REQUIRE(layer.get_rates() == r);

    // During a run: that run only
    edit->add_globalevent(
        [](Model<> * m) -> void {
            m->get_layer_as< layer::Mixing<> >("households").set_rates(0.0);
        },
        "lockdown"
    );
    edit->run(2, 1);
    REQUIRE(layer.get_rates() == std::vector< double >(n_groups, 0.0));
    edit->rm_globalevent("lockdown");
    edit->run(1, 1);
    REQUIRE(layer.get_rates() == r);

    // A matrix replaces the rates between runs, but not during one
    edit->add_globalevent(
        [](Model<> * m) -> void {
            m->get_layer_as< layer::Mixing<> >("households")
                .set_contact_matrix({1.0});
        },
        "switch"
    );
    REQUIRE_THROWS_AS(edit->run(1, 1), std::logic_error);

    // Rates need the grouping (the queue reads it before the first step)
    epimodels::ModelSIR<> bare("flu", n, 0.02, 0.2, 0.1);
    auto nogroups = layer::mixing<>("households", {1}, {0, 1, 2});
    nogroups->set_rates(3.0);
    bare.add_layer(nogroups);
    bare.verbose_off();
    REQUIRE_THROWS_AS(bare.run(1, 1), std::logic_error);

    // One rate per group, or one for all
    auto wrong = make(rates);
    wrong->get_layer_as< layer::Mixing<> >("households").set_rates({1.0, 2.0});
    REQUIRE_THROWS_AS(wrong->run(1, 1), std::length_error);

}
