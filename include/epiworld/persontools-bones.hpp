#ifndef EPIWORLD_PERSONTOOLS_BONES_HPP
#define EPIWORLD_PERSONTOOLS_BONES_HPP

/**
 * @brief List of tools available for the individual to 
 * 
 * @tparam TSeq 
 */
template<typename TSeq = bool>
class PersonTools {
    friend class Person<TSeq>;
    friend class Model<TSeq>;

private:
    Person<TSeq> * person; 
    std::vector<Tool<TSeq>> tools;
    std::vector< int > dates;
    MixerFun<TSeq> susceptibility_reduction_mixer;
    MixerFun<TSeq> transmission_reduction_mixer;
    MixerFun<TSeq> recovery_enhancer_mixer;
    MixerFun<TSeq> death_reduction_mixer;

public:
    PersonTools() {};
    void add_tool(int date, Tool<TSeq> tool);
    epiworld_double get_susceptibility_reduction(Virus<TSeq> * v);
    epiworld_double get_transmission_reduction(Virus<TSeq> * v);
    epiworld_double get_recovery_enhancer(Virus<TSeq> * v);
    epiworld_double get_death_reduction(Virus<TSeq> * v);

    void set_susceptibility_reduction_mixer(MixerFun<TSeq> fun);
    void set_transmission_reduction_mixer(MixerFun<TSeq> fun);
    void set_recovery_enhancer_mixer(MixerFun<TSeq> fun);
    void set_death_reduction_mixer(MixerFun<TSeq> fun);

    size_t size() const;
    Tool<TSeq> & operator()(int i);

    Person<TSeq> * get_person();
    Model<TSeq> * get_model();
    void reset();
    bool has_tool(unsigned int t) const;
    bool has_tool(std::string name) const;

};



#endif