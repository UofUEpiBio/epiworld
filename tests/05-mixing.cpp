// #define EPI_DEBUG
#include "../include/epiworld/epiworld.hpp"

using namespace epiworld;

template<typename TSeq = int>
EntityToAgentFun<TSeq> dist_factory(int from, int to) {
    return [from, to](Entity<TSeq> & e, Model<TSeq> * m) -> void {

            auto & agents = m->get_agents();
            for (int i = from; i < to; ++i)
            {
                e.add_agent(&agents[i], m);
            }
            
            return;

        };
}

template<typename TSeq = int>
VirusToAgentFun<TSeq> dist_virus(int i)
{
    return [i](Virus<TSeq> & v, Model<TSeq> * m) -> void {

            m->get_agents()[i].set_virus(v, m);
            return;

        };

}

int main() {

    std::vector< double > contact_matrix = {
        1.0, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0
    };

    epimodels::ModelSEIRMixing<> model(
        "Flu", // std::string vname,
        10000, // epiworld_fast_uint n,
        0.01,// epiworld_double prevalence,
        40.0,// epiworld_double contact_rate,
        1.0,// epiworld_double transmission_rate,
        1.0,// epiworld_double avg_incubation_days,
        1.0/2.0,// epiworld_double recovery_rate,
        contact_matrix
    );

    // Copy the original virus
    Virus<> v1 = model.get_virus(0);
    model.rm_virus(0);

    model.add_virus_fun(v1, dist_virus<>(0));

    // Creating three groups
    Entity<> e1("Entity 1");
    Entity<> e2("Entity 2");
    Entity<> e3("Entity 3");

    model.add_entity_fun(e1, dist_factory<>(0, 3000));
    model.add_entity_fun(e2, dist_factory<>(3000, 6000));
    model.add_entity_fun(e3, dist_factory<>(6000, 10000));

    // Running and checking the results
    model.run(50, 123);
    model.print();

    // Getting all agents
    int n_right = 0;
    int n_wrong = 0;

    for (const auto & a : model.get_agents())
    {
        if (a.get_state() != epimodels::ModelSEIRMixing<int>::SUSCEPTIBLE)
        {
            if (a.get_entity(0).get_id() == 0)
            {
                n_right++;
                continue;
            }

            n_wrong++;
            
        }
            
    }

    // Reruning the model where individuals from group 0 transmit all to group 1
    contact_matrix[0] = 0.0;
    contact_matrix[6] = 1.0;
    contact_matrix[4] = 0.5;
    contact_matrix[1] = 0.5;
    model.set_contact_matrix(contact_matrix);

    // Running and checking the results
    model.run(50, 123);
    model.print();

    // Getting all agents
    n_right = 0;
    n_wrong = 0;

    for (const auto & a : model.get_agents())
    {

        if (a.get_id() == 0)
        {
            n_right++;
        } 
        else if (a.get_state() != epimodels::ModelSEIRMixing<int>::SUSCEPTIBLE)
        {
            if (a.get_entity(0).get_id() == 1)
            {
                n_right++;
                continue;
            }

            n_wrong++;
            
        }
            
    }

    return 0;

}
