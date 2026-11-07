#include "tests.hpp"

using namespace epiworld;

// Pushing and pulling must draw the same outcome for a susceptible agent.
//
// A susceptible agent at the centre of a star, surrounded by agents that carry
// a virus, is exposed for one day, many times over. Pulling (roulette) and
// pushing (odds + reservoir) should both give
//
//     P(no infection)   = 1 / (1 + R),   P(infected by j) = r_j / (1 + R),
//
// with r_j = p_j / (1 - p_j), R = sum of r_j, and p_j the per-contact
// probability after the tools of both agents. Carriers in a stage that is not
// infectious never transmit, and contacts with p = 1 make infection certain, the infector
// drawn uniformly among them.
//
// Why a star: the formula above is about one susceptible agent exposed to
// several infectious contacts at once -- the case where push and pull differ
// in how they get there (one draw over all contacts vs. odds added one contact
// at a time plus a reservoir sample). The star isolates exactly that: its
// centre is the only susceptible, and every leaf is a source with a known
// probability, so each run is one draw with a closed-form answer. In a ring
// every susceptible has at most two contacts, which barely exercises the
// reservoir, and outcomes spread along the ring, so there is no closed form to
// check against. Whole networks (small-world rings with rewiring) are covered
// in 34b, against each other rather than a formula.

namespace {

constexpr int E_ = 1, I_ = 2; // stage 0 is Susceptible

// Star: agent 0 at the centre, agents 1..nleaves around it (the model has
// nleaves + 1 agents).
void make_star(Model & model, int nleaves)
{
    std::vector< int > source, target;
    for (int k = 1; k <= nleaves; ++k)
    {
        source.push_back(0);
        target.push_back(k);
    }
    model.add_layer(layer::network(
        "network", AdjList(source, target, nleaves + 1, false)
    ));
}

// The viruses: only I transmits (E carries the virus but does not); E and I
// never change. Each virus has its own probability of transmission.
Virus make_virus(std::string name, epiworld_double p)
{
    Virus v(name);
    v.add_stage("Susceptible");
    v.add_stage("Exposed", stage::carrier);
    v.add_stage("Infected", stage::infectious);
    v.infection(0, E_, p);
    return v;
}

// The agents `ids` (of `n`) start in `stage`
Assign at(std::vector< size_t > ids, int stage, size_t n)
{
    std::vector< int > labels(n, -1);
    for (auto i : ids)
        labels[i] = stage;
    return assign::labels(labels);
}

// Runs one-day replicates and counts who infected the centre (-1 = nobody).
std::map< int, int > count_infectors(Model & model, int nsims, int seed)
{
    std::map< int, int > counts;
    auto saver = [&counts](size_t, Model * m) -> void {
        std::vector< int > date, source, target, virus, sexp, sexp_layer;
        m->get_db().get_transmissions(date, source, target, virus, sexp, sexp_layer);
        int who = -1;
        for (size_t k = 0u; k < date.size(); ++k)
            if ((date[k] == 1) && (target[k] == 0))
                who = source[k];
        counts[who]++;
    };
    model.run_multiple(1, nsims, seed, saver, true, false, 1);
    return counts;
}

void check_probs(
    const std::map< int, int > & counts,
    const std::map< int, double > & expected,
    int nsims,
    const std::string & label
)
{
    for (const auto & kv : expected)
    {
        int observed = counts.count(kv.first) ? counts.at(kv.first) : 0;
        double mu = nsims * kv.second;
        double sd = std::sqrt(nsims * kv.second * (1.0 - kv.second));
        INFO(label << ": infector " << kv.first << " observed " << observed <<
            ", expected " << mu);
        CHECK(std::abs(observed - mu) <= 5.0 * sd + 1.0);
    }

    // Nobody outside `expected` may ever infect the centre
    for (const auto & kv : counts)
    {
        INFO(label << ": unexpected infector " << kv.first);
        CHECK(expected.count(kv.first) == 1u);
    }
}

} // namespace

EPIWORLD_TEST_CASE("Transmission - push and pull draw the same infector", "[transmission]") {

    const int nsims = 40000;

    // Two viruses, tools on both ends, and an excluded carrier -------------
    //  leaf 1, 2: virus A (p = 0.2)
    //  leaf 3:    virus B (p = 0.6)
    //  leaf 4:    virus B, wearing a mask that halves transmission
    //  leaf 5:    virus C (p = 0.9) but still Exposed: never infects
    //  leaf 6:    susceptible, not a source
    //  centre:    vaccinated, susceptibility reduced by a quarter
    const double vax = 0.25, mask = 0.5;
    std::map< int, double > p_contact = {
        {1, (1 - vax) * 0.2},
        {2, (1 - vax) * 0.2},
        {3, (1 - vax) * 0.6},
        {4, (1 - vax) * 0.6 * (1 - mask)}
    };

    double R = 0.0;
    for (auto & kv : p_contact)
        R += kv.second / (1.0 - kv.second);

    std::map< int, double > expected = {{-1, 1.0 / (1.0 + R)}};
    for (auto & kv : p_contact)
        expected[kv.first] = kv.second / (1.0 - kv.second) / (1.0 + R);

    for (auto mode : {"pull", "push"})
    {

        Model model(7);
        make_star(model, 6);
        model.add_virus(make_virus("A", 0.2), at({1u, 2u}, I_, 7u));
        model.add_virus(make_virus("B", 0.6), at({3u, 4u}, I_, 7u));
        model.add_virus(make_virus("C", 0.9), at({5u}, E_, 7u));

        // When pushing, the vaccine reads a parameter and the mask is a
        // function: same reductions, so the same probabilities
        const bool push = std::string(mode) == "push";
        model.add_param(vax, "Vaccine efficacy");

        Tool t_vax("vaccine");
        if (push)
            t_vax.set_susceptibility_reduction("Vaccine efficacy");
        else
            t_vax.set_susceptibility_reduction(vax);
        model.add_tool(t_vax, assign::agents({0u}));

        Tool t_mask("mask");
        if (push)
            t_mask.set_transmission_reduction_fun(
                [mask](Tool &, Agent *, VirusPtr &, Model *) {
                    return mask;
                }
            );
        else
            t_mask.set_transmission_reduction(mask);
        model.add_tool(t_mask, assign::agents({4u}));

        model.set_transmission_mode(mode);
        auto counts = count_infectors(model, nsims, 1231);

        REQUIRE(model.get_last_transmission_mode() ==
            (std::string(mode) == "push" ?
                TransmissionMode::push : TransmissionMode::pull));

        check_probs(counts, expected, nsims, std::string("tools/") + mode);

    }

    // Certain transmission ----------------------------------------------------
    // Leaves 1 and 3 transmit with p = 1 and leaf 2 with p = 0.3: the centre is
    // always infected, by leaf 1 or 3 with equal chance, never by leaf 2. With
    // pulling this relies on roulette() treating p = 1 as certain.
    std::map< int, double > expected_certain = {{1, 0.5}, {3, 0.5}};

    for (auto mode : {"pull", "push"})
    {

        Model model(4);
        make_star(model, 3);
        model.add_virus(make_virus("sure", 1.0), at({1u, 3u}, I_, 4u));
        model.add_virus(make_virus("maybe", 0.3), at({2u}, I_, 4u));

        model.set_transmission_mode(mode);
        auto counts = count_infectors(model, nsims / 4, 77);

        check_probs(counts, expected_certain, nsims / 4, std::string("certain/") + mode);

    }

}
