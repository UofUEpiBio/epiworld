#ifndef EPIWORLD_IMMUNE_SYSTEM_HPP
#define EPIWORLD_IMMUNE_SYSTEM_HPP

#include "../epiworld.hpp"

#define PERSONRULE(funname) \
template<typename Tnet = Network>\
inline double (a) (const Tnet & Array, uint i, uint j, NetCounterData * data)

/**Lambda function for definition of a network counter function*/
#define TOOL_LAMBDA(funname) \
epiworld::ToolFun<TSeq> funname = \
    [](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)>

template<typename TSeq>
inline void factory_tool_simple_immunesys(
    Model<TSeq> & model,
    double defaul_efficacy = 0.1,
    double defaul_recovery = 0.5,
    double defaul_trans = 0.5,
    double defaul_death = 0.001
    ) 
{

    // Designing the functions
    TOOL_LAMBDA(tmp_efficacy) {
        return (*t->p00);
    }

    TOOL_LAMBDA(tmp_transmission) {
        return (*t->p01);
    }

    TOOL_LAMBDA(tmp_recovery) {
        return (*t->p02);
    }

    TOOL_LAMBDA(tmp_death) {
        return (*t->p03);
    }

    // Creating the tool
    epiworld::Tool<TSeq> immune("immune system");
    immune.set_efficacy(tmp_efficacy);
    immune.set_transmission(tmp_transmission);
    immune.set_recovery(tmp_recovery);
    immune.set_death(tmp_death);

    // Setting the parameters
    immune.add_param(0.10, "imm efficacy", model);
    immune.add_param(0.10, "imm recovery", model);
    immune.add_param(0.90, "imm trans", model);
    immune.add_param(0.001, "imm death", model);

    // Adding the tool
    model.add_tool(immune, 1.0);

    return;

}

#endif