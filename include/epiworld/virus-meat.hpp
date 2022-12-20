#ifndef EPIWORLD_VIRUS_MEAT_HPP
#define EPIWORLD_VIRUS_MEAT_HPP

template<typename TSeq>
inline Virus<TSeq>::Virus(std::string name) {
    set_name(name);
}

// template<typename TSeq>
// inline Virus<TSeq>::Virus(TSeq sequence, std::string name) {
//     baseline_sequence = std::make_shared<TSeq>(sequence);
//     set_name(name);
// }

template<typename TSeq>
inline void Virus<TSeq>::mutate(
    Model<TSeq> * model
) {

    if (mutation_fun)
        if (mutation_fun(agent, *this, model))
            model->get_db().record_variant(*this);

    return;
    
}

template<typename TSeq>
inline void Virus<TSeq>::set_mutation(
    MutFun<TSeq> fun
) {
    mutation_fun = MutFun<TSeq>(fun);
}

template<typename TSeq>
inline const TSeq * Virus<TSeq>::get_sequence() {
    return &(*baseline_sequence);
}

template<typename TSeq>
inline void Virus<TSeq>::set_sequence(TSeq sequence) {
    baseline_sequence = std::make_shared<TSeq>(sequence);
    return;
}

template<typename TSeq>
inline Agent<TSeq> * Virus<TSeq>::get_agent() {
    return agent;
}

template<typename TSeq>
inline void Virus<TSeq>::set_agent(Agent<TSeq> * p, epiworld_fast_uint idx) {
    agent = p;
    agent_idx = static_cast<int>(idx);
}

template<typename TSeq>
inline void Virus<TSeq>::set_id(int idx) {
    id = idx;
    return;
}

template<typename TSeq>
inline int Virus<TSeq>::get_id() const {
    
    return id;
}

template<typename TSeq>
inline void Virus<TSeq>::set_date(int d) {
    date = d;
    return;
}

template<typename TSeq>
inline int Virus<TSeq>::get_date() const {
    
    return date;
}

template<typename TSeq>
inline epiworld_double Virus<TSeq>::get_prob_infecting(
    Model<TSeq> * model
)
{

    if (probability_of_infecting_fun)
        return probability_of_infecting_fun(agent, *this, model);
        
    return EPI_DEFAULT_VIRUS_PROB_INFECTION;

}



template<typename TSeq>
inline epiworld_double Virus<TSeq>::get_prob_recovery(
    Model<TSeq> * model
)
{

    if (probability_of_recovery_fun)
        return probability_of_recovery_fun(agent, *this, model);
        
    return EPI_DEFAULT_VIRUS_PROB_RECOVERY;

}



template<typename TSeq>
inline epiworld_double Virus<TSeq>::get_prob_death(
    Model<TSeq> * model
)
{

    if (probability_of_death_fun)
        return probability_of_death_fun(agent, *this, model);
        
    return EPI_DEFAULT_VIRUS_PROB_DEATH;

}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_infecting_fun(VirusFun<TSeq> fun)
{
    probability_of_infecting_fun = fun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_recovery_fun(VirusFun<TSeq> fun)
{
    probability_of_recovery_fun = fun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_death_fun(VirusFun<TSeq> fun)
{
    probability_of_death_fun = fun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_infecting(epiworld_double * prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Agent<TSeq> *, Virus<TSeq> &, Model<TSeq> *)
        {
            return *prob;
        };
    
    probability_of_infecting_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_recovery(epiworld_double * prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Agent<TSeq> *, Virus<TSeq> &, Model<TSeq> *)
        {
            return *prob;
        };
    
    probability_of_recovery_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_death(epiworld_double * prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Agent<TSeq> *, Virus<TSeq> &, Model<TSeq> *)
        {
            return *prob;
        };
    
    probability_of_death_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_infecting(epiworld_double prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Agent<TSeq> *, Virus<TSeq> &, Model<TSeq> *)
        {
            return prob;
        };
    
    probability_of_infecting_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_recovery(epiworld_double prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Agent<TSeq> *, Virus<TSeq> &, Model<TSeq> *)
        {
            return prob;
        };
    
    probability_of_recovery_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_death(epiworld_double prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Agent<TSeq> *, Virus<TSeq> &, Model<TSeq> *)
        {
            return prob;
        };
    
    probability_of_death_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_post_recovery(PostRecoveryFun<TSeq> fun)
{
    if (post_recovery_fun)
    {
        printf_epiworld(
            "Warning: a PostRecoveryFun is alreay in place (overwriting)."
            );
    }

    post_recovery_fun = fun;
}

template<typename TSeq>
inline void Virus<TSeq>::post_recovery(
    Model<TSeq> * model
)
{

    if (post_recovery_fun)
        post_recovery_fun(agent, *this, model);    

    return;
        
}

template<typename TSeq>
inline void Virus<TSeq>::set_post_immunity(
    epiworld_double prob
)
{

    if (post_recovery_fun)
    {

        std::string msg =
            std::string(
                "You cannot set post immunity when a post_recovery "
                ) +
            std::string(
                "function is already in place. Redesign the post_recovery function."
                );

        throw std::logic_error(msg);
        
    }

    // To make sure that we keep registering the virus
    ToolPtr<TSeq> __no_reinfect = std::make_shared<Tool<TSeq>>(
        "Immunity (" + *virus_name + ")"
    );

    __no_reinfect->set_susceptibility_reduction(prob);
    __no_reinfect->set_death_reduction(0.0);
    __no_reinfect->set_transmission_reduction(0.0);
    __no_reinfect->set_recovery_enhancer(0.0);

    PostRecoveryFun<TSeq> tmpfun = 
        [__no_reinfect](
            Agent<TSeq> * p, Virus<TSeq> &, Model<TSeq> * m
            )
        {
            
            // Have we registered the tool?
            if (__no_reinfect->get_id() == -99)
                m->get_db().record_tool(*__no_reinfect);

            p->add_tool(__no_reinfect, m);

            return;

        };

    post_recovery_fun = tmpfun;

}

template<typename TSeq>
inline void Virus<TSeq>::set_post_immunity(
    epiworld_double * prob
)
{

    if (post_recovery_fun)
    {

        std::string msg =
            std::string(
                "You cannot set post immunity when a post_recovery "
                ) +
            std::string(
                "function is already in place. Redesign the post_recovery function."
                );

        throw std::logic_error(msg);

    }

    // To make sure that we keep registering the virus
    ToolPtr<TSeq> __no_reinfect = std::make_shared<Tool<TSeq>>(
        "Immunity (" + *virus_name + ")"
    );

    __no_reinfect->set_susceptibility_reduction(prob);
    __no_reinfect->set_death_reduction(0.0);
    __no_reinfect->set_transmission_reduction(0.0);
    __no_reinfect->set_recovery_enhancer(0.0);

    PostRecoveryFun<TSeq> tmpfun = 
        [__no_reinfect](Agent<TSeq> * p, Virus<TSeq> &, Model<TSeq> * m)
        {

            // Have we registered the tool?
            if (__no_reinfect->get_id() == -99)
                m->get_db().record_tool(*__no_reinfect);

            p->add_tool(__no_reinfect, m);

            return;

        };

    post_recovery_fun = tmpfun;

}

template<typename TSeq>
inline void Virus<TSeq>::set_name(std::string name)
{

    if (name == "")
        virus_name = nullptr;
    else
        virus_name = std::make_shared<std::string>(name);

}

template<typename TSeq>
inline std::string Virus<TSeq>::get_name() const
{

    if (virus_name)
        return *virus_name;
    
    return "unknown virus";

}

template<typename TSeq>
inline std::vector< epiworld_double > & Virus<TSeq>::get_data() {
    return data;
}

template<typename TSeq>
inline void Virus<TSeq>::set_status(
    epiworld_fast_int init,
    epiworld_fast_int end,
    epiworld_fast_int removed
)
{
    status_init    = init;
    status_post    = end;
    status_removed = removed;
}

template<typename TSeq>
inline void Virus<TSeq>::set_queue(
    epiworld_fast_int init,
    epiworld_fast_int end,
    epiworld_fast_int removed
)
{

    queue_init    = init;
    queue_post     = end;
    queue_removed = removed;

}

template<typename TSeq>
inline void Virus<TSeq>::get_status(
    epiworld_fast_int * init,
    epiworld_fast_int * end,
    epiworld_fast_int * removed
)
{

    if (init != nullptr)
        *init = status_init;

    if (end != nullptr)
        *end = status_post;

    if (removed != nullptr)
        *removed = status_removed;

}

template<typename TSeq>
inline void Virus<TSeq>::get_queue(
    epiworld_fast_int * init,
    epiworld_fast_int * end,
    epiworld_fast_int * removed
)
{

    if (init != nullptr)
        *init = queue_init;

    if (end != nullptr)
        *end = queue_post;

    if (removed != nullptr)
        *removed = queue_removed;
        
}

#endif
