#include "tests.hpp"

using namespace epiworld;

// The queue, reinfection and variants, on a natural history.
//
// Model A: S -> I (infectious) -> T (immune for a fixed 4 days) -> S again,
// and half the agents start in P (protected for a fixed 5 days, then S).
// Agents in P have no infected neighbors and no rates, so only the queue the
// engine derives from the stages visits them. Whether the model pulls,
// pushes or picks:
// 1. Every agent in P leaves on day 5 exactly (none is left behind).
// 2. Immunity lasts 4 days exactly, and some agents are infected again: their
//    episode count goes up and their entry days are of the latest infection.
// 3. Pull, push and auto give the same mean final counts (fixed network,
//    many runs).
//
// Model B: an SIR whose virus reads "Recovery rate (fast)" (1.0) in place of
// "Recovery rate" (0.01). Every infected agent recovers the next day.
//
// Model C: the same variant, born by mutation during the run.

namespace {

constexpr int S = 0, I = 1, T = 2, P = 3;
constexpr size_t N = 1000u;

std::shared_ptr< Model > make_model_a(const std::string & mode, const AdjList & net)
{

    Virus v("v");
    v.add_stage("Susceptible");
    v.add_stage("Infected", stage::infectious);
    v.add_stage("Temporarily immune");
    v.add_stage("Protected");
    v.infection(S, I, 0.3);
    v.rate(I, T, 0.25).effect(Effect::recovery);
    v.after(T, S, Duration::fixed(4));
    v.after(P, S, Duration::fixed(5));

    auto model = std::make_shared< Model >(N);
    model->add_virus(v, assign::count(10));
    model->set_initial(assign::shares({{P, 0.5}}));
    model->add_layer(layer::network("net", net));
    model->set_transmission_mode(mode);
    model->verbose_off();
    return model;

}

} // namespace

EPIWORLD_TEST_CASE("Natural history - queue and reinfection and variants", "[natural-history]") {

    // A fixed network, drawn once
    Model scratch(N);
    scratch.seed(31);
    const AdjList net = rgraph_smallworld(N, 4, 0.05, false, scratch);

    std::vector< double > mean_total(3u, 0.0);
    const std::vector< std::string > modes = {"pull", "push", "auto"};

    for (size_t m = 0u; m < modes.size(); ++m)
    {

        // 1. and 2., on one run
        {
            auto model = make_model_a(modes[m], net);
            model->run(40, 15);

            std::vector< int > date, counts;
            std::vector< std::string > state;
            model->get_db().get_hist_total(&date, nullptr, &state, &counts);

            for (size_t k = 0u; k < date.size(); ++k)
                if (state[k] == "Protected")
                {
                    if (date[k] < 5)
                        REQUIRE(counts[k] == 495);
                    else
                        REQUIRE(counts[k] == 0);
                }

            int reinfected = 0;
            for (size_t i = 0u; i < N; ++i)
            {
                const int t_in = model->get_stage_entered(i, T);
                const int s_in = model->get_stage_entered(i, S);
                if ((t_in >= 0) && (s_in > t_in))
                    REQUIRE(s_in - t_in == 4);

                if (model->get_episode(i) > 1)
                {
                    // Entry days are of the current infection: it started in
                    // I, the day-0 stage (P or S) is forgotten, and S, if
                    // entered again, came after the immunity
                    ++reinfected;
                    const int i_in = model->get_stage_entered(i, I);
                    REQUIRE(i_in >= 0);
                    REQUIRE(model->get_stage_entered(i, P) == -1);
                    if (t_in >= 0)
                        REQUIRE(t_in > i_in);
                    if (s_in >= 0)
                        REQUIRE(s_in > t_in);
                }
            }
            REQUIRE(reinfected > 0);
        }

        // 3. The mean final counts of many runs
        auto model = make_model_a(modes[m], net);
        std::vector< double > final_i;
        auto saver = [&final_i](size_t, Model * mod) -> void {
            final_i.push_back(mod->get_db().get_today_total_stage("Infected"));
        };
        model->run_multiple(40, 200, 2024, saver, true, false);

        for (auto x : final_i)
            mean_total[m] += x / static_cast< double >(final_i.size());

    }

    // Same distribution: the means are within a few percent of each other
    REQUIRE(std::abs(mean_total[1] - mean_total[0]) / mean_total[0] < 0.05);
    REQUIRE(std::abs(mean_total[2] - mean_total[0]) / mean_total[0] < 0.05);

    // Model B: a variant reads its own parameter
    {
        epimodels::ModelSIR model("v", N, 0.0, 0.5, 0.01);
        model.add_param(1.0, "Recovery rate (fast)");

        // A strain: a copy of the model's virus, reading its own parameter
        Virus fast = model.get_virus(0);
        fast.set_name("fast");
        fast.use_param("Recovery rate", "Recovery rate (fast)");
        model.add_virus(fast, assign::count(20));

        model.add_layer(layer::network("net", net));
        model.verbose_off();
        model.run(30, 3);

        int n_recovered = 0;
        for (size_t i = 0u; i < N; ++i)
        {
            const int in = model.get_stage_entered(i, 1);
            const int out = model.get_stage_entered(i, 2);
            if ((in >= 0) && (out >= 0))
            {
                REQUIRE(out - in == 1);
                ++n_recovered;
            }
        }
        REQUIRE(n_recovered >= 20);
    }

    // Model C: a variant born during the run. The original never recovers;
    // from day 5 on, every carrier of it mutates into a variant that reads
    // "Recovery rate (fast)" (1.0). The carriers infected from then on carry
    // a variant and recover the next day; the database records each
    // mutation as a variant, a child of the original.
    {
        epimodels::ModelSIR model("v", N, 0.0, 0.5, 0.0);
        model.add_param(1.0, "Recovery rate (fast)");
        model.assign_virus(0, assign::count(10));

        model.get_virus(0).set_mutation(
            [](Agent *, Virus & v, Model * m) -> bool {
                if ((m->today() < 5) || (v.get_id() != 0))
                    return false;
                v.use_param("Recovery rate", "Recovery rate (fast)");
                return true;
            }
        );

        model.add_layer(layer::network("net", net));
        model.verbose_off();
        model.run(30, 7);

        // Infected on day 7 or later (by a variant carrier): out the next day
        int n_variant = 0, n_never = 0;
        for (size_t i = 0u; i < N; ++i)
        {
            const int in = model.get_stage_entered(i, 1);
            const int out = model.get_stage_entered(i, 2);
            if (in >= 7)
            {
                REQUIRE(out - in == 1);
                ++n_variant;
            }
            else if ((in >= 0) && (in < 5) && (out < 0))
                ++n_never;
        }
        REQUIRE(n_variant > 20);
        REQUIRE(n_never == 0);   // the early carriers mutated, then recovered

        auto info = model.get_db().get_run_outputs({"virus_info"}).at("virus_info");
        const auto & parent = std::get< std::vector< int > >(info.columns[4u]);
        REQUIRE(parent.size() >= 2u);
        REQUIRE(parent[0u] == -99);
        for (size_t k = 1u; k < parent.size(); ++k)
            REQUIRE(parent[k] == 0);
    }

    // Guards: a virus in a model keeps its natural history, and a variant
    // must read parameters the model has
    {
        epimodels::ModelSIR model("v", N, 0.01, 0.5, 0.1);
        REQUIRE_THROWS(model.get_virus(0).add_stage("Another"));
        REQUIRE_THROWS(model.get_virus(0).rate(1, 0, 0.1));

        model.get_virus(0).set_mutation(
            [](Agent *, Virus & v, Model *) -> bool {
                if (v.get_id() != 0)
                    return false;
                v.use_param("Recovery rate", "No such parameter");
                return true;
            }
        );
        model.add_layer(layer::network("net", net));
        model.verbose_off();
        REQUIRE_THROWS(model.run(5, 1));
    }

}
