#ifndef EPIWORLD_PERSON_HPP
#define EPIWORLD_PERSON_HPP

#define HEALTHY  0
#define DECEASED -99
#define INFECTED -1
#define RECOVERED 1

template<typename TSeq>
class Virus;

template<typename TSeq>
class Tool;

template<typename TSeq>
class PersonViruses;

template<typename TSeq>
class PersonTools;

template<typename TSeq>
class Person {
    friend class Model<TSeq>;
    friend class Tool<TSeq>;

private:
    Model<TSeq> * model;
    PersonViruses<TSeq> viruses;
    PersonTools<TSeq> tools;
    std::vector< Person<TSeq> * > neighbors;
    int status = HEALTHY;

public:

    Person();

    void add_tool(int d, Tool<TSeq> tool);
    void add_virus(int d, Virus<TSeq> virus);

    double get_efficacy(Virus<TSeq> * v);
    double get_transmisibility(Virus<TSeq> * v);
    double get_recovery(Virus<TSeq> * v);
    double get_death(Virus<TSeq> * v);
    
    std::mt19937 * get_rand_endgine();
    Model<TSeq> * get_model(); 

    Virus<TSeq> & get_virus(int i);
    PersonViruses<TSeq> & get_viruses();

    void mutate_virus();
    void add_neighbor(Person<TSeq> * p);

    void update_status();
    int get_status() const;

};

template<typename TSeq>
inline Person<TSeq>::Person() {
    tools.person   = this;
    viruses.person = this;
    status         = HEALTHY;
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
    status = INFECTED;
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
inline void Person<TSeq>::mutate_virus() {
    viruses.mutate();
}

template<typename TSeq>
inline void Person<TSeq>::add_neighbor(
    Person<TSeq> * p
) {
    neighbors.push_back(p);
    p->neighbors.push_back(this);
}


template<typename TSeq>
inline void Person<TSeq>::update_status() {

    // No change if deceased
    if (status == DECEASED)
        return;

    if ((status == HEALTHY) | (status == RECOVERED))
    {
        // Step 1: Compute the individual efficcacy
        std::vector< double > probs;
        std::vector< double > probs_only_v;
        std::vector< Virus<TSeq>* > variants;
        std::vector< int > certain_infection; ///< Viruses for which there's no chance of escape

        // Computing the efficacy
        double p_none = 1.0; ///< Product of all (1 - efficacy)
        double tmp_efficacy;
        for (int n = 0; n < neighbors.size(); ++n)
        {

            Person<TSeq> * neighbor = neighbors[n];

            // Non-infected individuals make no difference
            if (neighbor->get_status() != INFECTED)
                continue;

            PersonViruses<TSeq> nviruses = neighbor->get_viruses();
            
            // Now over the neighbor's viruses
            for (int v = 0; v < nviruses.size(); ++v)
            {

                // Computing the corresponding efficacy
                Virus<TSeq> * tmp_v = &(neighbor->get_virus(v));

                // And it is a function of transmisibility as well
                tmp_efficacy = get_efficacy(tmp_v) * neighbor->get_transmisibility(tmp_v);
                
                probs.push_back(1.0 - tmp_efficacy);
                variants.push_back(tmp_v);

                // Adding to the product
                p_none *= tmp_efficacy;

                // Is it impossible to escape?
                if (tmp_efficacy < 1e-100)
                    certain_infection.push_back(probs.size() - 1);


            }
        }

        // Case in which infection is certain. All certain infections have
        // equal chance of taking the individual
        double r = model->runif();
        if (certain_infection.size() > 0)
        {
            add_virus(model->today(), *variants[
                certain_infection[std::floor(r * certain_infection.size())]
            ]);

            // And we go back
            status = INFECTED;
            return;
        }

        // Step 2: Calculating the prob of none or single
        double p_none_or_single = p_none;
        for (int v = 0; v < probs.size(); ++v)
        {
            probs_only_v.push_back(probs[v] * (p_none / (1.0 - probs[v])));
            p_none_or_single += probs_only_v[v];
        }

        
        // Step 3: Roulette
        double cumsum = p_none/p_none_or_single;

        for (int v = 0; v < probs.size(); ++v)
        {
            // If it yield here, then bingo, the individual will acquire the disease
            cumsum += probs_only_v[v]/(p_none_or_single);
            if (r < cumsum)
            {
                add_virus(model->today(), *variants[v]);
                model->get_db().up_infected(variants[v]);
                status = INFECTED;
                return;
            }
            
        }

        // // We shouldn't have reached this place
        // throw std::logic_error(
        //     "Your calculations are wrong. This should't happen:\ncumsum : " +
        //     std::to_string(cumsum) + "\nr      : " + std::to_string(r) + "\n"
        //     );

    } else if (status == INFECTED)
    {

        Virus<TSeq> * vptr = &viruses(0u);

        // Since individuals can either recover, die, or stay (all exclusive)
        // we compute conditional probabilities
        // P(die | die or recover or neither) =
        //    P(die or recover or neither | die) * P(die) / P(die or recover or neither)
        //    = P(die) / P(die or recover or neither)
        // P(die or recover or neither) = 1 - P(die) * P(recover)
        // thus
        // P(die | die or recover or neither) = P(die) / (1 - P(die) * P(recover))
        double p_die = get_death(vptr);
        double p_rec = get_recovery(vptr);
        double r = model->runif();

        double cumsum = p_die / (1.0 - p_die * p_rec); 

        if (r < cumsum)
        {
            model->get_db().up_deceased(vptr);
            status = DECEASED;
            return;
        } 
        
        cumsum += p_rec / (1.0 - p_die * p_rec);
        if (r < cumsum)
        {
            model->get_db().up_recovered(vptr);
            status = RECOVERED;
            viruses.clear();
            return;
        }

        // If continues sick, then mutate the virus
        mutate_virus();

    } 

    return;

}

template<typename TSeq>
inline int Person<TSeq>::get_status() const {
    return status;
}

#undef HEALTHY
#undef DECEASED
#undef INFECTED
#undef RECOVERED

#endif