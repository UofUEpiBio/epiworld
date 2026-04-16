#define EPI_DEBUG

#include "tests.hpp"
#ifdef __linux__
#include <sys/resource.h>
#endif

// Reads a memory metric from /proc/self/status in KB (Linux only).
size_t get_proc_status_memory_kb(const std::string & key) {
#ifdef __linux__
    std::ifstream proc_status("/proc/self/status");
    std::string line;
    size_t memory = 0;

    while (std::getline(proc_status, line)) {
        if (line.rfind(key, 0) == 0) {
            std::istringstream iss(line);
            std::string parsed_key;
            iss >> parsed_key >> memory;
            break;
        }
    }

    return memory;
#else
    return 0;
#endif /* __linux__ */
}

// Current resident memory (KB).
size_t get_memory_usage() {
    return get_proc_status_memory_kb("VmRSS:");
}

// Peak resident memory (KB).
size_t get_peak_memory_usage() {
#ifdef __linux__
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) == 0)
        return static_cast<size_t>(usage.ru_maxrss); // Linux reports KB.

    // Fallback path.
    return get_proc_status_memory_kb("VmHWM:");
#else
    return 0;
#endif /* __linux__ */
}

using namespace epiworld;

template<typename TSeq>
inline size_t agent_size_full(Agent<TSeq> & agent)
{
    auto baseline_size = sizeof(agent);
    baseline_size += agent.get_virus() ? sizeof(*agent.get_virus()) : 0;
    for (const auto & tool : agent.get_tools())
        baseline_size += sizeof(*tool);
    return baseline_size;
}

template<typename TSeq>
inline size_t agents_size_full(Model<TSeq> & model)
{
    size_t total_size = 0;
    for (size_t i = 0; i < model.size(); ++i)
        total_size += agent_size_full(model.get_agent(i));
    return total_size;
}

EPIWORLD_TEST_CASE("SEIRMixing", "[SEIR-mixing]") {

    size_t memory_before = get_memory_usage();
    size_t peak_before = get_peak_memory_usage();

    // std::vector< double > contact_matrix = {
    //     1.0, 0.0, 0.0,
    //     0.0, 1.0, 0.0,
    //     0.0, 0.0, 1.0
    // };

    size_t n_groups = 40;
    std::vector< double > contact_matrix(
        n_groups * n_groups, 40.0 / 
        static_cast<double>(n_groups)
    );

    size_t n = 10000;
    epimodels::ModelSEIRMixing<> model(
        "Flu", // std::string vname,
        n, // epiworld_fast_uint n,
        0.01,  // epiworld_double prevalence,
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

    // Creating a tool
    Tool<> t1("Tool 1");


    // Selecting the first half individuals to get the tool
    std::vector< size_t > idx(n/2);
    for (size_t i = 0; i < n/2; ++i)
    {
        idx[i] = i;
    }
    t1.set_distribution(distribute_tool_to_set<>(idx));
    t1.set_susceptibility_reduction(0.5);
    t1.set_recovery_enhancer(0.5);
    
    model.add_tool(t1);

    // Running and checking the results
    // model.run(50, 123);
    model.run_multiple(50, 10, 1233, nullptr, true, true, 4);
    size_t memory_after = get_memory_usage();
    size_t peak_after = get_peak_memory_usage();
    long long delta_current = static_cast<long long>(memory_after) - static_cast<long long>(memory_before);
    long long delta_peak = static_cast<long long>(peak_after) - static_cast<long long>(peak_before);

    model.print();

    // Measure memory after simulation
    std::cout << "-----------Overall-------------------\n";
    std::cout << "Memory before : " << memory_before << " KB" << std::endl;
    std::cout << "Memory after  : " << memory_after << " KB" << std::endl;
    std::cout << "Current delta : " << delta_current << " KB" << std::endl;
    std::cout << "Peak before   : " << peak_before << " KB" << std::endl;
    std::cout << "Peak after    : " << peak_after << " KB" << std::endl;
    std::cout << "Peak delta    : " << delta_peak << " KB" << std::endl;
    std::cout << "-------------------------------------\n";
   
    std::cout << "sizeof(int)         : " << sizeof(int) << std::endl;
    std::cout << "sizeof(size_t)      : " << sizeof(size_t) << std::endl;
    std::cout << "sizeof(double)      : " << sizeof(double) << std::endl;
    std::cout << "sizeof(Agent)       : " << sizeof(model.get_agent(0)) << " Bytes" << std::endl;
    std::cout << "sizeof(Agent) (all) : " <<
        sizeof(model.get_agent(0)) * model.size() / 1024.0 <<
        " KB" << std::endl;
    std::cout << "sizeof(Virus)       : " << sizeof(model.get_virus(0)) << std::endl;
    std::cout << "sizeof(Tool)        : " << sizeof(model.get_tool(0)) << std::endl;
    std::cout << "sizeof(Entity)      : " << sizeof(model.get_entity(0)) << std::endl;
    std::cout << "sizeof(Model)       : "  << sizeof(model) << std::endl;

    std::cout << "--------------- Functions ---------------\n";
    std::cout << "sizeof(ToolFun<TSeq>)        : " << sizeof(ToolFun<>) << std::endl;
    std::cout << "sizeof(ToolToAgentFun<TSeq>) : " << sizeof(ToolToAgentFun<>) << std::endl;
    std::cout << "sizeof(UpdateFun<TSeq>)      : " << sizeof(UpdateFun<>) << std::endl;


    std::cout << "--------------- Total agents size ---------------\n";
    std::cout << "Single agent size : " << agent_size_full(model.get_agent(0)) << " Bytes" << std::endl;
    std::cout << "Total agents size : " << agents_size_full(model) / 1024.0 << " KB" << std::endl;
    // auto v = std::make_shared< Virus<> >(model.get_virus(0));
    // std::cout << 
    //     "Address of model.virus_functions " << &model.get_virus(0).virus_functions << std::endl <<
    //     "Address of v.virus_functions     "     << &v->virus_functions << std::endl;




}
