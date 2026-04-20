#include "tests.hpp"
#include "../include/measles/measles.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Vaccine efficacy on measles school", "[ModelMeaslesSchoolVax]") {
    
    // Queuing doesn't matter and get results that are meaningful
    int n_seeds = 1;
    measles::ModelMeaslesSchool<> model_0(
        500,    // Number of agents
        n_seeds, // Number of initial cases
        20.0,     // Contact rate
        1.0,     // Transmission rate
        0.65,    // Vaccination efficacy
        0.3,     // Vaccination reduction recovery rate
        7.0,     // Incubation period
        4.0,     // Prodromal period
        5.0,     // Rash period
        3.0,     // Days undetected
        0.1,     // Hospitalization rate
        7.0,     // Hospitalization duration
        1.0,     // Proportion vaccinated
        21u,     // Quarantine period
        .8,      // Quarantine willingness
        4u       // Isolation period
    );

    // Setting the distribution function of the initial cases
    model_0.get_virus(0).set_distribution(
        [&n_seeds](Virus<> & v, Model<> * m) -> void {
        for (int i = 0; i < n_seeds; ++i)
            m->get_agents()[i].set_virus(*m, v);
        return;
    });

    size_t nsims = 500;
    std::vector< double > outbreak_sizes(nsims, 0.0);
    
    auto saver = [&outbreak_sizes](size_t n, Model<> * model) {

        // Extracting the outbreak size
        std::vector< int > date, virus, outbreak;
        model->get_db().get_outbreak_size(date, virus, outbreak);

        outbreak_sizes[n] = outbreak.back();
    };
    
    model_0.
        run_multiple(200, nsims, 1231, saver, true, true, 2).
        print(false);
    
    // Computing the mean of outbreak size
    double mean_outbreak_size = std::accumulate(
        outbreak_sizes.begin(), outbreak_sizes.end(), 0.0
    ) / static_cast<double>(nsims);


    double expected_outbreak_size = (1.0 - model_0("Vax efficacy")) *
        static_cast<double>(model_0.size());
    REQUIRE_FALSE(moreless(expected_outbreak_size, mean_outbreak_size, 10.0));

    std::cout <<
        "Expected outbreak size: " << expected_outbreak_size << "\n" <<
        "Observed outbreak size: " << mean_outbreak_size <<
        std::endl;
    
}
