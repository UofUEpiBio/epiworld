#ifndef MEASLES_IMMUNOGLOBULIN_BONES_HPP
#define MEASLES_IMMUNOGLOBULIN_BONES_HPP

using namespace epiworld;

#include <memory>
#include <vector>
#include <cassert>
#include <string_view>

/**
 * @brief Template for a common Immunoglobulin tool.
 * 
 * The main difference with a regular tool is that the reduction
 * in susceptibility is at the agent level and fixed for the
 * entire simulation. In other words, if the efficacy is 65%,
 * then 65% of the agents that receive the vaccine will be fully
 * protected (100% reduction in susceptibility) and 35% will not be
 * protected at all (0% reduction in susceptibility).
 * 
 * @ingroup tools
 * 
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ToolImmunoglobulin: public Tool<TSeq> {
private:

    int _immune = -1;
    std::string _par_efficacy;
    epiworld_double _set_immunity(Model<TSeq> & model);

    // This half-life
    std::string _par_half_life_mean;
    std::string _par_half_life_sd;
    int _removal_time = -1;
    bool _remove_tool();

    // This method is used to throw an error when trying to set a method
    // that cannot be set for this tool.
    void _error_method(std::string_view method_name) const;

public:
    ToolImmunoglobulin(
        std::string name,
        std::string par_efficacy,
        std::string par_half_life_mean,
        std::string par_half_life_sd
    ) : Tool<TSeq>(name) {
        this->_par_efficacy = par_efficacy;
        this->_par_half_life_mean = par_half_life_mean;
        this->_par_half_life_sd = par_half_life_sd;
    };

    virtual epiworld_double get_susceptibility_reduction(
        VirusPtr<TSeq> & v,
        Model<TSeq> * model
    ) override;
    
    virtual void set_susceptibility_reduction_fun(ToolFun<TSeq> fun) override;
    virtual void set_susceptibility_reduction(std::string param) override;
    virtual void set_susceptibility_reduction(epiworld_double prob) override;

    std::unique_ptr<Tool<TSeq>> clone_ptr() const override;

};

#endif