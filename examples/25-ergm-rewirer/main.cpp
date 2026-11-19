#include "../../include/epiworld/epiworld.hpp"

#include <unordered_set>

using namespace epiworld;

// A network layer whose ties form and dissolve every day, as in a separable
// temporal ERGM (STERGM; Krivitsky and Handcock 2014), with given
// coefficients (estimate them in R with `tergm`).
//
// Each day, from yesterday's ties y(t-1):
//
// - Formation: a few Metropolis-Hastings toggles on the dyads not tied
//   yesterday, under an ERGM with three terms (`tergm`'s `edges`,
//   `nodematch("group")` and `concurrent`) evaluated on the formation network
//   y+ (yesterday's ties and today's new ones). A dyad (i, j) not tied
//   yesterday is proposed at random; if it is not tied, the tie is added with
//   probability min(1, exp(theta . delta)), and if it was formed today, it is
//   removed with probability min(1, exp(-theta . delta)), where delta is the
//   change in the statistics: 1 tie, 1 if i and j share a group, and how many
//   of the two get a second partner (`concurrent`: agents with two or more).
// - Dissolution: each of yesterday's ties ends with probability 1 / D, D
//   being the mean tie duration in days (`tergm`'s dissolution `edges`
//   coefficient is log(D - 1)).
//
// The coefficients and D are model parameters, so a global event or a
// calibration can change them. The number of proposals per day is a setting
// of the rewirer: a handful per agent is a few MCMC steps, as `tergm`'s
// simulation takes per time step; more steps get closer to an exact draw.
layer::Network::Rewiring stergm(
    std::vector< size_t > group,  // Each agent's group (for nodematch)
    size_t proposals              // Formation proposals per day
)
{
    ParamRef theta_edges("Formation edges");
    ParamRef theta_match("Formation match");
    ParamRef theta_concurrent("Formation concurrent");
    ParamRef duration("Tie duration");

    return [=](layer::Network & net, Model & model) mutable -> void {

        const size_t n = net.size();
        if (group.size() != n)
            throw std::length_error("stergm(): one group per agent.");

        const double te = theta_edges(model);
        const double tm = theta_match(model);
        const double tc = theta_concurrent(model);
        const double d = duration(model);
        if (d < 1.0)
            throw std::range_error("stergm(): the tie duration must be >= 1.");

        // The dyads formed today: the only ones formation may remove
        auto key = [n](size_t i, size_t j) -> uint64_t {
            return (i < j) ? (uint64_t(i) * n + j) : (uint64_t(j) * n + i);
        };
        std::unordered_set< uint64_t > formed;

        // Formation, on y+ (it starts as yesterday's ties)
        for (size_t s = 0u; s < proposals; ++s)
        {
            const size_t i = std::min(static_cast< size_t >(model.runif() * n), n - 1u);
            const size_t j = std::min(static_cast< size_t >(model.runif() * n), n - 1u);
            if (i == j)
                continue;

            const bool tied = net.has_edge(i, j);
            const bool today = tied && (formed.count(key(i, j)) > 0u);
            if (tied && !today)
                continue; // Yesterday's tie: dissolution decides

            // The change in `concurrent` counts the ends whose degree in y+
            // without the tie is 1 (adding it gives them a second partner)
            const size_t di = net.get_degree(i) - (tied ? 1u : 0u);
            const size_t dj = net.get_degree(j) - (tied ? 1u : 0u);
            const double delta = te +
                ((group[i] == group[j]) ? tm : 0.0) +
                tc * static_cast< double >((di == 1u) + (dj == 1u));

            if (!tied)
            {
                if ((delta >= 0.0) || (model.runif() < std::exp(delta)))
                {
                    net.add_edge(i, j);
                    formed.insert(key(i, j));
                }
            }
            else if ((delta <= 0.0) || (model.runif() < std::exp(-delta)))
            {
                net.rm_edge(i, j);
                formed.erase(key(i, j));
            }
        }

        // Dissolution, of yesterday's ties
        std::vector< std::pair< size_t, size_t > > ending;
        for (size_t i = 0u; i < n; ++i)
        {
            auto nb = net.neighbors_view(i);
            for (size_t k = 0u; k < nb.size(); ++k)
            {
                const size_t j = nb.id(k);
                if ((j > i) && (formed.count(key(i, j)) == 0u) &&
                    (model.runif() < 1.0 / d))
                    ending.emplace_back(i, j);
            }
        }
        for (const auto & e : ending)
            net.rm_edge(e.first, e.second);

    };
}

int main() {

    const size_t n = 2000u;

    // Two groups of 1,000 (e.g., two neighborhoods)
    std::vector< size_t > group(n);
    for (size_t i = 0u; i < n; ++i)
        group[i] = (i < n / 2u) ? 0u : 1u;

    // An infection that spreads through partnerships (SIS), on a static
    // network and on one whose ties form and dissolve, both starting from the
    // same random network (0.8 partners per agent)
    auto build = [&](bool dynamic) {

        auto model = std::make_unique< epimodels::ModelSIS >(
            "an STI", // Name of the virus
            n,        // Number of agents
            0.02,     // Initial prevalence
            0.05,     // Transmission probability (per partner and day)
            1.0 / 60  // Recovery rate
        );

        // Ties within a group form four times as often, a second partner
        // is less likely (odds times 0.25), and ties last 50 days on average
        model->add_param(-5.0, "Formation edges");
        model->add_param(std::log(4.0), "Formation match");
        model->add_param(std::log(0.25), "Formation concurrent");
        model->add_param(50.0, "Tie duration");

        auto partners = layer::network(
            "partners", graph::bernoulli(0.8 / static_cast< double >(n - 1u))
        );
        if (dynamic)
            partners->set_rewiring(stergm(group, n));
        model->add_layer(partners);
        model->verbose_off();
        return model;

    };

    // The network and the prevalence every 50 days
    auto describe = [&](Model * m) {

        if ((m->today() % 50) != 0)
            return;

        auto & net = *m->get_layer("partners").as_network();
        size_t ties = 0u, within = 0u, concurrent = 0u, infected = 0u;
        for (size_t i = 0u; i < n; ++i)
        {
            auto nb = net.neighbors_view(i);
            concurrent += (nb.size() >= 2u) ? 1u : 0u;
            infected += (m->get_agent(i).get_virus() != nullptr) ? 1u : 0u;
            for (size_t k = 0u; k < nb.size(); ++k)
                if (nb.id(k) > i)
                {
                    ++ties;
                    within += (group[i] == group[nb.id(k)]) ? 1u : 0u;
                }
        }

        printf_epiworld(
            "  day %3i: mean degree %4.2f, within a group %5.1f%%, "
            "concurrent %4.1f%%, infected %4.1f%%\n",
            m->today(), 2.0 * ties / n,
            ties > 0u ? 100.0 * within / ties : 0.0,
            100.0 * concurrent / n, 100.0 * infected / n
        );

    };

    for (bool dynamic : {false, true})
    {
        auto model = build(dynamic);
        printf_epiworld(
            "%s network:\n", dynamic ? "A changing (STERGM)" : "A static"
        );
        model->add_globalevent(describe, "Describe");
        model->run(300, 1231);
        printf_epiworld("\n");
    }

    return 0;

}
