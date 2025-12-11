#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SIRCONN parallel", "[SIRCONN parallel]") {


    // Adding multi-file write
    auto sav_0 = make_save_run<>(
        "02-reproducible-sirconn-saves/main_out_%li", // std::string fmt,
        true,  // bool total_hist,
        false, // bool variant_info,
        false, // bool variant_hist,
        false, // bool tool_info,
        false, // bool tool_hist,
        true , // bool transmission,
        false, // bool transition,
        true,  // bool reproductive
        true,  // bool generation,
        true   // bool outbreak_size
    );

    auto sav_1 = make_save_run<>(
        "02-reproducible-sirconn-saves/main_out_pll_%li", // std::string fmt,
        true,  // bool total_hist,
        false, // bool variant_info,
        false, // bool variant_hist,
        false, // bool tool_info,
        false, // bool tool_hist,
        true , // bool transmission,
        false, // bool transition,
        true,  // bool reproductive
        true,  // bool generation,
        true   // bool outbreak_size
    );

    // Queuing doesn't matter and get results that are meaningful
    epimodels::ModelSIRCONN<> model_0("a virus", 1000, 0.01, 2, .3, .3);
    model_0.seed(112);
    model_0.verbose_off();
    model_0.print(true); // print before run
    model_0.run_multiple(100, 4, 1231, sav_0, true, true, 1);

    epimodels::ModelSIRCONN<> model_1("a virus", 1000, 0.01, 2, .3, .3);
    model_1.seed(112);
    model_1.verbose_off();
    model_1.run_multiple(100, 4, 1231, sav_1, true, true, 2);


    // Re running the models to see if it is reproducible
    model_0.seed(112);
    model_0.run_multiple(100, 4, 1231, sav_0, true, true, 2);

    model_1.seed(112);
    model_1.run_multiple(100, 4, 1231, sav_1, true, true, 2);

    std::vector< std::string > files({"reproductive", "total_hist"});
    for (auto f: files) 
    {

        for (size_t i = 0u; i < 4; ++i)
        {
            // Creating the file names
            std::string fname_0 = "02-reproducible-sirconn-saves/main_out_" + std::to_string(i) + "_" + f  + ".csv";
            std::string fname_1 = "02-reproducible-sirconn-saves/main_out_pll_" + std::to_string(i) + "_" + f  + ".csv";

            auto file0 = file_reader(fname_0);
            auto file1 = file_reader(fname_1);

            if (file0 != file1)
            {
                printf_epiworld("Files %s and %s are different\n", fname_0.c_str(), fname_1.c_str());
            }

            #ifdef CATCH_CONFIG_MAIN
            REQUIRE_THAT(file0, Catch::Equals(file1));
            #endif

        }

    }

    // tmat_0 = model_0.get_db().transition_probability(false);
    // tmat_1 = model_1.get_db().transition_probability(false);

    // #ifdef CATCH_CONFIG_MAIN
    // REQUIRE_THAT(tmat_0, Catch::Equals(tmat_1));
    // #else
    // printf_epiworld("Reproducibility test\n");
    // model_0.print(true);
    // model_1.print(true);
    // #endif 


    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}