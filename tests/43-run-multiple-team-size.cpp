#include "tests.hpp"

using namespace epiworld;

// OpenMP can grant run_multiple() fewer threads than requested (see #303).
EPIWORLD_TEST_CASE("run_multiple runs every replicate in a smaller team", "[run-multiple-team-size]") {

    const size_t nreps = 10u;

    epimodels::ModelSIR<> model("a virus", 0.01, 0.5, 0.3);
    model.agents_smallworld(200, 4, false, 0.1).verbose_off();

    // Reference: the full team of four threads
    SaverMemory expected;
    model.run_multiple(20, nreps, 123, expected, true, false, 4);

    // Same, counting the callbacks per run id (they never run concurrently)
    std::vector< int > calls(nreps, 0);
    SaverMemory saver;
    auto fun = [&calls, saver](size_t run_id, Model<> * m) {
        ++calls[run_id];
        saver(run_id, m);
    };

    #ifdef _OPENMP
    // With nested parallelism off, run_multiple() called from within a
    // parallel region gets a team of one thread instead of four.
    int max_levels = omp_get_max_active_levels();
    omp_set_max_active_levels(1);

    #pragma omp parallel num_threads(2)
    {
        #pragma omp single
        model.run_multiple(20, nreps, 123, fun, true, false, 4);
    }

    omp_set_max_active_levels(max_levels);
    #else
    model.run_multiple(20, nreps, 123, fun, true, false, 4);
    #endif

    REQUIRE(calls == std::vector< int >(nreps, 1));
    REQUIRE(model.get_n_replicates() == 2u * nreps);
    REQUIRE(
        saver.results().at("total_hist").columns ==
        expected.results().at("total_hist").columns
    );

}
