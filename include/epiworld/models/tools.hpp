#ifndef EPIWORLD_MODELS_TOOLS_H
#define EPIWORLD_MODELS_TOOLS_H

#include "../tool-bones.hpp"

/**
 * @brief Template for a Measles-Mumps-Rubella (MMR) tool
 * 
 * @ingroup tools
 * 
 */
template<typename TSeq = EPI_DEFAULT_TSEQ>
class ToolMMR: public Tool<TSeq> {
private:

    
static thread_local std::shared_ptr<std::vector<int>> immune;
static thread_local std::shared_ptr<std::vector<int>> model_id;

    // Reset immunity for all agents at the beginning of the model.
    void reset_immunity(Model<TSeq> * model);
    epiworld_double mmr_measles_immunity = 0.97;
    // Tool<TSeq>::tool_name = "MMR Vaccine";

public:

    void add_to_model(Model<TSeq> * m);
    ToolMMR(std::string name = "MMR Vaccine") : Tool<TSeq>(name) {};

    virtual epiworld_double get_susceptibility_reduction(VirusPtr<TSeq> v, Model<TSeq> * model) override;

    void set_efficacy(epiworld_double efficacy);

    std::unique_ptr<Tool<TSeq>> clone_ptr() const override;

};

template<typename TSeq>
thread_local std::shared_ptr<std::vector<int>> ToolMMR<TSeq>::immune = nullptr;

template<typename TSeq>
thread_local std::shared_ptr<std::vector<int>> ToolMMR<TSeq>::model_id = nullptr;

template<typename TSeq>
inline epiworld_double ToolMMR<TSeq>::get_susceptibility_reduction(VirusPtr<TSeq> v, Model<TSeq> * model)
{

    // Have we initialized the tool?
    if (model_id == nullptr)
    {
        model_id = std::make_shared<std::vector<int>>(model->size(), -99);
        immune = std::make_shared<std::vector<int>>(model->size(), 0);
    }

    // Agent-level information
    auto & model_id_i = (*model_id)[this->get_agent()->get_id()];
    auto & immune_i = (*immune)[this->get_agent()->get_id()];

    // Updating a single agent (if needed)
    if (model_id_i != static_cast<int>(model->get_sim_id()))
    {
        model_id_i = static_cast<int>(model->get_sim_id());
        immune_i = (model->runif() < mmr_measles_immunity) ? 1 : 0;
    }

    return  (immune_i == 1) ? 1.0 : 0.0;

}

template<typename TSeq>
inline void ToolMMR<TSeq>::set_efficacy(epiworld_double efficacy)
{
    this->mmr_measles_immunity = efficacy;
}

template<typename TSeq>
inline std::unique_ptr<Tool<TSeq>> ToolMMR<TSeq>::clone_ptr() const
{
    return std::make_unique<ToolMMR<TSeq>>(*this);
}


#endif