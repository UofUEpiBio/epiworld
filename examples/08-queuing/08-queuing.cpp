#include "../../include/epiworld/epiworld.hpp"
#include "../../include/epiworld/models/surveillance.hpp"

int main(int argc, char* argv[]) {


    unsigned int ndays       = 100;
    unsigned int popsize     = 20000;
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


    epiworld::Model<bool> surveillance1;
    epiworld::Model<bool> surveillance2;

    set_up_surveillance(surveillance1, "covid19");    
    set_up_surveillance(surveillance2, "covid19");

    surveillance1.print_status_codes();

    // Adding a bernoulli graph as step 0
    epiworld::AdjList rgraph = epiworld::rgraph_smallworld(popsize, 3, .01, false, surveillance1);
    surveillance1.pop_from_adjlist(rgraph);
    surveillance2.pop_from_adjlist(rgraph);

    // Setting up queuing or not
    surveillance1.queuing_off();
    surveillance2.queuing_on();

    // Initializing and printing
    surveillance1.init(ndays, 123);
    surveillance2.init(ndays, 123);

    // Running and checking the results
    surveillance1.run();
    surveillance1.print();

    surveillance2.run();
    surveillance2.print();

    return 0;

}
