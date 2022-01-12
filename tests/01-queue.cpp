#include "../include/epiworld/epiworld.hpp"

using namespace epiworld;

int main()
{

    Model<> model;
    model.pop_from_random(100, 2);

    Virus<> v("covid");
    
    model.add_virus_n(v, 10);
    model.init(20, 20);

    Queue<> q;
    q.initialize(&model, nullptr);

    return 0;

}