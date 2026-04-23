/**
 * @file shortcreek.cpp
 *
 * @brief C++ port of the Short Creek measles mixing model vignette.
 * This program replicates the simulation setup from the R vignette
 * `vignettes/short-creek.qmd` using only the epiworld C++ library and the
 * standard library. It builds a ModelMeaslesMixing model with Short Creek
 * population data, distributes entities and vaccination tools, and runs
 * multiple simulations.
 *
 * @note Compile with:
 *   g++ -std=c++17 -fopenmp -O2 -g -Wall -pedantic \
 *       -I$(Rscript -e "cat(system.file('include', package='epiworldR'))") \
 *       -o shortcreek shortcreek.cpp
 */

// The epiworld header-only library (provided by the epiworldR package)
#include "tests.hpp"
#include "../include/measles/measles.hpp"

/**
 * @brief Population record for a single age/school group.
 */
struct GroupData {
    std::string name; ///< Label (e.g. "under1", "5to11s1")
    int         pop;  ///< Population count
    int         age;  ///< Upper age limit
    double      vax;  ///< Vaccination rate (proportion)
};

EPIWORLD_TEST_CASE("ModelMeaslesMixing with Short Creek data", "[ModelMeaslesMixing][ShortCreek]") {

    // -----------------------------------------------------------------
    // 1. Data: short_creek (15 groups)
    //    Columns: age_labels, agepops, agelims, vacc_rate
    // -----------------------------------------------------------------
    std::vector<GroupData> short_creek = {
        {"under1",   113,  1, 0.0000000000},
        {"1to4",     450,  4, 0.2355555556},
        {"5to11s1",  281, 11, 0.0569395018},
        {"5to11s2",  393, 11, 0.3180661578},
        {"5to11s3",  213, 11, 0.3661971831},
        {"12to13s4",  85, 13, 0.2235294118},
        {"12to13s5", 149, 13, 0.3557046980},
        {"12to13s6",  83, 13, 0.5783132530},
        {"14to17s7", 178, 17, 0.1460674157},
        {"14to17s8", 169, 17, 0.2307692308},
        {"14to17s9", 320, 17, 0.2812500000},
        {"18to24",   892, 24, 0.2331838565},
        {"25to44",  1279, 44, 0.5770132916},
        {"45to69",   962, 69, 0.9209979210},
        {"70+",       63, 90, 1.0000000000}
    };

    // -----------------------------------------------------------------
    // 2. Data: short_creek_matrix (15x15, column-major order)
    //    Row-stochastic contact mixing matrix.
    // -----------------------------------------------------------------
    // clang-format off
    std::vector<double> contact_matrix = {
        // Column 0 (under1)
        0.101569154623368, 0.021010469619477, 0.005151685928808,
        0.005151685928808, 0.005151685928808, 0.002010347357566,
        0.002010347357566, 0.002010347357566, 0.002391931464823,
        0.002391931464823, 0.002391931464823, 0.003681544594600,
        0.011302443804954, 0.007708137792522, 0.004987824514901,
        // Column 1 (1to4)
        0.171006930027974, 0.347388210444288, 0.055078853901556,
        0.055078853901556, 0.055078853901556, 0.014993533783853,
        0.014993533783853, 0.014993533783853, 0.010909054020753,
        0.010909054020753, 0.010909054020753, 0.022514127762116,
        0.069819445303705, 0.050916073154859, 0.052359066261088,
        // Column 2 (5to11s1)
        0.048215803664504, 0.063335466762203, 0.498079576961765,
        0.059540709497840, 0.059540709497840, 0.047197268043520,
        0.047197268043520, 0.047197268043520, 0.012882064133479,
        0.012882064133479, 0.012882064133479, 0.013202652493887,
        0.044556627545654, 0.031600688525445, 0.034168257090256,
        // Column 3 (5to11s2)
        0.067433490534342, 0.088579496219024, 0.083272237838616,
        0.521811105302541, 0.083272237838616, 0.066008990537734,
        0.066008990537734, 0.066008990537734, 0.018016552329030,
        0.018016552329030, 0.018016552329030, 0.018464919680063,
        0.062315852759580, 0.044195980749109, 0.047786921837974,
        // Column 4 (5to11s3)
        0.036547922350674, 0.048008734591990, 0.045132281576655,
        0.045132281576655, 0.483671149040580, 0.035775865100604,
        0.035775865100604, 0.035775865100604, 0.009764696300466,
        0.009764696300466, 0.009764696300466, 0.010007704559424,
        0.033774240808627, 0.023953546818220, 0.025899782064856,
        // Column 5 (12to13s4)
        0.007782566361142, 0.007131449003924, 0.019522191041155,
        0.019522191041155, 0.019522191041155, 0.407414533833567,
        0.041993009752449, 0.041993009752449, 0.021257827334479,
        0.021257827334479, 0.021257827334479, 0.004874055722452,
        0.011147902561990, 0.011250100563511, 0.013240169146999,
        // Column 6 (12to13s5)
        0.013642381033060, 0.012501010606879, 0.034221252530966,
        0.034221252530966, 0.034221252530966, 0.073611275918999,
        0.439032800000118, 0.073611275918999, 0.037263720856910,
        0.037263720856910, 0.037263720856910, 0.008543932972298,
        0.019541617432195, 0.019720764517214, 0.023209237681209,
        // Column 7 (12to13s6)
        0.007599447152644, 0.006963650203832, 0.019062845369598,
        0.019062845369598, 0.019062845369598, 0.041004938934745,
        0.041004938934745, 0.406426463015863, 0.020757643161903,
        0.020757643161903, 0.020757643161903, 0.004759372058394,
        0.010885598972296, 0.010985392314958, 0.012928635755304,
        // Column 8 (14to17s7)
        0.019957391509568, 0.011183150946212, 0.011484192688625,
        0.011484192688625, 0.011484192688625, 0.045816529718160,
        0.045816529718160, 0.045816529718160, 0.502429691020368,
        0.051565914857751, 0.051565914857751, 0.039691273064406,
        0.022845483039361, 0.026678696152192, 0.022000473601511,
        // Column 9 (14to17s8)
        0.018948309916387, 0.010617710729830, 0.010903531260548,
        0.010903531260548, 0.010903531260548, 0.043499963608815,
        0.043499963608815, 0.043499963608815, 0.048958649499774,
        0.499822425662391, 0.048958649499774, 0.037684410943172,
        0.021690374346360, 0.025329773313037, 0.020888090104806,
        // Column 10 (14to17s9)
        0.035878456646414, 0.020104541026897, 0.020645739664944,
        0.020645739664944, 0.020645739664944, 0.082366794998939,
        0.082366794998939, 0.082366794998939, 0.092702768283597,
        0.092702768283597, 0.543566544446213, 0.071355097643877,
        0.041070531306717, 0.047961700947762, 0.039551413216200,
        // Column 11 (18to24)
        0.086893826704095, 0.065288401191126, 0.033295076640263,
        0.033295076640263, 0.033295076640263, 0.029716497383995,
        0.029716497383995, 0.029716497383995, 0.112279078986019,
        0.112279078986019, 0.112279078986019, 0.515668581071570,
        0.155279762670087, 0.153026364331379, 0.109824227239486,
        // Column 12 (25to44)
        0.285835701601053, 0.216941460351065, 0.120397202627888,
        0.120397202627888, 0.120397202627888, 0.072825848086815,
        0.072825848086815, 0.072825848086815, 0.069245158824773,
        0.069245158824773, 0.069245158824773, 0.166379615564338,
        0.346565482130962, 0.292848765919280, 0.256446665790226,
        // Column 13 (45to69)
        0.096904205532963, 0.078644817109726, 0.042447260784826,
        0.042447260784826, 0.042447260784826, 0.036534051040717,
        0.036534051040717, 0.036534051040717, 0.040197839449756,
        0.040197839449756, 0.040197839449756, 0.081508060275207,
        0.145576890298048, 0.245172299948029, 0.304026405685853,
        // Column 14 (70+)
        0.001784412341810, 0.002301431193527, 0.001306071183787,
        0.001306071183787, 0.001306071183787, 0.001223561651970,
        0.001223561651970, 0.001223561651970, 0.000943324333871,
        0.000943324333871, 0.000943324333871, 0.001664651594194,
        0.003627747019465, 0.008651714952484, 0.032682830009331
    };
    // clang-format on

    // -----------------------------------------------------------------
    // 3. Derived quantities (matching the R vignette)
    // -----------------------------------------------------------------

    // Total population
    int N = 0;
    for (const auto & g : short_creek)
        N += g.pop;

    // Hospitalization rate for a 10% hospitalization probability:
    //   P(hosp) = h_rate / (h_rate + recovery_rate)
    //   recovery_rate = 1 / rash_period = 1/3
    //   => h_rate = P(hosp) * recovery_rate / (1 - P(hosp))
    double rash_period       = 3.0;
    double hosp_probability  = 0.1;
    double recovery_rate     = 1.0 / rash_period;
    double h_rate            = hosp_probability * recovery_rate /
                               (1.0 - hosp_probability);

    // -----------------------------------------------------------------
    // 4. Create the model
    //    (mirrors the R call to ModelMeaslesMixing)
    // -----------------------------------------------------------------

    // Vignette parameters
    double transmission_rate = 0.9;
    double prodromal_period  = 4.0;

    // contact_rate = 15 / transmission_rate / prodromal_period
    double contact_rate = 15.0 / transmission_rate / prodromal_period;

    // Scale the contact matrix by the contact rate
    // (contact_rate is now absorbed into the contact matrix)
    for (auto & v : contact_matrix)
        v *= contact_rate;

    measles::ModelMeaslesMixing<> measles_model(
        static_cast<epiworld_fast_uint>(N),     // n
        1.0 / static_cast<double>(N),           // prevalence
        transmission_rate,                       // transmission_rate
        0.97,                                    // vax_efficacy
        0.5,                                     // vax_reduction_recovery_rate (default)
        12.0,                                    // incubation_period
        prodromal_period,                        // prodromal_period
        rash_period,                             // rash_period
        contact_matrix,                          // contact_matrix (column-major)
        h_rate,                                  // hospitalization_rate
        7.0,                                     // hospitalization_period
        2.0,                                     // days_undetected
        21,                                      // quarantine_period
        0.9,                                     // quarantine_willingness
        0.9,                                     // isolation_willingness
        4,                                       // isolation_period
        0.95,                                    // prop_vaccinated
        0.8,                                     // contact_tracing_success_rate
        4u                                       // contact_tracing_days_prior
    );

    // -----------------------------------------------------------------
    // 5. Add entities from the population data
    //    (mirrors the R function add_entities_from_dataframe)
    // -----------------------------------------------------------------
    int roll_sum = 0;
    for (const auto & g : short_creek)
    {
        int from = roll_sum;
        int to   = roll_sum + g.pop;

        // Create an entity and set its distribution to a contiguous
        // range of agents [from, to).
        epiworld::Entity<> e(
            g.name,
            epiworld::distribute_entity_to_range<>(from, to)
        );

        measles_model.add_entity(e);
        roll_sum = to;
    }

    // -----------------------------------------------------------------
    // 6. Distribute vaccination tool by entity
    //    (mirrors distribute_tool_to_entities + set_distribution_tool)
    // -----------------------------------------------------------------
    std::vector<double> vacc_rates;
    vacc_rates.reserve(short_creek.size());
    for (const auto & g : short_creek)
        vacc_rates.push_back(g.vax);

    measles_model.get_tool(0).set_distribution(
        epiworld::distribute_tool_to_entities<>(
            vacc_rates,
            true // as_proportion
        )
    );

    // -----------------------------------------------------------------
    // 7. Run 200 simulations for 200 days
    //    (mirrors run_multiple with make_saver for outbreak_size and
    //     hospitalizations)
    // -----------------------------------------------------------------
    auto save_fmt = epi_temp_file("27-shortcreek");

    measles_model.run_multiple(
        200u,  // ndays
        10u,  // nsims
        52,  // seed
        epiworld::make_save_run<>(
            save_fmt.full_path.c_str(),    // file name format
            false,             // total_hist
            false,             // virus_info
            false,             // virus_hist
            false,             // tool_info
            false,             // tool_hist
            false,             // transmission
            false,             // transition
            true,             // reproductive
            false,             // generation
            false,             // active_cases
            true,              // outbreak_size
            true               // hospitalizations
        ),
        true,  // reset between runs
        true,  // verbose
        4      // nthreads
    );

    //

    measles_model.print();

    measles_model.run(200, 1071959870).print();

}
