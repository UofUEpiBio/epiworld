#ifndef GLOBALACTIONS_MEAT_HPP
#define GLOBALACTIONS_MEAT_HPP


// This function creates a global action that distributes a tool
// to agents with probability p.
/**
 * @brief Global action that distributes a tool to agents with probability p.
 * 
 * @tparam TSeq Sequence type (should match `TSeq` across the model)
 * @param p Probability of distributing the tool.
 * @param tool_fun Tool function.
 * @return std::function<void(Model<TSeq>*)> 
 */
template<typename TSeq>
inline std::function<void(Model<TSeq>*)> globalaction_tool(
    double p,
    ToolFun<TSeq> tool_fun
) {

    std::function<void(Model<TSeq>*)> fun = [p,tool_fun](
        Model<TSeq> * model
        ) -> void {

        for (auto & agent : model->agents)
        {

            // Check if the agent has the tool_fun
            if (agent.has_tool(tool_fun))
                continue;

            // Adding the tool
            if (model->runif() < p)
                agent.add_tool(tool_fun, model);
            
        
        }

        return;
            

    };

    return fun;

}

// Same function as above, but p is now a function of a vector of coefficients
// and a vector of variables.
/**
 * @brief Global action that distributes a tool to agents with probability
 * p = 1 / (1 + exp(-\sum_i coef_i * agent(vars_i))).
 * 
 * @tparam TSeq Sequence type (should match `TSeq` across the model)
 * @param coefs Vector of coefficients.
 * @param vars Vector of variables.
 * @param tool_fun Tool function.
 * @return std::function<void(Model<TSeq>*)> 
 */
template<typename TSeq>
inline std::function<void(Model<TSeq>*)> globalaction_tool_logit(
    std::vector< epiworld_double > coefs,
    std::vector< size_t > vars,
    ToolFun<TSeq> tool_fun
) {

    std::function<void(Model<TSeq>*)> fun = [coefs,vars,tool_fun](
        Model<TSeq> * model
        ) -> void {

        for (auto & agent : model->agents)
        {

            // Check if the agent has the tool_fun
            if (agent.has_tool(tool_fun))
                continue;

            // Computing the probability using a logit. Uses OpenMP reduction
            // to sum the coefficients.
            #pragma omp parallel for reduction(+:p)
            double p = 0.0;
            for (size_t i = 0u; i < coefs.size(); ++i)
                p += coefs.at(i) * agent(vars[i]);

            p = 1.0 / (1.0 + std::exp(-p));

            // Adding the tool
            if (model->runif() < p)
                agent.add_tool(tool_fun, model);
            
        
        }

        return;
            

    };

    return fun;

}

#endif