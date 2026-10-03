#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Savers are invariant to the number of threads", "[savers]") {

    epimodels::ModelSEIRCONN<> model("virus", 100, 0.1, 5.0, 0.5, 3.0, 0.2);
    model.verbose_off();

    SaveOptions all;
    RunOutputs::for_each_table([&](const char *, auto option, auto) {
        all.*option = true;
    });

    // Savers write without locks: each thread fills its own slots...
    SaverMemory<> one(all), four(all);
    model.run_multiple(10, 7, 91, one, true, false, 1);
    model.run_multiple(10, 7, 91, four, true, false, 4);
    REQUIRE(one.results() == four.results());

    // ...or writes its own files. Debug builds add the thread to each row.
    #ifndef EPI_DEBUG
    const std::string dir = epi_temp_file("40b-savers").directory + "/";
    SaverFiles<> files_one(dir + "one-%d", all), files_four(dir + "four-%d", all);
    model.run_multiple(10, 7, 91, files_one, true, false, 1);
    model.run_multiple(10, 7, 91, files_four, true, false, 4);

    for (int sim_id = 0; sim_id < 7; ++sim_id)
        RunOutputs::for_each_table([&](const char * name, auto, auto) {
            const std::string suffix =
                std::to_string(sim_id) + "_" + name + ".csv";
            REQUIRE(file_reader(dir + "one-" + suffix) ==
                file_reader(dir + "four-" + suffix));
        });
    #endif

}
