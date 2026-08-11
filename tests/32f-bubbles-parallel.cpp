#include "tests.hpp"

using namespace epiworld;

// Builds a household id vector: agents 0..n-1 grouped in households of `hh_size`.
static std::vector<size_t> make_households(size_t n, size_t hh_size)
{
    std::vector<size_t> hh(n);
    for (size_t i = 0u; i < n; ++i)
        hh[i] = i / hh_size;
    return hh;
}

EPIWORLD_TEST_CASE("Bubbles - replicates are independent", "[bubbles]") {

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

        epimodels::ModelSEIR<> model("flu", 0.1, 0.2, 4.5, 1.0/8.0);
        model.seed(2024);
        model.agents_smallworld(n, 8, false, 0.10);

        Bubbles<> bubbles(hh, BubbleFlavor::Household, group, 0.0, 0, -1, 0);
        model.add_globalevent(bubbles);
        model.verbose_off();

        Results res;
        res.outbreak.assign(nsims, -1);

        model.run_multiple(
            60, nsims, 1231,
            [&res](size_t run_id, Model<> * m) -> void {

                // Outbreak size: everyone who left the susceptible state.
                int infected = 0;
                for (auto & a : m->get_agents())
                    if (a.get_state() != 0)
                        ++infected;

                res.outbreak[run_id] = infected;

                // Every transmission must be within a bubble of *this* model's
                // partition. Reading the partition off the model is the point:
                // each copy owns its own.
                const auto & bid =
                    Bubbles<>::get_from(*m)->get_bubble_id();

                std::vector<int> date, source, target, virus, sexp;
                m->get_db().get_transmissions(
                    date, source, target, virus, sexp
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
