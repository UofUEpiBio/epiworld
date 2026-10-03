#include "tests.hpp"
using namespace epiworld;
EPIWORLD_TEST_CASE("Saver callback hooks and simulation IDs", "[savers]") {
    std::vector<size_t> ids;
    struct ObservedSaver : SaverCallback<> {
        int begins = 0, ends = 0;
        using SaverCallback<>::SaverCallback;
        void begin(size_t n) override { REQUIRE(n == 7); ++begins; }
        void end() override { ++ends; }
    };
    ObservedSaver saver({}, [&](size_t id, RunOutputs&& out) {
        REQUIRE(out.total_hist.size() == 4 * 11);
        REQUIRE(out.total_hist.sim_id.front() == id);
        ids.push_back(id);
    });
    epimodels::ModelSEIRCONN<> model("virus", 100, 0.1, 5.0, 0.5, 3.0, 0.2);
    model.verbose_off();
    model.run_multiple(10, 7, 123, saver, true, false, 4);
    std::sort(ids.begin(), ids.end());
    REQUIRE(ids == std::vector<size_t>{0, 1, 2, 3, 4, 5, 6});
    REQUIRE(saver.begins == 1);
    SaverCallback<> failing({}, [](size_t, RunOutputs&&) {
        throw std::runtime_error("sink failed");
    });
    REQUIRE_THROWS_WITH(
        model.run_multiple(10, 7, 123, failing, true, false, 4), "sink failed");
    REQUIRE_THROWS(SaverFiles<>("%s"));
    REQUIRE_THROWS(SaverFiles<>("%n"));
    REQUIRE_THROWS(SaverFiles<>("%lu-%lu"));
    REQUIRE(saver.ends == 1);
    REQUIRE_THROWS(model.run_multiple(10, 0, 123, saver));
    REQUIRE(saver.begins == 1);
}
