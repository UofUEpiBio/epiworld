#include "tests.hpp"
#include <set>
#include <map>
#include <unordered_map>
#include <algorithm>

using namespace epiworld;

// Builds a household id vector: agents 0..n-1 grouped in households of `hh_size`.
static std::vector<size_t> make_households(size_t n, size_t hh_size)
{
    std::vector<size_t> hh(n);
    for (size_t i = 0u; i < n; ++i)
        hh[i] = i / hh_size;
    return hh;
}

// Households making up each bubble.
static std::map<int, std::set<size_t>> households_per_bubble(
    const std::vector<size_t> & hh, const std::vector<int> & bid
)
{
    std::map<int, std::set<size_t>> res;
    for (size_t a = 0u; a < bid.size(); ++a)
        res[bid[a]].insert(hh[a]);
    return res;
}

// Transmissions split into within- and out-of-bubble.
struct TransmissionCounts {
    int within = 0;
    int outside = 0;
};

// Adding the intervention to the model is the whole installation: it sets
// itself up on the first day of *every* run, so the partition is drawn from
// that run's seed and the tool is back on the agents after the reset that
// stripped it -- no stale partition carried over, and no setup step for the
// user to remember.

// Two households of three, joined by the single tie 2--3:
//
//   household 0: 0 -- 1 -- 2            household 1: 3 -- 4 -- 5
//                          \___________________/
//
// Under BubbleTies::Complete the bubble is a group of the household layer
// (within-group mixing, q = 1): outside the policy window each household meets
// in full, and while it is in force the whole bubble does.
static void two_households(Model & model)
{
    model.add_layer(layer::network(
        "network", AdjList({0, 1, 3, 4, 2}, {1, 2, 4, 5, 3}, 6, false)
    ));
}

static std::vector< size_t > households(size_t n, size_t size)
{
    std::vector< size_t > hh(n);
    for (size_t i = 0u; i < n; ++i)
        hh[i] = i / size;
    return hh;
}

// What a Complete bubble leaves alone (the grouping), and what it refuses:
// a missing household layer, and two policies moving agents in one layer.

// Bubbles
//
// One test case; each former test file is a block below, with the name it had.
EPIWORLD_TEST_CASE("Bubbles", "[bubbles]") {

    // ---- 32a-bubbles.cpp: Bubbles - household partition structure
    {

    size_t n = 300u, hh_size = 3u, group = 2u;
    auto hh = make_households(n, hh_size);

    epimodels::ModelSEIR model("flu", n, 0.05, 0.1, 4.5, 1.0/8.0);
    model.seed(11);
    model.add_layer(layer::network(
        "network", rgraph_smallworld(n, 6, 0.05, false, model)
    ));

    // Adding the intervention is all there is to it: it sets itself up (the
    // partition included) on the first day of the run.
    Bubbles bubbles(hh, BubbleFlavor::Household, group, 0.0, 0, -1, 0);
    model.add_globalevent(bubbles);
    model.verbose_off();
    model.run(5);

    const auto & bid = Bubbles::get_from(model)->get_bubble_id();
    REQUIRE(bid.size() == n);

    // Agents in the same household share a bubble.
    for (size_t a = 0u; a < n; ++a)
        REQUIRE(bid[a] == bid[(a / hh_size) * hh_size]);

    // Each bubble contains at most `group` households (and the partition is a
    // function of households, so households never split across bubbles).
    std::unordered_map<int, std::set<size_t>> households_in_bubble;
    for (size_t a = 0u; a < n; ++a)
        households_in_bubble[bid[a]].insert(hh[a]);

    size_t max_hh = 0u;
    for (auto & kv : households_in_bubble)
        max_hh = std::max(max_hh, kv.second.size());

    REQUIRE(max_hh <= group);

    // Bubbles are grown along existing ties, so a household may end up in a
    // smaller bubble (or alone) when it has no unassigned connected partner.
    // Hence there are at least ceil(n_households / group) bubbles.
    size_t n_households = n / hh_size;
    size_t min_bubbles = (n_households + group - 1u) / group;
    REQUIRE(households_in_bubble.size() >= min_bubbles);

    // Household contact graph: h1 ~ h2 iff some member of h1 is connected to a
    // member of h2.
    std::map<size_t, std::set<size_t>> hh_adj;
    for (auto & a : model.get_agents())
    {
        size_t ia = static_cast<size_t>(a.get_id());
        for (auto * nb : tests_net(model).neighbors_view(ia))
        {
            size_t ib = static_cast<size_t>(nb->get_id());
            if (hh[ia] != hh[ib])
                hh_adj[hh[ia]].insert(hh[ib]);
        }
    }

    // Every bubble must be CONNECTED in the household graph. Grouping
    // households that share no tie would be a no-op, since the intervention can
    // only suppress transmission along existing edges (never create them).
    for (auto & kv : households_in_bubble)
    {
        const std::set<size_t> & members = kv.second;
        if (members.size() < 2u)
            continue;

        // BFS from an arbitrary member, restricted to the bubble.
        std::set<size_t> seen;
        std::vector<size_t> stack{*members.begin()};
        seen.insert(*members.begin());
        while (!stack.empty())
        {
            size_t cur = stack.back();
            stack.pop_back();
            for (size_t nb : hh_adj[cur])
                if (members.count(nb) && !seen.count(nb))
                {
                    seen.insert(nb);
                    stack.push_back(nb);
                }
        }

        REQUIRE(seen.size() == members.size());
    }
    }

    // ---- 32b-bubbles-peer.cpp: Bubbles - peer partition respects households
    {

    size_t n = 300u, hh_size = 3u;
    auto hh = make_households(n, hh_size);
    size_t n_households = n / hh_size;

    // Household contact graph: h1 ~ h2 iff some member of h1 is connected to a
    // member of h2. Filled from the (deterministic) network below.
    std::map<size_t, std::set<size_t>> hh_adj;

    // Partition produced by the peer rule under a given cap on the number of
    // households a bubble may hold.
    auto partition_with_cap = [&](size_t cap) -> std::vector<int> {
        epimodels::ModelSEIR model("flu", n, 0.05, 0.1, 4.5, 1.0/8.0);
        model.seed(7);
        model.add_layer(layer::network(
            "network", rgraph_smallworld(n, 8, 0.15, false, model)
        ));

        Bubbles bubbles(
            hh, BubbleFlavor::Peer, 1u, 0.0, 0, -1, 0, "Social bubble", cap
        );
        model.add_globalevent(bubbles);
        model.verbose_off();
        model.run(5);

        for (auto & a : model.get_agents())
        {
            size_t ia = static_cast<size_t>(a.get_id());
            for (auto * nb : tests_net(model).neighbors_view(ia))
            {
                size_t ib = static_cast<size_t>(nb->get_id());
                if (hh[ia] != hh[ib])
                    hh_adj[hh[ia]].insert(hh[ib]);
            }
        }

        return Bubbles::get_from(model)->get_bubble_id();
    };

    std::vector<int> bid = partition_with_cap(2u);

    // Households never split across bubbles.
    for (size_t a = 0u; a < n; ++a)
        REQUIRE(bid[a] == bid[(a / hh_size) * hh_size]);

    auto bubbles_2 = households_per_bubble(hh, bid);

    // Peer nominations merge households, so some households share a bubble.
    REQUIRE(bubbles_2.size() < n_households);

    // ...but the merges must NOT percolate. With the default cap of two
    // households per bubble, no bubble may exceed it and there must be at least
    // n_households / 2 bubbles. (Uncapped union-find collapsed the whole
    // population into a single bubble, imposing no restriction at all.)
    for (auto & kv : bubbles_2)
        REQUIRE(kv.second.size() <= 2u);

    REQUIRE(bubbles_2.size() >= (n_households + 1u) / 2u);

    // Every household sharing a bubble must be genuinely connected: with a cap
    // of two, the pair is joined by at least one member-to-member contact.
    // Bubbling households that share no contact would be a no-op, since the
    // intervention can only suppress transmission along existing edges.
    for (auto & kv : bubbles_2)
    {
        if (kv.second.size() < 2u)
            continue;
        auto it = kv.second.begin();
        size_t h1 = *it++, h2 = *it;
        REQUIRE(hh_adj[h1].count(h2) == 1u);
    }

    // `max_households` is the dial on bubble size: raising it lets the
    // nominations chain further, but never beyond the cap.
    auto bubbles_4 = households_per_bubble(hh, partition_with_cap(4u));

    size_t largest = 0u;
    for (auto & kv : bubbles_4)
        largest = std::max(largest, kv.second.size());

    REQUIRE(largest <= 4u);
    REQUIRE(largest > 2u);              // the cap is a dial, not a constant
    REQUIRE(bubbles_4.size() < bubbles_2.size()); // ...and fewer, larger bubbles
    }

    // ---- 32c-bubbles-transmission-factor.cpp: Bubbles - transmission factor dampens out-of-bubble contact
    {

    size_t n = 400u, hh_size = 4u, group = 2u;
    auto hh = make_households(n, hh_size);

    // Runs the model with the factor given at construction; `override_factor`
    // (when non-negative) is instead written to the model parameter before the
    // run, so the tool must pick it up from the model.
    auto run_with = [&](
        epiworld_double factor, epiworld_double override_factor
    ) -> TransmissionCounts {

        epimodels::ModelSEIR model("flu", n, 0.1, 0.2, 4.5, 1.0/8.0);
        model.seed(2024);
        model.add_layer(layer::network(
            "network", rgraph_smallworld(n, 8, 0.10, false, model)
        ));

        Bubbles bubbles(hh, BubbleFlavor::Household, group, factor, 0, -1, 0);
        model.add_globalevent(bubbles);

        if (override_factor >= 0.0)
            model.add_param(override_factor, bubbles.get_param_name(), true);

        model.verbose_off();
        model.run(80);

        // The intervention registers the factor as a model parameter when it
        // sets itself up, but a value already in the model is left alone.
        REQUIRE(
            model.get_param(bubbles.get_param_name()) ==
            (override_factor >= 0.0 ? override_factor : factor)
        );

        const auto & bid = Bubbles::get_from(model)->get_bubble_id();

        std::vector<int> date, source, target, virus, sexp, sexp_layer;
        model.get_db().get_transmissions(date, source, target, virus, sexp, sexp_layer);

        TransmissionCounts counts;
        for (size_t i = 0u; i < source.size(); ++i)
        {
            if (source[i] < 0) // seed case (external)
                continue;

            if (bid[static_cast<size_t>(source[i])] ==
                bid[static_cast<size_t>(target[i])])
                ++counts.within;
            else
                ++counts.outside;
        }

        return counts;

    };

    // factor = 0: a perfectly observed bubble. Contact outside it is cut, while
    // contact *within* the bubble is untouched -- the outbreak still spreads
    // inside bubbles.
    TransmissionCounts strict = run_with(0.0, -1.0);
    REQUIRE(strict.outside == 0);
    REQUIRE(strict.within > 0);

    // factor = 1: the bubble imposes nothing, so contacts across bubbles
    // transmit as they would without the intervention.
    TransmissionCounts off = run_with(1.0, -1.0);
    REQUIRE(off.outside > 0);

    // A soft contact reduction sits in between: some out-of-bubble
    // transmission, but less than with no policy at all.
    TransmissionCounts soft = run_with(0.25, -1.0);
    REQUIRE(soft.outside > 0);
    REQUIRE(soft.outside < off.outside);

    // The factor is read from the model on every exposure, so the model's
    // parameter -- not the value the object was built with -- governs the run.
    TransmissionCounts from_param = run_with(0.0, 1.0);
    REQUIRE(from_param.outside == off.outside);
    REQUIRE(from_param.within == off.within);
    }

    // ---- 32d-bubbles-rewiring.cpp: Bubbles - rewiring advances epochs
    {

    size_t n = 300u, hh_size = 3u;
    auto hh = make_households(n, hh_size);

    epimodels::ModelSEIR model("flu", n, 0.05, 0.1, 4.5, 1.0/8.0);
    model.seed(5);
    model.add_layer(layer::network(
        "network", rgraph_smallworld(n, 6, 0.05, false, model)
    ));

    int start = 10, rewire = 7, ndays = 60;
    Bubbles bubbles(hh, BubbleFlavor::Household, 2u, 0.0, start, -1, rewire);
    model.add_globalevent(bubbles);
    model.verbose_off();
    model.run(ndays);

    // The intervention recomputes the partition at each rewiring epoch. The
    // model owns the intervention, so the epoch is read off the model.
    int expected_epoch = (ndays - start) / rewire;
    REQUIRE(Bubbles::get_from(model)->get_last_epoch() == expected_epoch);
    }

    // ---- 32e-bubbles-setup-per-run.cpp: Bubbles - set up on every run
    {

    size_t n = 300u, hh_size = 3u;
    auto hh = make_households(n, hh_size);

    epimodels::ModelSEIR model("flu", n, 0.05, 0.1, 4.5, 1.0/8.0);
    model.add_layer(layer::network(
        "network", rgraph_smallworld(n, 6, 0.05, false, model)
    ));

    Bubbles bubbles(hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0);
    model.add_globalevent(bubbles);
    model.verbose_off();

    // Nothing is installed before the run: the model's own copy of the
    // intervention has no partition yet.
    REQUIRE(Bubbles::get_from(model)->get_bubble_id().empty());

    model.run(5, 111);
    std::vector<int> p1 = Bubbles::get_from(model)->get_bubble_id();

    model.run(5, 222);
    std::vector<int> p2 = Bubbles::get_from(model)->get_bubble_id();

    REQUIRE(p1.size() == n);
    REQUIRE(p2.size() == n);

    // Both are valid partitions (households never split across bubbles).
    for (size_t a = 0u; a < n; ++a)
    {
        REQUIRE(p1[a] == p1[(a / hh_size) * hh_size]);
        REQUIRE(p2[a] == p2[(a / hh_size) * hh_size]);
    }

    // Different seeds shuffle households differently -> the partitions differ.
    // (A partition computed once and kept would make both runs identical.)
    bool differ = false;
    for (size_t a = 0u; a < n; ++a)
        if (p1[a] != p2[a]) { differ = true; break; }
    REQUIRE(differ);

    // Every agent carries the bubble tool at the end of the second run: reset()
    // takes it away, so this only holds if the intervention hands it out on
    // each run rather than once.
    for (auto & a : model.get_agents())
        REQUIRE(a.has_tool("Social bubble"));

    // The object the user built stays a template: the model works off its own
    // copy, which is where the partition lives.
    REQUIRE(bubbles.get_bubble_id().empty());
    }

    // ---- 32f-bubbles-parallel.cpp: Bubbles - replicates are independent
    {

    size_t n = 400u, hh_size = 4u, group = 2u, nsims = 8u;
    auto hh = make_households(n, hh_size);

    // Per-replicate outbreak size, plus a count of transmissions that crossed a
    // bubble *of the model that produced them*. Both are collected from the
    // run_multiple callback, which epiworld serializes.
    struct Results {
        std::vector<int> outbreak;
        int cross_bubble = 0;
    };

    auto run_with = [&](int nthreads) -> Results {

        epimodels::ModelSEIR model("flu", n, 0.1, 0.2, 4.5, 1.0/8.0);
        model.seed(2024);
        model.add_layer(layer::network(
            "network", rgraph_smallworld(n, 8, 0.10, false, model)
        ));

        Bubbles bubbles(hh, BubbleFlavor::Household, group, 0.0, 0, -1, 0);
        model.add_globalevent(bubbles);
        model.verbose_off();

        Results res;
        res.outbreak.assign(nsims, -1);

        model.run_multiple(
            60, nsims, 1231,
            [&res](size_t run_id, Model * m) -> void {

                // Outbreak size: everyone who left the susceptible state.
                int infected = 0;
                for (auto & a : m->get_agents())
                    if (m->get_stage(a) != 0)
                        ++infected;

                res.outbreak[run_id] = infected;

                // Every transmission must be within a bubble of *this* model's
                // partition. Reading the partition off the model is the point:
                // each copy owns its own.
                const auto & bid =
                    Bubbles::get_from(*m)->get_bubble_id();

                std::vector<int> date, source, target, virus, sexp, sexp_layer;
                m->get_db().get_transmissions(
                    date, source, target, virus, sexp, sexp_layer
                );

                for (size_t i = 0u; i < source.size(); ++i)
                {
                    if (source[i] < 0) // seed case (external)
                        continue;

                    if (bid[static_cast<size_t>(source[i])] !=
                        bid[static_cast<size_t>(target[i])])
                        ++res.cross_bubble;
                }

            },
            true, false, nthreads
        );

        return res;

    };

    Results serial   = run_with(1);
    Results parallel = run_with(2);

    // The outbreak happened at all.
    for (size_t i = 0u; i < nsims; ++i)
        REQUIRE(serial.outbreak[i] > 0);

    // Replicates are reproducible from their seeds no matter how many threads
    // ran them: nothing is shared between the per-thread copies of the model.
    REQUIRE(parallel.outbreak == serial.outbreak);

    // With transmission_factor = 0, no transmission may cross a bubble. A
    // partition shared between threads would show up here as cross-bubble
    // transmissions, since each copy would be reading a partition that another
    // copy overwrote.
    REQUIRE(serial.cross_bubble == 0);
    REQUIRE(parallel.cross_bubble == 0);
    }

    // ---- 32g-bubbles-household-layer.cpp: Bubbles - complete bubbles in the household layer
    {

    const std::vector< size_t > hh = households(6u, 3u);

    // -- 1. The layer: one group per household, everyone meets ------------
    Bubbles policy(hh, BubbleFlavor::Household, 2u, 0.0, 2, 5, 0);
    REQUIRE_THROWS_WITH(policy.make_layer(), Catch::Matchers::Contains("name"));
    policy.set_ties(BubbleTies::Complete, "household");

    auto layer = policy.make_layer();
    REQUIRE(layer->get_name() == "household");
    REQUIRE(layer->get_n_groups() == 2u);
    REQUIRE(layer->within_groups());

    // -- 2. Agents move into the bubble while the policy is in force -------
    // Window [2, 5): the moves made by the daily event take effect at the end
    // of the step, so the groups seen on day d are the ones of day d's draws.
    epimodels::ModelSIR model("flu", 6, 0.0, 0.0, 0.0);
    model.verbose_off();
    two_households(model);
    model.add_layer(layer);
    model.add_globalevent(policy);

    std::vector< std::vector< size_t > > seen;
    model.add_globalevent(
        [&seen](Model * m) -> void {
            auto & l = m->get_layer_as< layer::Mixing >("household");
            std::vector< size_t > g(6u);
            for (size_t i = 0u; i < 6u; ++i)
                g[i] = l.get_group(i);
            seen.push_back(g);
        },
        "record the groups"
    );

    for (int rep = 0; rep < 2; ++rep)
    {
        // The second run starts from the households again (the reset
        // restores the layer)
        seen.clear();
        model.run(6, 20);

        // The two households are connected, so they bubble together
        const auto & bid = Bubbles::get_from(model)->get_bubble_id();
        REQUIRE(bid[0] == bid[5]);
        const size_t b = static_cast< size_t >(bid[0]);

        REQUIRE(seen.size() == 6u);
        for (size_t d = 1u; d <= 6u; ++d)
            for (size_t i = 0u; i < 6u; ++i)
                REQUIRE(seen[d - 1u][i] == (((d >= 2u) && (d < 5u)) ? b : hh[i]));
    }

    // -- 3. Who infects whom -----------------------------------------------
    // Agent 0 carries the virus, transmission is certain, nobody recovers,
    // and the network layer does not transmit (weight 0; the grouping still
    // reads it). Every transmission comes from the household layer. Returns
    // who got infected, with household and bubble contact probabilities
    // `h` and `b`, and `size` households per bubble.
    auto infected = [&](
        BubbleTies ties, double h, double b, size_t size, int ndays, int seed
    ) -> std::vector< bool > {

        epimodels::ModelSIR m("flu", 6, 0.0, 1.0, 0.0);
        m.verbose_off();
        m.assign_virus(0, assign::agents({0u}));
        m.add_param(0.0, "Network weight");
        two_households(m);
        m.get_layer("network").set_weight("Network weight");

        Bubbles p(hh, BubbleFlavor::Household, size, 0.0, 0, -1, 0);
        p.set_ties(BubbleTies::Complete, "household", b);
        m.add_layer(p.make_layer(h));
        p.set_ties(ties, "household", b);
        m.add_globalevent(p);

        m.run(static_cast< epiworld_fast_uint >(ndays), seed);

        std::vector< int > d, src, tgt, v, se, lay;
        m.get_db().get_transmissions(d, src, tgt, v, se, lay);
        const int hl = static_cast< int >(m.get_layer_id("household"));
        std::vector< bool > res(6u, false);
        for (size_t k = 0u; k < tgt.size(); ++k)
        {
            res[static_cast< size_t >(tgt[k])] = true;
            REQUIRE(lay[k] == ((src[k] >= 0) ? hl : -1));
        }

        return res;

    };

    auto count = [](const std::vector< bool > & x) {
        return static_cast< size_t >(std::count(x.begin(), x.end(), true));
    };

    // Within its household only, the virus reaches 3 agents; completed into
    // one bubble from day 1, all 6; a bubble contact probability of 0 keeps
    // everyone apart (housemates included: one probability per group).
    REQUIRE(count(infected(BubbleTies::Existing, 1.0, 1.0, 2u, 10, 3)) == 3u);
    REQUIRE(count(infected(BubbleTies::Complete, 1.0, 1.0, 2u, 10, 3)) == 6u);
    REQUIRE(count(infected(BubbleTies::Complete, 1.0, 0.0, 2u, 10, 3)) == 1u);

    // On day 1 only agent 0 transmits. In the merged bubble every pair meets
    // with the bubble probability (0.3); with bubbles of one household,
    // housemates keep the household probability (0.6).
    {
        const int nsims = 4000;
        double housemate = 0.0, other = 0.0, alone = 0.0;
        for (int sim = 0; sim < nsims; ++sim)
        {
            auto x = infected(BubbleTies::Complete, 0.6, 0.3, 2u, 1, sim);
            housemate += x[1];
            other     += x[3];
            alone     += infected(BubbleTies::Complete, 0.6, 0.3, 1u, 1, sim)[1];
        }

        REQUIRE_THAT(housemate / nsims, Catch::Matchers::WithinAbs(0.3, 0.03));
        REQUIRE_THAT(other / nsims, Catch::Matchers::WithinAbs(0.3, 0.03));
        REQUIRE_THAT(alone / nsims, Catch::Matchers::WithinAbs(0.6, 0.03));
    }

    // -- 4. Redrawn bubbles: the groups follow the partition ---------------
    // 300 agents in households of 3 on a small world, bubbles of 2
    // households redrawn every 4 days in [3, 20). Every day the layer's
    // groups are the previous day's partition (moves wait for the end of the
    // step) while the policy is in force, and the households otherwise.
    {
        const size_t n = 300u;
        const auto hh300 = households(n, 3u);

        epimodels::ModelSIR m("flu", n, 0.01, 0.2, 0.1);
        m.verbose_off();
        m.add_layer(layer::network("network", graph::smallworld(6, 0.1)));

        Bubbles p(hh300, BubbleFlavor::Household, 2u, 0.0, 3, 20, 4);
        p.set_ties(BubbleTies::Complete, "household");
        m.add_layer(p.make_layer());
        m.add_globalevent(p);

        std::vector< int > previous;
        size_t checked = 0u, merged = 0u;
        m.add_globalevent(
            [&](Model * mm) -> void {

                auto & l = mm->get_layer_as< layer::Mixing >("household");
                const auto * b = Bubbles::get_from(*mm);
                const int day = static_cast< int >(mm->today());

                const bool in_force = (day >= 3) && (day < 20);
                for (size_t i = 0u; i < n; ++i)
                    REQUIRE(l.get_group(i) == (
                        in_force ? static_cast< size_t >(previous[i]) : hh300[i]
                    ));

                // members_of() agrees with the groups, and bubbles hold
                // more than one household
                for (size_t g = 0u; g < l.get_n_groups(); ++g)
                {
                    for (size_t i : l.members_of(g))
                        REQUIRE(l.get_group(i) == g);

                    if (in_force && (l.members_of(g).size() > 3u))
                        ++merged;
                }

                previous = b->get_bubble_id();
                ++checked;

            },
            "check the groups"
        );

        m.run(25, 77);

        REQUIRE(checked == 25u);
        REQUIRE(merged > 0u);
    }
    }

    // ---- 32h-bubbles-household-layer-rules.cpp: Bubbles - household layer rules
    {

    const size_t n = 300u;
    const auto hh = households(n, 3u);

    // A household policy and a Peer one over another grouping (classrooms,
    // scattered across the network), each redrawn on its own schedule.
    std::vector< size_t > room(n);
    for (size_t i = 0u; i < n; ++i)
        room[i] = (i * 7u) % 50u;

    auto make = [&](BubbleTies ties) {

        auto m = std::make_unique< epimodels::ModelSIR >(
            "flu", n, 0.0, 0.2, 0.1
        );
        m->verbose_off();
        m->add_layer(layer::network("network", graph::smallworld(6, 0.1)));

        Bubbles home(hh, BubbleFlavor::Household, 3u, 0.0, 2, 30, 5,
            "Home bubble", 2u, "Home factor", ties, "household");
        Bubbles school(room, BubbleFlavor::Peer, 2u, 0.0, 4, -1, 3,
            "School bubble", 3u, "School factor", ties, "school");

        if (ties == BubbleTies::Complete)
        {
            m->add_layer(home.make_layer());
            m->add_layer(school.make_layer());
        }

        m->add_globalevent(home);
        m->add_globalevent(school);
        return m;

    };

    // -- 1. Grouping never sees the household layers -------------------------
    // With no prevalence nothing else differs, and a q = 1 layer draws no
    // random numbers: both policies must group exactly as they do when
    // neither moves anyone.
    auto partitions = [&](BubbleTies ties) {

        auto m = make(ties);
        std::vector< std::vector< int > > res;
        m->add_globalevent(
            [&res](Model * mm) -> void {
                for (const char * name : {"Home bubble", "School bubble"})
                    res.push_back(Bubbles::get_from(*mm, name)->get_bubble_id());
            },
            "record the partitions"
        );
        m->run(40, 123);
        return res;

    };

    REQUIRE(partitions(BubbleTies::Existing) == partitions(BubbleTies::Complete));

    // -- 2. Complete needs its layer, and a layer of its own -----------------
    // No household layer, or a network layer by that name: neither will do
    for (bool network_named_household : {false, true})
    {
        epimodels::ModelSIR m("flu", n, 0.01, 0.2, 0.1);
        m.verbose_off();
        m.add_layer(layer::network("network", graph::smallworld(6, 0.1)));
        if (network_named_household)
            m.add_layer(layer::network("household", graph::smallworld(2, 0.0)));

        Bubbles p(hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0,
            "Social bubble", 2u, "Bubble transmission factor",
            BubbleTies::Complete, "household");
        m.add_globalevent(p);

        REQUIRE_THROWS_WITH(m.run(5, 1), Catch::Matchers::Contains("make_layer"));
    }

    // A within-group layer by that name whose groups are not the households:
    // fewer groups, or as many but with other members
    for (size_t n_groups : {50u, 100u})
    {
        epimodels::ModelSIR m("flu", n, 0.01, 0.2, 0.1);
        m.verbose_off();
        m.add_layer(layer::network("network", graph::smallworld(6, 0.1)));

        std::vector< size_t > other(n);
        for (size_t i = 0u; i < n; ++i)
            other[i] = i % n_groups;

        auto wrong = layer::mixing("household");
        wrong->set_groups(assign::labels(other), n_groups);
        wrong->set_contact_prob(1.0);
        m.add_layer(wrong);

        Bubbles p(hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0,
            "Social bubble", 2u, "Bubble transmission factor",
            BubbleTies::Complete, "household");
        m.add_globalevent(p);

        REQUIRE_THROWS_WITH(m.run(5, 1), Catch::Matchers::Contains("one group per household"));
    }

    {
        epimodels::ModelSIR m("flu", n, 0.01, 0.2, 0.1);
        m.verbose_off();
        m.add_layer(layer::network("network", graph::smallworld(6, 0.1)));
        Bubbles a(hh, BubbleFlavor::Household, 2u, 0.0, 0, -1, 0,
            "Bubble A", 2u, "Factor A", BubbleTies::Complete, "household");
        Bubbles b(hh, BubbleFlavor::Household, 3u, 0.0, 0, -1, 0,
            "Bubble B", 2u, "Factor B", BubbleTies::Complete, "household");
        m.add_layer(a.make_layer());
        m.add_globalevent(a);
        m.add_globalevent(b);

        REQUIRE_THROWS_WITH(m.run(5, 1), Catch::Matchers::Contains("its own layer"));
    }

    // -- 3. Two policies, two layers: both run, every day ---------------------
    auto m = make(BubbleTies::Complete);
    REQUIRE_NOTHROW(m->run_multiple(20, 4, 5));
    }

}
