#ifndef EPIWORLD_PERSONTOOLS_BONES_HPP
#define EPIWORLD_PERSONTOOLS_BONES_HPP

/**
 * @brief List of tools available for the individual to 
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class PersonTools {
    friend class Virus<TSeq>;
    friend class Tool<TSeq>;
    friend class Person<TSeq>;

private:
    Person<TSeq> * person; 
    std::vector<Tool<TSeq>> tools;
    std::vector< int > dates;
    MixerFun<TSeq> efficacy_mixer;
    MixerFun<TSeq> transmisibility_mixer;
    MixerFun<TSeq> recovery_mixer;
    MixerFun<TSeq> death_mixer;

public:
    PersonTools() {};
    void add_tool(int date, Tool<TSeq> tool);
    double get_efficacy(Virus<TSeq> * v);
    double get_transmisibility(Virus<TSeq> * v);
    double get_recovery(Virus<TSeq> * v);
    double get_death(Virus<TSeq> * v);

    void set_efficacy_mixer(MixerFun<TSeq> fun);
    void set_transmisibility_mixer(MixerFun<TSeq> fun);
    void set_recovery_mixer(MixerFun<TSeq> fun);
    void set_death_mixer(MixerFun<TSeq> fun);

    size_t size() const;
    Tool<TSeq> & operator()(int i);

    Person<TSeq> * get_person();
    Model<TSeq> * get_model();
    void reset();

};



#endif