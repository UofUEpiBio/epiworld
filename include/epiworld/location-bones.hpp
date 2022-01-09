#ifndef EPIWORLD_LOCATION_BONES_HPP
#define EPIWORLD_LOCATION_BONES_HPP

template<typename TSeq>
class Person;

template<typename TSeq>
class Location {
private:
    
    int capacity;
    std::string location_name;
    int id;

    std::vector< Person<TSeq> * >;

    /**
     * @brief Spatial location parameters
     * 
     */
    ///@[
    epiworld_double longitude = 0.0;
    epiworld_double latitude  = 0.0;
    epiworld_double altitude  = 0.0;
    ///@]

public:
    add_person(Person<TSeq> & p);
    add_person(Person<TSeq> * p);
    size_t count() const;
    void reset();

};



#endif
