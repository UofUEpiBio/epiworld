#ifndef CATCH_CONFIG_MAIN
#define EPI_DEBUG
#endif

#include "tests.hpp"

// Function to get current memory usage in kilobytes
size_t get_memory_usage() {
    std::ifstream proc_status("/proc/self/status");
    std::string line;
    size_t memory = 0;

    while (std::getline(proc_status, line)) {
        if (line.rfind("VmRSS:", 0) == 0) { // Resident Set Size
            std::istringstream iss(line);
            std::string key;
            iss >> key >> memory; // Extract value in KB
            break;
        }
    }

    return memory; // Returns KB
}

using namespace epiworld;

EPIWORLD_TEST_CASE("SEIRMixing", "[SEIR-mixing]") {

    size_t memory_before = get_memory_usage();

    // std::vector< double > contact_matrix = {
    //     1.0, 0.0, 0.0,
    //     0.0, 1.0, 0.0,
    //     0.0, 0.0, 1.0
    // };

    size_t n_groups = 40;
    std::vector< double > contact_matrix(
        n_groups * n_groups, 1.0 / 
        static_cast<double>(n_groups)
    );

    size_t n = 1000;
    epimodels::ModelSEIRMixing<> model(
        "Flu", // std::string vname,
        n, // epiworld_fast_uint n,
        0.01,  // epiworld_double prevalence,
        40.0,  // epiworld_double contact_rate,
        1.0,   // epiworld_double transmission_rate,
        1.0,   // epiworld_double avg_incubation_days,
        1.0/2.0,// epiworld_double recovery_rate,
        contact_matrix
    );

    // Copy the original virus
    Virus<> v1 = model.get_virus(0);
    model.rm_virus(0);
    v1.set_distribution(dist_virus<>(0));

    model.add_virus(v1);

    // Creating three groups
    for (size_t i = 0; i < n_groups; ++i)
    {

        // Distribution set
        std::vector< size_t > idx(n / n_groups);
        for (size_t j = 0; j < n / n_groups; ++j)
        {
            idx[j] = i * (n / n_groups) + j;
        }

        Entity<> e(
            "Entity " + std::to_string(i),
            distribute_entity_to_set<>(idx)
        );

        model.add_entity(e);
    }
    // Entity<> e1("Entity 1", dist_factory<>(0, 3000));
    // Entity<> e2("Entity 2", dist_factory<>(3000, 6000));
    // Entity<> e3("Entity 3", dist_factory<>(6000, n));

    // model.add_entity(e1);
    // model.add_entity(e2);
    // model.add_entity(e3);

    // Creating a tool
    Tool<> t1("Tool 1");


    // Selecting the first half individuals to get the tool
    std::vector< size_t > idx(n/2);
    for (size_t i = 0; i < n/2; ++i)
    {
        idx[i] = i;
    }
    t1.set_distribution(distribute_tool_to_set<>(idx));
    
    model.add_tool(t1);

    size_t nsims = 100;
    #ifdef EPI_DEBUG
    std::vector< double > group_sampling(n_groups * nsims, 0.0);
    auto saver = [&group_sampling, n_groups](size_t n, Model<>* m) -> void{

        auto sampling = dynamic_cast< epimodels::ModelSEIRMixing<>* >(m)->
            get_sampled_times_groups();

        // Saving the transition probabilities
        for (size_t i = 0; i < n_groups; ++i)
            for (size_t j = 0; j < n_groups; ++j)
                group_sampling[i * n_groups + j] += sampling[i * n_groups + j];

        return;
    };
    #else
    auto saver = [](size_t n, Model<>* m) -> void {
        return;
    };
    #endif

    // Running and checking the results
    // model.run(50, 123);
    model.run_multiple(50, nsims, 1233, saver, true, true, 4);

    // Measure memory after simulation
    size_t memory_after = get_memory_usage();
    std::cout << "Memory after: " << memory_after << " KB" << std::endl;
    std::cout << "Memory used: " << (memory_after - memory_before) << " KB" << std::endl;
   
    #ifdef EPI_DEBUG
    std::cout << "sizeof(int)    :" << sizeof(int) << std::endl;
    std::cout << "sizeof(size_t) :" << sizeof(size_t) << std::endl;
    std::cout << "sizeof(short)  :" << sizeof(double) << std::endl;
    std::cout << "sizeof(Agent)  :" << sizeof(model.get_agent(0)) << std::endl;
    std::cout << "sizeof(Virus)  :" << sizeof(model.get_virus(0)) << std::endl;
    std::cout << "sizeof(Tool)   :" << sizeof(model.get_tool(0)) << std::endl;
    std::cout << "sizeof(Entity) :" << sizeof(model.get_entity(0)) << std::endl;
    std::cout << "sizeof(Model)  :"  << sizeof(model) << std::endl;

    // auto v = std::make_shared< Virus<> >(model.get_virus(0));
    // std::cout << 
    //     "Address of model.virus_functions " << &model.get_virus(0).virus_functions << std::endl <<
    //     "Address of v.virus_functions     "     << &v->virus_functions << std::endl;
    #endif

    model.print();

    #ifndef CATCH_CONFIG_MAIN
    return 0;
    #endif

}
