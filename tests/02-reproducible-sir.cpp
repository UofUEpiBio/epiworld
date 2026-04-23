#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("SIR parallel", "[SIR parallel]") {

    auto fn_0 = epi_temp_file("01c-sir-0", "%li");
    auto fn_1 = epi_temp_file("01c-sir-1", "%li");

    // Adding multi-file write
    auto sav_0 = epiworld::make_save_run<>(
        fn_0.full_path.c_str(), // std::string fmt,
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

    auto sav_1 = epiworld::make_save_run<>(
        fn_1.full_path.c_str(), // std::string fmt,
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
    epimodels::ModelSIR<> model_0(
        "a virus", 0.01, .9, .3
        );
    
    model_0.seed(112);

    model_0.agents_smallworld(10000, 5, false, 0.01);
    
    model_0.verbose_off();

    model_0.run_multiple(100, 4, 1231, sav_0, true, true, 1);

    epimodels::ModelSIR<> model_1(
        "a virus", 0.01, .9, .3
        );

    model_1.seed(112);

    model_1.agents_smallworld(10000, 5, false, 0.01);

    model_1.run_multiple(100, 4, 1231, sav_1, true, true, 2);

    // Do the same file comparison done in 02-reproducible-sirconn.cpp
    std::vector< std::string > files({"reproductive", "total_hist"});
    for (auto f: files) 
    {
        for (size_t i = 0u; i < 4u; ++i)
        {

            std::string file_0 = fn_0.directory + "/" + std::to_string(i) + "_" + f +  + ".csv";
            std::string file_1 = fn_1.directory + "/" + std::to_string(i) + "_" + f +  + ".csv";
            

            auto file0 = file_reader(file_0);
            auto file1 = file_reader(file_1);

            if (file0 != file1)
            {
                std::cout << "Files " << file_0 << " and " << file_1 << " are different" << std::endl;
            }

            REQUIRE_THAT(file0, Catch::Equals(file1));


        }
    }




}

