#ifndef MEASLES_IMMUNOGLOBULIN_MEAT_HPP
#define MEASLES_IMMUNOGLOBULIN_MEAT_HPP

using namespace epiworld;
#include "immunoglobulin-bones.hpp"

#include <cassert>

template<typename TSeq>
inline void ToolImmunoglobulin<TSeq>::_error_method(std::string_view method_name) const {
    throw std::logic_error(
        std::string(
            "The method " + std::string(method_name) + " cannot be set for the "
        ) +
        std::string("ToolImmunoglobulin (Tool).")
    );
}

template<typename TSeq>
inline epiworld_double ToolImmunoglobulin<TSeq>::get_susceptibility_reduction(
    VirusPtr<TSeq> &,
    Model<TSeq> * model
)
{

    // Updating a single agent (if needed)
    if (_immune == -1)
    {
        _set_immunity(*model);
    }

    // Deciding if the tool should be removed
    #ifdef EPI_DEBUG
    EPI_DEBUG_PRINTF(
        "Agent %d; sim_id %li; today %d; removal_time %d\n",
        this->agent->get_id(), model->get_sim_id(),
        model->today(), _removal_time
    );
    #endif
    if (model->today() >= _removal_time)
    {
        this->get_agent()->rm_tool(*model, this->pos_in_agent);
        return 0.0;
    }

    return  (_immune == 1) ? 1.0 : 0.0;

}

template<typename TSeq>
inline void ToolImmunoglobulin<TSeq>::set_susceptibility_reduction_fun(
    ToolFun<TSeq>
)
{
    _error_method("set_susceptibility_reduction_fun");
}

template<typename TSeq>
inline void ToolImmunoglobulin<TSeq>::set_susceptibility_reduction(std::string)
{
    _error_method("set_susceptibility_reduction(string)");
}

template<typename TSeq>
inline void ToolImmunoglobulin<TSeq>::set_susceptibility_reduction(epiworld_double)
{

    _error_method("set_susceptibility_reduction(epiworld_double)");

}

template<typename TSeq>
inline epiworld_double ToolImmunoglobulin<TSeq>::_set_immunity(Model<TSeq> & model)
{

    // Setting immunity
    _immune = (model.runif() < model.par(this->_par_efficacy)) ? 1 : 0;

    // Setting the removal time (regardless)
    auto half_life = model.par(this->_par_half_life_mean);
    if (half_life > 0.0)
    {

        // Drawing the half-life for this agent
        auto hl_sd = model.par(this->_par_half_life_sd);
        auto hl_mean = model.par(this->_par_half_life_mean);

        if (hl_sd > 0.0)
            half_life = model.rnorm(hl_mean, hl_sd);
        else
            half_life = hl_mean;

        _removal_time = model.today() + static_cast<int>(std::round(half_life));
    }

    return _immune;
}

template<typename TSeq>
inline std::unique_ptr<Tool<TSeq>> ToolImmunoglobulin<TSeq>::clone_ptr() const
{
    auto ans =  std::make_unique<ToolImmunoglobulin<TSeq>>(*this);
    ans->_immune = -1;
    return ans;
}

#endif
