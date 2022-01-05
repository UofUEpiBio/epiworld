#include <iostream>
#include <memory>
#include <vector>
#include <random>

#include "../include/epiworld/epiworld.hpp"

int main() {

    epiworld::Model<bool> m;
    epiworld::Virus<bool> v;
    epiworld::Tool<bool> t;

    m.pop_from_adjlist("../examples/edgelist.txt");

    m.add_virus(v, .5);
    m.add_tool(t, .5);

    m.init(1123, 50);

    // Cloning
    epiworld::Model<bool>m2(m);

    // Printing the addresses
    std::cout << 
        "Model           : " << &m <<", " << &m2 << std::endl <<
        "DataBases.model : " << m.get_db().get_model() <<", " << m2.get_db().get_model() << std::endl;

    // Looking at people
    std::cout << "Neighbors person[0] in m : " ;
    for (auto & n : m.get_population()->at(0u).get_neighbors())
        std::cout << n << ", ";
    std::cout << std::endl;


    std::cout << "Neighbors person[0] in m2 : " ;
    for (auto & n : m2.get_population()->at(0u).get_neighbors())
        std::cout << n << ", ";
    std::cout << std::endl;

    std::cout << "Person[0] in m viruses and tools  : " <<
        m.get_population()->at(0u).get_viruses().get_host() << ", " <<
        m.get_population()->at(0u).get_tools().get_person() << std::endl;

    std::cout << "Person[0] in m2 viruses and tools : " <<
        m2.get_population()->at(0u).get_viruses().get_host() << ", " <<
        m2.get_population()->at(0u).get_tools().get_person() << std::endl;
            

    return 0;
}