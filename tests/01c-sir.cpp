#ifndef CATCH_CONFIG_MAIN
// #define EPI_DEBUG
#endif

#define EPI_DEBUG_NO_THREAD_ID
#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SIR-omp", "[OMP-SIR]") {

    // Saver function
    auto saver_0 = epiworld::make_save_run<int>(
        std::string("01c/%lu-episims-0")
        );

    auto saver_1 = epiworld::make_save_run<int>(
        std::string("01c/%lu-episims-1")
        );

    // Queuing doesn't matter and get results that are meaningful
    epimodels::ModelSIR<> model_0(
        "a virus", 0.01, .9, .3
        );

    model_0.seed(1231);
    model_0.agents_smallworld(100000, 5, false, 0.01);
    model_0.verbose_off();
    model_0.run_multiple(100, 10, 1231, saver_0, true, true, 1);

    epimodels::ModelSIR<> model_1(
        "a virus", 0.01, .9, .3
        );

    model_1.seed(1231);
    model_1.agents_smallworld(100000, 5, false, 0.01);
    model_1.verbose_off();
    model_1.run_multiple(100, 10, 1231, saver_1, true, true, 2);

    for (size_t i = 0; i < 10; ++i)
    {
        auto fn0 = file_reader(
            std::string("01c/") +
            std::to_string(i) +
            std::string("-episims-0.txt_total_hist.csv")
            );

        auto fn1 = file_reader(
            std::string("01c/") +
            std::to_string(i) +
            std::string("-episims-1.txt_total_hist.csv")
            );

        if (fn0 != fn1)
            printf("Models are not equal in file\n");
    }

    if (model_0.get_db() != model_1.get_db())
        printf("Models are not equal in db\n");

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}