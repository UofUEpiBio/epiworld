#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#define EPI_DEBUG_NO_THREAD_ID
#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SIR-omp", "[OMP-SIR]") {

    // Saver function
    auto saver_0 = epiworld::make_save_run<>(
        std::string("01c-sir-saves/%lu-episims-0"), true, true, true, false, true, true, true, true
        );

    auto saver_1 = epiworld::make_save_run<>(
        std::string("01c-sir-saves/%lu-episims-1"), true, true, true, false, true, true, true, true
        );

    // Queuing doesn't matter and get results that are meaningful
    epimodels::ModelSIR<> model_0(
        "a virus", 0.01, .9, .3
        );

    model_0.seed(1231);
    model_0.agents_smallworld(10000, 5, false, 0.01);
    model_0.verbose_off();
    model_0.run_multiple(100, 100, 1231, saver_0, true, true, 1);

    epimodels::ModelSIR<> model_1(
        "a virus", 0.01, .9, .3
        );

    model_1.seed(1231);
    model_1.agents_smallworld(10000, 5, false, 0.01);
    model_1.verbose_off();
    model_1.run_multiple(100, 100, 1231, saver_1, true, true, 2);

    std::vector< std::string > files = {
        "reproductive.csv",
        "tool_hist.csv",
        "total_hist.csv",
        "virus_info.csv"
    };
    for (size_t i = 0; i < 10; ++i)
    {

        for (auto f : files)
        {
            auto fn0 = file_reader(
                std::string("01c-sir-saves/") +
                std::to_string(i) +
                std::string("-episims-0_") +
                f
                );

            auto fn1 = file_reader(
                std::string("01c-sir-saves/") +
                std::to_string(i) +
                std::string("-episims-1_")  +
                f
                );

            if (fn0 != fn1)
                printf("Models ARE NOT equal in file %s %lu\n", f.c_str(), i);

            REQUIRE(fn0 == fn1);
        }
        
    }





}