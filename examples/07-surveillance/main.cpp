#define EPI_DEBUG
#include "../../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main(int argc, char* argv[]) {

    unsigned int ndays       = 100;
    unsigned int popsize     = 10000;
    unsigned int preval      = 10;
    epiworld_double sur_prob = 0.001;
    if (argc == 5)
    {
        ndays    = strtol(argv[1], nullptr, 0);
        popsize  = strtol(argv[2], nullptr, 0);
        preval   = strtol(argv[3], nullptr, 0);
        sur_prob = strtod(argv[4], nullptr);
    }
    else if (argc != 1)
        std::logic_error("What is the surveillance prob?");

    epiworld::Model<> model = models::surveillance(
        "a virus", // Name of the virus
        preval, // prevalence
        0.9,  // efficacy_vax
        3u,   // latent_period
        12u,   // infect_period
        .7,   // prob_symptoms
        .25,  // prop_vaccinated
        .5,   // prop_vax_redux_transm
        .5,   // prop_vax_redux_infect
        sur_prob,  // surveillance_prob
        1.0,   // prob_transmission
        0.001, // Prob death
        0.1    // Prob re-infect
    );

    // Adding a bernoulli graph as step 0
    model.agents_from_adjlist(
        epiworld::rgraph_smallworld(popsize, 5, .01, false, model)
    );

    // Initializing and printing
    model.init(ndays, 123);

    // Running and checking the results
    model.print();
    model.run();
    model.print();

    model.write_data(
        "","", "", "", "07-surveillance_hist.txt", "", "", ""
    );

    model.get_user_data().write("07-surveillance_user_data.txt");

    return 0;

}