#ifndef EPIWORLD_PERSON_MEAT_HPP
#define EPIWORLD_PERSON_MEAT_HPP

template<typename TSeq>
inline Person<TSeq>::Person()
{
    
}

template<typename TSeq>
inline void Person<TSeq>::init()
{
    tools.person = this;
    viruses.host = this;
    status       = HEALTHY;
}
    

template<typename TSeq>
inline void Person<TSeq>::add_tool(
    int d,
    Tool<TSeq> tool
) {
    tools.add_tool(d, tool);
}

template<typename TSeq>
inline void Person<TSeq>::add_virus(
    int d,
    Virus<TSeq> virus
) {
    viruses.add_virus(d, virus);
    status_next = INFECTED;
}

template<typename TSeq>
inline double Person<TSeq>::get_efficacy(
    Virus<TSeq> * v
) {
    return tools.get_efficacy(v);
}

template<typename TSeq>
inline double Person<TSeq>::get_transmisibility(
    Virus<TSeq> * v
) {
    return tools.get_transmisibility(v);
}

template<typename TSeq>
inline double Person<TSeq>::get_recovery(
    Virus<TSeq> * v
) {
    return tools.get_recovery(v);
}

template<typename TSeq>
inline double Person<TSeq>::get_death(
    Virus<TSeq> * v
) {
    return tools.get_death(v);
}

template<typename TSeq>
inline int Person<TSeq>::get_id() const
{
    return id;
}

template<typename TSeq>
inline std::mt19937 * Person<TSeq>::get_rand_endgine() {
    return model->get_rand_endgine();
}

template<typename TSeq>
inline Model<TSeq> * Person<TSeq>::get_model() {
    return model;
}

template<typename TSeq>
inline PersonViruses<TSeq> & Person<TSeq>::get_viruses() {
    return viruses;
}

template<typename TSeq>
inline Virus<TSeq> & Person<TSeq>::get_virus(int i) {
    return viruses(i);
}

template<typename TSeq>
inline PersonTools<TSeq> & Person<TSeq>::get_tools() {
    return tools;
}

template<typename TSeq>
inline Tool<TSeq> & Person<TSeq>::get_tool(int i) {
    return tools(i);
}

template<typename TSeq>
inline void Person<TSeq>::mutate_variant() {
    viruses.mutate();
}

template<typename TSeq>
inline void Person<TSeq>::add_neighbor(
    Person<TSeq> * p,
    bool check_source,
    bool check_target
) {
    // Can we find the neighbor?
    if (check_source)
    {

        bool found = false;
        for (auto & n: neighbors)    
            if (n->get_id() == p->get_id())
            {
                found = true;
                break;
            }

        if (!found)
            neighbors.push_back(p);

    } else 
        neighbors.push_back(p);

    if (check_target)
    {

        bool found = false;
        for (auto & n: p->neighbors)
            if (n->get_id() == id)
            {
                found = true;
                break;
            }

        if (!found)
            p->neighbors.push_back(this);
    
    } else 
        p->neighbors.push_back(this);
    

}

template<typename TSeq>
inline std::vector< Person<TSeq> *> & Person<TSeq>::get_neighbors()
{
    return neighbors;
}

template<typename TSeq>
inline void Person<TSeq>::update_status() {

    // No change if deceased
    if (status == STATES::DECEASED)
        return;

    if ((status == STATES::HEALTHY) | (status == STATES::RECOVERED))
    {
        // Step 1: Compute the individual efficcacy
        std::vector< double > probs;
        std::vector< double > probs_only_v;
        std::vector< Virus<TSeq>* > variants;
        std::vector< int > certain_infection; ///< Viruses for which there's no chance of escape

        // Computing the efficacy
        double p_none = 1.0; ///< Product of all (1 - efficacy)
        double tmp_efficacy;
        for (unsigned int n = 0; n < neighbors.size(); ++n)
        {

            Person<TSeq> * neighbor = neighbors[n];

            // Non-infected individuals make no difference
            if ((neighbor->get_status() != STATES::INFECTED) & (neighbor->get_status() != STATES::ASYMPTOMATIC))
                continue;

            PersonViruses<TSeq> & nviruses = neighbor->get_viruses();
            
            // Now over the neighbor's viruses
            for (unsigned int v = 0; v < nviruses.size(); ++v)
            {

                // Computing the corresponding efficacy
                Virus<TSeq> * tmp_v = &(nviruses(v));

                // And it is a function of transmisibility as well
                tmp_efficacy = std::max(
                    get_efficacy(tmp_v),
                    (1.0 - neighbor->get_transmisibility(tmp_v))
                    );
                
                probs.push_back(1.0 - tmp_efficacy);
                variants.push_back(tmp_v);

                // Adding to the product
                p_none *= tmp_efficacy;

                // Is it impossible to escape?
                if (tmp_efficacy < 1e-100)
                    certain_infection.push_back(probs.size() - 1);

            }
            
        }

        // No virus to compute on
        if (probs.size() == 0)
            return;

        // Case in which infection is certain. All certain infections have
        // equal chance of taking the individual
        double r = model->runif();
        if (certain_infection.size() > 0)
        {
            int ord = certain_infection[std::floor(r * certain_infection.size())];
            
            add_virus(model->today(), *variants[ord]);

            get_model()->get_db().record_transmision(
                this->id,
                variants[ord]->get_host()->get_id(),
                variants[ord]->get_id()
                );

            return;

        }

        // Step 2: Calculating the prob of none or single
        double p_none_or_single = p_none;
        for (unsigned int v = 0; v < probs.size(); ++v)
        {
            probs_only_v.push_back(probs[v] * (p_none / (1.0 - probs[v])));
            p_none_or_single += probs_only_v[v];
        }

        
        // Step 3: Roulette
        double cumsum = p_none/p_none_or_single;
        if (r < cumsum)
            return;

        for (unsigned int v = 0; v < probs.size(); ++v)
        {
            // If it yield here, then bingo, the individual will acquire the disease
            cumsum += probs_only_v[v]/(p_none_or_single);
            if (r < cumsum)
            {
                add_virus(model->today(), *variants[v]);

                get_model()->get_db().record_transmision(
                    this->id,
                    variants[v]->get_host()->get_id(),
                    variants[v]->get_id()
                );

                return;
            }
            
        }

    } else if (status == STATES::INFECTED)
    {

        Virus<TSeq> * vptr = &viruses(0u);

        double p_die = get_death(vptr);
        double p_rec = get_recovery(vptr);
        double r = model->runif();

        double cumsum = p_die * (1 - p_rec) / (1.0 - p_die * p_rec); 

        if (r < cumsum)
        {
            
            model->get_db().up_deceased(vptr);
            status_next = DECEASED;
            return;

        } 
        
        cumsum += p_rec * (1 - p_die) / (1.0 - p_die * p_rec);
        if (r < cumsum)
        {
            // Updating db and running actions
            model->get_db().up_recovered(vptr);

            status_next = RECOVERED;

            // Checking if something happens after recovery
            // (e.g., full immunity)
            vptr->post_recovery();

            viruses.reset();
            
            return;

        }

    } 

    return;

}

template<typename TSeq>
inline int Person<TSeq>::get_status() const {
    return status;
}

template<typename TSeq>
inline void Person<TSeq>::reset() {

    this->viruses.reset();
    this->tools.reset();
    this->status      = STATES::HEALTHY;
    this->status_next = STATES::HEALTHY;

}

#endif