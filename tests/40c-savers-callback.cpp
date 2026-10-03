#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Saver hooks, failures, and file formats", "[savers]") {

    epimodels::ModelSEIRCONN<> model("virus", 100, 0.1, 5.0, 0.5, 3.0, 0.2);

    struct CountingSaver : SaverCallback<>
    {
        using SaverCallback<>::SaverCallback;
        size_t begins = 0u, ends = 0u, nexperiments = 0u;
        void begin(size_t n) override { ++begins; nexperiments = n; }
        void end() override { ++ends; }
    };

    // begin() and end() run once, and the callback once per simulation
    std::vector< size_t > ids;
    CountingSaver saver({}, [&](size_t sim_id, RunOutputs && outputs) {
        REQUIRE(outputs.total_hist.size() == 11u * 4u);
        REQUIRE(outputs.total_hist.sim_id.front() == static_cast<int>(sim_id));
        ids.push_back(sim_id);
    });
    model.run_multiple(10, 7, 123, saver, true, false, 4);
    std::sort(ids.begin(), ids.end());
    REQUIRE(ids == std::vector< size_t >{0, 1, 2, 3, 4, 5, 6});
    REQUIRE(saver.begins == 1u);
    REQUIRE(saver.nexperiments == 7u);
    REQUIRE(saver.ends == 1u);

    // A failure is rethrown, skips end(), and leaves the model verbose
    CountingSaver failing({}, [](size_t, RunOutputs &&) {
        throw std::runtime_error("sink failed");
    });
    model.verbose_on();
    REQUIRE_THROWS_WITH(
        model.run_multiple(10, 7, 123, failing, true, false, 4), "sink failed"
    );
    REQUIRE(failing.ends == 0u);
    REQUIRE(model.get_verbose());

    // Bad arguments are rejected before begin()
    REQUIRE_THROWS(model.run_multiple(10, 0, 123, saver));
    REQUIRE(saver.begins == 1u);

    SaverMemory<> memory;
    REQUIRE_THROWS_AS(memory.write(0u, RunOutputs()), std::out_of_range);

    // File names take exactly one integer placeholder
    for (const char * format : {"%s", "%n", "%lu-%lu", "sim", "%#x"})
        REQUIRE_THROWS_AS(SaverFiles<>(format), std::invalid_argument);

    for (const char * format : {"%03lu-sim", "%zu", "sim-%d", "%lld"})
        REQUIRE_NOTHROW(SaverFiles<>(format));

}
