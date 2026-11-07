#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Cloning", "[clone]") {

    epiworld::Model m(1000);

    epiworld::Virus v("covid 19");
    v.add_stage("Susceptible");
    v.add_stage("Infected", stage::infectious);
    v.infection(0, 1, 0.5);
    epiworld::Tool t("vax");

    m.seed(1333);
    m.add_layer(layer::network(
        "network", rgraph_smallworld(1000, 5, .01, false, m)
    ));

    m.add_virus(v, assign::share(0.5));
    m.add_tool(t, assign::share(.5));

    // Cloning
    epiworld::Model m2(m);

    // Printing the addresses
    std::cout <<
        "Model           : " << &m <<", " << &m2 << std::endl <<
        "DataBases.model : " << m.get_db().get_model() <<", " << m2.get_db().get_model() << std::endl;

    // Looking at people
    std::cout << "Neighbors agent[0] in m : " ;
    for (auto * n : tests_net(m).neighbors_view(0u))
        std::cout << n << ", ";
    std::cout << std::endl;


    std::cout << "Neighbors agent[0] in m2 : " ;
    for (auto * n : tests_net(m2).neighbors_view(0u))
        std::cout << n << ", ";
    std::cout << std::endl;


}
