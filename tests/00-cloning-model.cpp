#include "tests.hpp"

using namespace epiworld;

EPIWORLD_TEST_CASE("Cloning", "[clone]") {

    epiworld::Model<> m(1000);

    auto susceptible_state = m.add_state("Susceptible", default_update_susceptible<>);
    auto recovered_state = m.add_state("Recovered");

    epiworld::Virus<> v("covid 19", 0.5, true);
    epiworld::Tool<> t("vax", .5, true);
    v.set_state(susceptible_state, recovered_state);

    m.seed(1333);
    m.add_layer(std::make_unique< layer::Network<> >(
        "network", rgraph_smallworld(1000, 5, .01, false, m)
    ));

    m.add_virus(v);
    m.add_tool(t);

    // Cloning
    epiworld::Model<> m2(m);

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
