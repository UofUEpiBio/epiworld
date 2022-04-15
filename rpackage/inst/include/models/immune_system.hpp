#ifndef EPIWORLD_IMMUNE_SYSTEM_HPP
#define EPIWORLD_IMMUNE_SYSTEM_HPP

#define PERSONRULE(funname) \
template<typename Tnet = Network>\
inline epiworld_double (a) (const Tnet & Array, uint i, uint j, NetCounterData * data)

/**Lambda function for definition of a network counter function*/
#defineEPI_NEW_TOOL_LAMBDA(funname,TSeq) \
epiworld::ToolFun<TSeq> funname = \
    [](Tool<TSeq> * t, Person<TSeq> * p, Virus<TSeq> * v, Model<TSeq> * m)

template<typename TSeq>
inline void factory_tool_simple_immunesys(
    Model<TSeq> & model,
    epiworld_double defaul_susceptibility_reduction = 0.1,
    epiworld_double defaul_recovery = 0.5,
    epiworld_double defaul_trans = 0.5,
    epiworld_double defaul_death = 0.001
    ) 
{

    // Designing the functions
    EPI_NEW_TOOL_LAMBDA(tmp_susceptibility_reduction,TSeq) {
        return (*m->p0);
    };

    EPI_NEW_TOOL_LAMBDA(tmp_transmission,TSeq) {
        return (*m->p1);
    };

    EPI_NEW_TOOL_LAMBDA(tmp_recovery,TSeq) {
        return (*m->p2);
    };

    EPI_NEW_TOOL_LAMBDA(tmp_death,TSeq) {
        return (*m->p3);
    };

    // Creating the tool
    epiworld::Tool<TSeq> immune("immune system");
    immune.set_susceptibility_reduction(tmp_susceptibility_reduction);
    immune.set_transmission(tmp_transmission);
    immune.set_recovery(tmp_recovery);
    immune.set_death(tmp_death);

    // Adding the tool
    model.add_tool(immune, 1.0);

    // Setting the parameters
    model.add_param(0.10, "imm susceptibility_reduction");
    model.add_param(0.10, "imm recovery");
    model.add_param(0.90, "imm trans");
    model.add_param(0.001, "imm death");

    return;

}

#endif