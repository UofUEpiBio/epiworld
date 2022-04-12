#ifndef EPIWORLD_USERDATA_BONES_HPP
#define EPIWORLD_USERDATA_BONES_HPP

template<typename TSeq>
class Model;

template<typename TSeq>
class DataBase;

template<typename TSeq>
class UserData
{
    friend class Model<TSeq>;
    friend class DataBase<TSeq>;

private:
    Model<TSeq> * model;

    std::vector< std::string > data_names;
    std::vector< int > data_dates;
    std::vector< epiworld_double > data_data;

    unsigned int k = 0u;
    unsigned int n = 0u;

    int last_day = -1;

public:

    UserData() {};

    UserData(std::vector< std::string > names);

    void add(std::vector<epiworld_double> x);
    void add(
        unsigned int j,
        epiworld_double x
        );

    epiworld_double & operator()(
        unsigned int i,
        unsigned int j
        );

    epiworld_double & operator()(
        unsigned int i,
        std::string name
        );

    std::vector< std::string > & get_names();

    std::vector< int > & get_dates();

    std::vector< epiworld_double > & get_data();

    void get_all(
        std::vector< std::string > * names    = nullptr,
        std::vector< int > * date             = nullptr,
        std::vector< epiworld_double > * data = nullptr
    );

    unsigned int nrow() const;
    unsigned int ncol() const;

    void write(std::string fn);
    void print() const;

};

#endif