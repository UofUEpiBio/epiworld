#define EPI_DEBUG_VIRUS
#include "tests.hpp"

using namespace epiworld;
EPIWORLD_TEST_CASE(
    "Virus set_state and get_state",
    "[virus-set-get-state]"
)
{
    
    // Print initial counter values
    std::cout << "=== Initial Virus Counter Values ===" << std::endl;
    std::cout << "Constructors called: " << Virus<>::counter_construct.load() << std::endl;
    std::cout << "Copy constructors: " << Virus<>::counter_copy_construct.load() << std::endl;
    std::cout << "Move constructors: " << Virus<>::counter_move_construct.load() << std::endl;
    std::cout << "Copy assignments: " << Virus<>::counter_copy_assign.load() << std::endl;
    std::cout << "Move assignments: " << Virus<>::counter_move_assign.load() << std::endl;
    std::cout << "Destructors called: " << Virus<>::counter_destruct.load() << std::endl;
    std::cout << "Total objects created: " << (Virus<>::counter_construct.load() + Virus<>::counter_copy_construct.load() + Virus<>::counter_move_construct.load()) << std::endl;
    std::cout << "Active instances: " << (Virus<>::counter_construct.load() + Virus<>::counter_copy_construct.load() + Virus<>::counter_move_construct.load() - Virus<>::counter_destruct.load()) << std::endl;
    std::cout << std::endl;
    
    epimodels::ModelSIRCONN<> model(
        "TestModel",                // std::string vname
        10,                        // epiworld_fast_uint n
        1.0/10.0,                       // epiworld_double prevalence
        4,                        // epiworld_double contact_rate
        0.5,                        // epiworld_double transmission_rate
        1.0/7.0                      // epiworld_double recovery_rate
    );
    
    // Print counter values after model creation
    std::cout << "=== After Model Creation ===" << std::endl;
    std::cout << "Constructors called: " << Virus<>::counter_construct.load() << std::endl;
    std::cout << "Copy constructors: " << Virus<>::counter_copy_construct.load() << std::endl;
    std::cout << "Move constructors: " << Virus<>::counter_move_construct.load() << std::endl;
    std::cout << "Copy assignments: " << Virus<>::counter_copy_assign.load() << std::endl;
    std::cout << "Move assignments: " << Virus<>::counter_move_assign.load() << std::endl;
    std::cout << "Destructors called: " << Virus<>::counter_destruct.load() << std::endl;
    std::cout << "Active instances: " << (Virus<>::counter_construct.load() + Virus<>::counter_copy_construct.load() + Virus<>::counter_move_construct.load() - Virus<>::counter_destruct.load()) << std::endl;
    std::cout << std::endl;
    
    model.run(10); // Run the model for 10 days

    // Print counter values after model run
    std::cout << "=== After Model Run ===" << std::endl;
    std::cout << "Constructors called: " << Virus<>::counter_construct.load() << std::endl;
    std::cout << "Copy constructors: " << Virus<>::counter_copy_construct.load() << std::endl;
    std::cout << "Move constructors: " << Virus<>::counter_move_construct.load() << std::endl;
    std::cout << "Copy assignments: " << Virus<>::counter_copy_assign.load() << std::endl;
    std::cout << "Move assignments: " << Virus<>::counter_move_assign.load() << std::endl;
    std::cout << "Destructors called: " << Virus<>::counter_destruct.load() << std::endl;
    std::cout << "Active instances: " << (Virus<>::counter_construct.load() + Virus<>::counter_copy_construct.load() + Virus<>::counter_move_construct.load() - Virus<>::counter_destruct.load()) << std::endl;
    std::cout << std::endl;

    model.print();

    // Print final counter values
    std::cout << "=== Final Virus Counter Values ===" << std::endl;
    std::cout << "Constructors called: " << Virus<>::counter_construct.load() << std::endl;
    std::cout << "Copy constructors: " << Virus<>::counter_copy_construct.load() << std::endl;
    std::cout << "Move constructors: " << Virus<>::counter_move_construct.load() << std::endl;
    std::cout << "Copy assignments: " << Virus<>::counter_copy_assign.load() << std::endl;
    std::cout << "Move assignments: " << Virus<>::counter_move_assign.load() << std::endl;
    std::cout << "Destructors called: " << Virus<>::counter_destruct.load() << std::endl;
    std::cout << "Total objects created: " << (Virus<>::counter_construct.load() + Virus<>::counter_copy_construct.load() + Virus<>::counter_move_construct.load()) << std::endl;
    std::cout << "Active instances: " << (Virus<>::counter_construct.load() + Virus<>::counter_copy_construct.load() + Virus<>::counter_move_construct.load() - Virus<>::counter_destruct.load()) << std::endl;

    return 0;
}