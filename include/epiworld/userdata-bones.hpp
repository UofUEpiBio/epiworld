#ifndef EPIWORLD_USERDATA_BONES_HPP
#define EPIWORLD_USERDATA_BONES_HPP

template<typename TSeq>
class Model;

template<typename TSeq>
class DataBase;

/**
 * @brief Personalized data by the user
 * 
 * @tparam TSeq 
 */
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

    UserData() = delete;
    UserData(Model<TSeq> & m) : model(&m) {};

    /**
     * @brief Construct a new User Data object
     * 
     * @param names A vector of names. The length of the vector sets
     * the number of columns to record.
     */
    UserData(std::vector< std::string > names);

    /**
     * @name Append data 
     * 
     * @param x A vector of length `ncol()` (if vector), otherwise a `epiworld_double`.
     * @param j Index of the data point, from 0 to `ncol() - 1`.
     */
    ///@{
    void add(std::vector<epiworld_double> x);
    void add(
        unsigned int j,
        epiworld_double x
        );
    ///@}

    /**
     * @name Access data 
     * 
     * @param i Row (0 through ndays - 1.)
     * @param j Column (0 through `ncols()`).
     * @return epiworld_double& 
     */
    ///@{
    epiworld_double & operator()(
        unsigned int i,
        unsigned int j
        );

    epiworld_double & operator()(
        unsigned int i,
        std::string name
        );
    ///@}

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