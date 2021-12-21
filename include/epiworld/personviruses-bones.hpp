#ifndef EPIWORLD_PERSONVIRUSES_BONES_HPP
#define EPIWORLD_PERSONVIRUSES_BONES_HPP

/**
 * @brief Set of viruses in host
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class PersonViruses {
    friend class Person<TSeq>;

private:
    Person<TSeq> * host;
    std::vector< Virus<TSeq> > viruses;
    int nactive = 0;

public:
    void add_virus(int date, Virus<TSeq> v);
    size_t size() const;
    int size_active() const;
    Virus<TSeq> & operator()(int i);
    void mutate();
    void clear();
    void deactivate(Virus<TSeq> & v);

};


#endif