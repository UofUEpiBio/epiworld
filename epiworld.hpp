#include <vector>
#include <functional>
#include <memory>
#include <stdexcept>
#include <random>
#include <fstream>
#include <string>
#include <map>
#include <unordered_map>
#include <chrono>
#include <climits>
#include <cstdint>

#ifndef EPIWORLD_HPP
#define EPIWORLD_HPP

namespace epiworld {

/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/config.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_CONFIG_HPP
#define EPIWORLD_CONFIG_HPP

#ifndef printf_epiworld
    #define printf_epiworld fflush(stdout);printf
#endif

#ifndef EPIWORLD_MAXNEIGHBORS
    #define EPIWORLD_MAXNEIGHBORS 100000
#endif

#ifdef EPIWORLD_USE_OMP
    #include <omp.h>
#else

#ifndef epiworld_double
    #define epiworld_double float
#endif

#ifndef epiworld_fast_int
    #define epiworld_fast_int std::int_fast16_t
#endif

#ifndef epiworld_fast_uint
    #define epiworld_fast_uint std::uint_fast16_t
#endif

#endif

template<typename TSeq>
class Model;

template<typename TSeq>
class Agent;

template<typename TSeq>
class PersonTools;

template<typename TSeq>
class Virus;

template<typename TSeq>
class Tool;

template<typename TSeq>
using VirusPtr = std::shared_ptr< Virus< TSeq > >;

template<typename TSeq>
using ToolPtr = std::shared_ptr< Tool< TSeq > >;

template<typename TSeq>
using ToolFun = std::function<epiworld_double(Tool<TSeq>&,Agent<TSeq>*,VirusPtr<TSeq>,Model<TSeq>*)>;

template<typename TSeq>
using MixerFun = std::function<epiworld_double(Agent<TSeq>*,VirusPtr<TSeq>,Model<TSeq>*)>;

template<typename TSeq>
using MutFun = std::function<bool(Agent<TSeq>*,Virus<TSeq>&,Model<TSeq>*)>;

template<typename TSeq>
using PostRecoveryFun = std::function<void(Agent<TSeq>*,Virus<TSeq>&,Model<TSeq>*)>;

template<typename TSeq>
using VirusFun = std::function<epiworld_double(Agent<TSeq>*,Virus<TSeq>&,Model<TSeq>*)>;

template<typename TSeq>
using UpdateFun = std::function<void(Agent<TSeq>*,Model<TSeq>*)>;

template<typename TSeq>
using GlobalFun = std::function<void(Model<TSeq>*)>;

template<typename TSeq>
struct Action;

template<typename TSeq>
using ActionFun = std::function<void(Action<TSeq>&,Model<TSeq>*)>;

/**
 * @brief Action data for update an agent
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
struct Action {
    Agent<TSeq> * agent;
    VirusPtr<TSeq> virus;
    ToolPtr<TSeq> tool;
    epiworld_fast_int new_status;
    epiworld_fast_int queue;
    ActionFun<TSeq> call;
public:
/**
     * @brief Construct a new Action object
     * 
     * All the parameters are rather optional.
     * 
     * @param agent_ Agent over who the action will happen
     * @param virus_ Virus to add
     * @param tool_ Tool to add
     * @param virus_idx Index of virus to be removed (if needed)
     * @param tool_idx Index of tool to be removed (if needed)
     * @param new_status_ Next status
     * @param queue_ Efect on the queue
     * @param call_ The action call (if needed)
     */
    Action(
        Agent<TSeq> * agent_,
        VirusPtr<TSeq> virus_,
        ToolPtr<TSeq> tool_,
        epiworld_fast_int new_status_,
        epiworld_fast_int queue_,
        ActionFun<TSeq> call_
    ) : agent(agent_), virus(virus_), tool(tool_), new_status(new_status_),
        queue(queue_), call(call_) {};
};

/**
 * @brief List of possible states in the model
 *  
 */
enum STATUS {
    SUSCEPTIBLE,
    EXPOSED,
    REMOVED
};

/**
 * @name Constants in epiworld 
 * 
 * @details The following are the default values some probabilities and
 * rates take when no value has been specified in the model.
 */
///@{
#ifndef DEFAULT_TOOL_CONTAGION_REDUCTION
    #define DEFAULT_TOOL_CONTAGION_REDUCTION    0.0
#endif

#ifndef DEFAULT_TOOL_TRANSMISSION_REDUCTION
    #define DEFAULT_TOOL_TRANSMISSION_REDUCTION 0.0
#endif

#ifndef DEFAULT_TOOL_RECOVERY_ENHANCER
    #define DEFAULT_TOOL_RECOVERY_ENHANCER      0.0
#endif

#ifndef DEFAULT_TOOL_DEATH_REDUCTION
    #define DEFAULT_TOOL_DEATH_REDUCTION        0.0
#endif

#ifndef EPI_DEFAULT_VIRUS_PROB_INFECTION
    #define EPI_DEFAULT_VIRUS_PROB_INFECTION    1.0
#endif

#ifndef EPI_DEFAULT_VIRUS_PROB_RECOVERY
    #define EPI_DEFAULT_VIRUS_PROB_RECOVERY     0.1428
#endif

#ifndef EPI_DEFAULT_VIRUS_PROB_DEATH
    #define EPI_DEFAULT_VIRUS_PROB_DEATH        0.0
#endif
///@}

#ifdef EPI_DEBUG
    #define EPI_DEBUG_NOTIFY_ACTIVE() \
        printf_epiworld("[epiworld-debug] DEBUGGING ON (compiled with EPI_DEBUG defined)\n");
    #define EPI_DEBUG_ALL_NON_NEGATIVE(vect) \
        for (auto & v : vect) \
            if (static_cast<double>(v) < 0.0) \
                throw std::logic_error("A negative value not allowed.");

    #define EPI_DEBUG_SUM_DBL(vect, num) \
        double _epi_debug_sum = 0.0; \
        for (auto & v : vect) \
        {   \
            _epi_debug_sum += static_cast<double>(v);\
            if (_epi_debug_sum > static_cast<double>(num)) \
                throw std::logic_error("[epiworld-debug] The sum of elements not reached."); \
        }

    #define EPI_DEBUG_SUM_INT(vect, num) \
        int _epi_debug_sum = 0; \
        for (auto & v : vect) \
        {   \
            _epi_debug_sum += static_cast<int>(v);\
            if (_epi_debug_sum > static_cast<int>(num)) \
                throw std::logic_error("[epiworld-debug] The sum of elements not reached."); \
        }

    #define EPI_DEBUG_VECTOR_MATCH_INT(a, b) \
        if (a.size() != b.size())  \
            throw std::length_error("[epiworld-debug] The vectors do not match size."); \
        for (size_t _i = 0u; _i < a.size(); ++_i) \
            if (a[_i] != b[_i]) \
                throw std::logic_error("[epiworld-debug] The vectors do not match.");


#else
    #define EPI_DEBUG_NOTIFY_ACTIVE()
    #define EPI_DEBUG_ALL_NON_NEGATIVE(vect)
    #define EPI_DEBUG_SUM_DBL(vect, num)
    #define EPI_DEBUG_SUM_INT(vect, num)
    #define EPI_DEBUG_VECTOR_MATCH_INT(a, b)
#endif

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/config.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/epiworld-macros.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MACROS_HPP
#define EPIWORLD_MACROS_HPP


/**
 * @brief Helper macro to define a new tool
 * 
 */
#define EPI_NEW_TOOL(fname,tseq) inline epiworld_double \
(fname)(\
    epiworld::Tool< tseq > & t, \
    epiworld::Agent< tseq > * p, \
    std::shared_ptr<epiworld::Virus< tseq >> v, \
    epiworld::Model< tseq > * m\
    )

/**
 * @brief Create a Tool within a function
 * 
 */
#define EPI_NEW_TOOL_LAMBDA(funname,tseq) \
    epiworld::ToolFun<tseq> funname = \
    [](epiworld::Tool<tseq> & t, \
    epiworld::Agent<tseq> * p, \
    std::shared_ptr<epiworld::Virus<tseq>> v, \
    epiworld::Model<tseq> * m) -> epiworld_double

/**
 * @brief Helper macro for accessing model parameters
 * 
 */
#define EPI_PARAMS(i) m->operator()(i)

/**
 * @brief Helper macro for defining Mutation Functions
 * 
 */
#define EPI_NEW_MUTFUN(funname,tseq) inline bool \
    (funname)(\
    epiworld::Agent<tseq> * p, \
    epiworld::Virus<tseq> & v, \
    epiworld::Model<tseq> * m )

#define EPI_NEW_MUTFUN_LAMBDA(funname,tseq) \
    epiworld::MutFun<tseq> funname = \
    [](epiworld::Agent<tseq> * p, \
    epiworld::Virus<tseq> & v, \
    epiworld::Model<tseq> * m) -> void

#define EPI_NEW_POSTRECOVERYFUN(funname,tseq) inline void \
    (funname)( \
    epiworld::Agent<tseq> * p, \
    epiworld::Virus<tseq> & v, \
    epiworld::Model<tseq> * m\
    )

#define EPI_NEW_POSTRECOVERYFUN_LAMBDA(funname,tseq) \
    epiworld::PostRecoveryFun<tseq> funname = \
    [](epiworld::Agent<tseq> * p, \
    epiworld::Virus<tseq> & v , \
    epiworld::Model<tseq> * m) -> void

#define EPI_NEW_VIRUSFUN(funname,tseq) inline epiworld_double \
    (funname)( \
    epiworld::Agent<tseq> * p, \
    epiworld::Virus<tseq> & v, \
    epiworld::Model<tseq> * m\
    )

#define EPI_NEW_VIRUSFUN_LAMBDA(funname,TSeq) \
    epiworld::VirusFun<TSeq> funname = \
    [](epiworld::Agent<TSeq> * p, \
    epiworld::Virus<TSeq> & v, \
    epiworld::Model<TSeq> * m) -> epiworld_double

#define EPI_RUNIF() m->runif()

#define EPIWORLD_RUN(a) \
    if (a.get_verbose()) \
    { \
        printf_epiworld("Running the model...\n");\
    } \
    for (unsigned int niter = 0; niter < a.get_ndays(); ++niter)

#define EPI_TOKENPASTE(a,b) a ## b
#define MPAR(num) *(m->EPI_TOKENPASTE(p,num))

#define EPI_NEW_UPDATEFUN(funname,tseq) inline void \
    (funname)(epiworld::Agent<tseq> * p, epiworld::Model<tseq> * m)

#define EPI_NEW_UPDATEFUN_LAMBDA(funname,tseq) \
    epiworld::UpdateFun<tseq> funname = \
    [](epiworld::Agent<tseq> * p, epiworld::Model<tseq> * m) -> void

#define EPI_NEW_GLOBALFUN(funname,tseq) inline void \
    (funname)(epiworld::Model<tseq>* m)

#define EPI_NEW_GLOBALFUN_LAMBDA(funname,tseq) inline void \
    epiworld::GlobalFun<tseq> funname = \
    [](epiworld::Model<tseq>* m) -> void

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/epiworld-macros.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/misc.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MISC_HPP 
#define EPIWORLD_MISC_HPP

template<typename TSeq>
class Model;

template<typename TSeq>
class Agent;

// Relevant for anything using vecHasher function ------------------------------
/**
 * @brief Vector hasher
 * @tparam T 
 */
template <typename T>
struct vecHasher {
    std::size_t operator()(std::vector< T > const&  dat) const noexcept {
        
        std::hash< T > hasher;
        std::size_t hash = hasher(dat[0u]);
        
        // ^ makes bitwise XOR
        // 0x9e3779b9 is a 32 bit constant (comes from the golden ratio)
        // << is a shift operator, something like lhs * 2^(rhs)
        if (dat.size() > 1u)
            for (unsigned int i = 1u; i < dat.size(); ++i)
                hash ^= hasher(dat[i]) + 0x9e3779b9 + (hash<<6) + (hash>>2);
        
        return hash;
        
    }
};

template<typename Ta = epiworld_double, typename Tb = unsigned int> 
using MapVec_type = std::unordered_map< std::vector< Ta >, Tb, vecHasher<Ta>>;

/**
 * @name Default sequence initializers
 * 
 * @details 
 * If the user does not provide a default sequence, this function is used when
 * a sequence needs to be initialized. Some examples: `Agent`, `Virus`, and
 * `Tool` need a default sequence.
 * 
 * @tparam TSeq 
 * @return TSeq 
 */
///@{
template<typename TSeq = int>
inline TSeq default_sequence();

int _n_sequences_created = 0;

template<>
inline bool default_sequence() {

    if (_n_sequences_created == 2)
        throw std::logic_error("Maximum number of sequence created.");

    return _n_sequences_created++ ? false : true;
}

template<>
inline int default_sequence() {
    return _n_sequences_created++;
}

template<>
inline epiworld_double default_sequence() {
    return static_cast<epiworld_double>(_n_sequences_created++);
}

template<>
inline std::vector<bool> default_sequence() {

    if (_n_sequences_created == 2)
        throw std::logic_error("Maximum number of sequence created.");

    return {_n_sequences_created++ ? false : true};
}

template<>
inline std::vector<int> default_sequence() {
    return {_n_sequences_created++};
}

template<>
inline std::vector<epiworld_double> default_sequence() {
    return {static_cast<epiworld_double>(_n_sequences_created++)};
}
///@}

/**
 * @brief Check whether `a` is included in `b`
 * 
 * @tparam Ta Type of `a`. Could be int, epiworld_double, etc.
 * @param a Scalar of class `Ta`.
 * @param b Vector `std::vector` of class `Ta`.
 * @return `true` if `a in b`, and `false` otherwise.
 */
template<typename Ta>
inline bool IN(const Ta & a, const std::vector< Ta > & b) noexcept
{
    for (const auto & i : b)
        if (a == i)
            return true;

    return false;
}

/**
 * @brief Conditional Weighted Sampling
 * 
 * @details 
 * The sampling function will draw one of `{-1, 0,...,probs.size() - 1}` in a
 * weighted fashion. The probabilities are drawn given that either one or none
 * of the cases is drawn; in the latter returns -1.
 * 
 * @param probs Vector of probabilities.
 * @param m A `Model`. This is used to draw random uniform numbers.
 * @return int If -1 then it means that none got sampled, otherwise the index
 * of the entry that got drawn.
 */
template<typename TSeq>
inline int roulette(
    const std::vector< epiworld_double > & probs,
    Model<TSeq> * m
    )
{

    // Step 1: Computing the prob on none 
    epiworld_double p_none = 1.0;
    std::vector< int > certain_infection;
    for (unsigned int p = 0u; p < probs.size(); ++p)
    {
        p_none *= (1.0 - probs[p]);

        if (probs[p] > (1 - 1e-100))
            certain_infection.push_back(p);
        
    }

    epiworld_double r = m->runif();
    // If there are one or more probs that go close to 1, sample
    // uniformly
    if (certain_infection.size() > 0)
        return certain_infection[std::floor(r * certain_infection.size())];

    // Step 2: Calculating the prob of none or single
    std::vector< epiworld_double > probs_only_p;
    epiworld_double p_none_or_single = p_none;
    for (unsigned int p = 0u; p < probs.size(); ++p)
    {
        probs_only_p.push_back(probs[p] * (p_none / (1.0 - probs[p])));
        p_none_or_single += probs_only_p[p];
    }

    // Step 3: Roulette
    epiworld_double cumsum = p_none/p_none_or_single;
    if (r < cumsum)
        return -1;

    for (unsigned int p = 0u; p < probs.size(); ++p)
    {
        // If it yield here, then bingo, the individual will acquire the disease
        cumsum += probs_only_p[p]/(p_none_or_single);
        if (r < cumsum)
            return static_cast<int>(p);
        
    }

    return static_cast<int>(probs.size() - 1u);

}

template<typename TSeq>
inline int roulette(
    unsigned int nelements,
    Model<TSeq> * m
    )
{

    #ifdef EPI_DEBUG
    if (nelements > m->array_double_tmp.size())
        throw std::logic_error("Trying to sample from more data than there is in roulette!");
    #endif

    // Step 1: Computing the prob on none 
    epiworld_double p_none = 1.0;
    unsigned int ncertain = 0u;
    // std::vector< int > certain_infection;
    for (unsigned int p = 0u; p < nelements; ++p)
    {
        p_none *= (1.0 - m->array_double_tmp[p]);

        if (m->array_double_tmp[p] > (1 - 1e-100))
            m->array_double_tmp[nelements + ncertain++] = p;
            // certain_infection.push_back(p);
        
    }

    epiworld_double r = m->runif();
    // If there are one or more probs that go close to 1, sample
    // uniformly
    if (ncertain > 0u)
        return m->array_double_tmp[nelements + std::floor(ncertain * r)]; //    certain_infection[std::floor(r * certain_infection.size())];

    // Step 2: Calculating the prob of none or single
    // std::vector< epiworld_double > probs_only_p;
    epiworld_double p_none_or_single = p_none;
    for (unsigned int p = 0u; p < nelements; ++p)
    {
        m->array_double_tmp[nelements + p] = 
            m->array_double_tmp[p] * (p_none / (1.0 - m->array_double_tmp[p]));
        p_none_or_single += m->array_double_tmp[nelements + p];
    }

    // Step 3: Roulette
    epiworld_double cumsum = p_none/p_none_or_single;
    if (r < cumsum)
        return -1;

    for (unsigned int p = 0u; p < nelements; ++p)
    {
        // If it yield here, then bingo, the individual will acquire the disease
        cumsum += m->array_double_tmp[nelements + p]/(p_none_or_single);
        if (r < cumsum)
            return static_cast<int>(p);
        
    }

    return static_cast<int>(nelements - 1u);

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/misc.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/progress.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_PROGRESS_HPP
#define EPIWORLD_PROGRESS_HPP

#ifndef EPIWORLD_PROGRESS_BAR_WIDTH
#define EPIWORLD_PROGRESS_BAR_WIDTH 80
#endif

/**
 * @brief A simple progress bar
  */
class Progress {
private:
    int    width;     ///< Total width size (number of bars)
    int    n;         ///< Total number of iterations
    epiworld_double step_size; ///< Size of the step
    int last_loc;     ///< Last location of the bar
    int cur_loc;      ///< Last location of the bar
    int i;            ///< Current iteration step
    
public:
    Progress() {};
    Progress(int n_, int width_);
    ~Progress() {};

    void start();
    void next();
    void end();

};

inline Progress::Progress(int n_, int width_) {


    width     = std::max(7, width_ - 7);
    n         = n_;
    step_size = static_cast<epiworld_double>(width)/static_cast<epiworld_double>(n);
    last_loc  = 0;
    i         = 0;

}

inline void Progress::start()
{
    for (int j = 0; j < (width); ++j)
    {
        printf_epiworld("_");
    }
    printf_epiworld("\n");
}

inline void Progress::next() {

    if (i == 0)
        start();

    cur_loc = std::floor((++i) * step_size);


    for (int j = 0; j < (cur_loc - last_loc); ++j)
    {
        printf_epiworld("|");
    }
    if (i >= n)
        end();

    last_loc = cur_loc;

}

inline void Progress::end() {

    printf_epiworld(" done.\n");

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/progress.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/userdata-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


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
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/userdata-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/userdata-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_USERDATA_MEAT_HPP
#define EPIWORLD_USERDATA_MEAT_HPP

template<typename TSeq>
class UserData;

template<typename TSeq>
inline UserData<TSeq>::UserData(std::vector< std::string > names)
{

    k = names.size();
    data_names = names;

}

template<typename TSeq>
inline void UserData<TSeq>::add(std::vector<epiworld_double> x)
{

    if (x.size() != k)
        throw std::out_of_range(
            "The size of -x-, " + std::to_string(x.size()) + ", does not match " +
            "the number of elements registered (" + std::to_string(k));

    for (auto & i : x)
        data_data.push_back(i);

    data_dates.push_back(model->today());

    n++;
    last_day = model->today();

}

template<typename TSeq>
inline void UserData<TSeq>::add(unsigned int j, epiworld_double x)
{

    // Starting with a new day?
    if (static_cast<int>(model->today()) != last_day)
    {

        std::vector< epiworld_double > tmp(k, 0.0);

        tmp[j] = x;

        add(tmp);

    }
    else
    {

        this->operator()(n - 1, j) = x;

    }

}

template<typename TSeq>
inline std::vector< std::string > & UserData<TSeq>::get_names() 
{
    return data_names;
}

template<typename TSeq>
inline std::vector< int > & UserData<TSeq>::get_dates() 
{
    return data_dates;
}

template<typename TSeq>
inline std::vector< epiworld_double > & UserData<TSeq>::get_data() 
{
    return data_data;
}

template<typename TSeq>
inline void UserData<TSeq>::get_all(
    std::vector< std::string > * names,
    std::vector< int > * dates,
    std::vector< epiworld_double > * data
) 
{
    
    if (names != nullptr)
        names = &this->data_names;

    if (dates != nullptr)
        dates = &this->data_dates;

    if (data != nullptr)
        data = &this->data_data;

}

template<typename TSeq>
inline epiworld_double & UserData<TSeq>::operator()(
    unsigned int i,
    unsigned int j
)
{

    if (j >= k)
        throw std::out_of_range("j cannot be greater than k - 1.");

    if (i >= n)
        throw std::out_of_range("j cannot be greater than n - 1.");

    return data_data[k * i + j];

}

template<typename TSeq>
inline epiworld_double & UserData<TSeq>::operator()(
    unsigned int i,
    std::string name
)
{
    int loc = -1;
    for (unsigned int l = 0u; l < k; ++l)
    {

        if (name == data_names[l])
        {

            loc = l;
            break;

        }

    }

    if (loc < 0)
        throw std::range_error(
            "The variable \"" + name + "\" is not present " +
            "in the user UserData database."
        );

    return operator()(i, static_cast<unsigned int>(loc));

}

template<typename TSeq>
inline unsigned int UserData<TSeq>::nrow() const
{
    return n;
}

template<typename TSeq>
inline unsigned int UserData<TSeq>::ncol() const
{
    return k;
}

template<typename TSeq>
inline void UserData<TSeq>::write(std::string fn)
{
    std::ofstream file_ud(fn, std::ios_base::out);

    // File header
    file_ud << "\"date\"";
    for (auto & cn : data_names)
        file_ud << " \"" + cn + "\"";
    file_ud << "\n";
    
    unsigned int ndata = 0u;
    for (unsigned int i = 0u; i < n; ++i)
    {
        file_ud << data_dates[i];

        for (unsigned int j = 0u; j < k; ++j)
            file_ud << " " << data_data[ndata++];

        file_ud << "\n";
    }

    return;
}

template<typename TSeq>
inline void UserData<TSeq>::print() const
{
    // File header
    printf_epiworld("Total records: %i\n", n);
    printf_epiworld("date");

    for (auto & cn : data_names)
    {

        printf_epiworld(" %s", cn.c_str());

    }

    printf_epiworld("\n");
    
    unsigned int ndata = 0u;
    
    for (unsigned int i = 0u; i < n; ++i)
    {

        printf_epiworld("%i", data_dates[i]);

        for (unsigned int j = 0u; j < k; ++j)
        {

            printf_epiworld(" %.2f", data_data[ndata++]);

        }

        printf_epiworld("\n");

    }

    return;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/userdata-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/seq_processing.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_SEQ_PROCESSING_HPP 
#define EPIWORLD_SEQ_PROCESSING_HPP

/**
 * @brief Hasher function to turn the sequence into an integer vector
 * 
 * @tparam TSeq 
 * @param x 
 * @return std::vector<int> 
 */
template<typename TSeq>
inline std::vector<int> default_seq_hasher(const TSeq & x);

template<>
inline std::vector<int> default_seq_hasher<std::vector<int>>(const std::vector<int> & x) {
    return x;
}

template<>
inline std::vector<int> default_seq_hasher<std::vector<bool>>(const std::vector<bool> & x) {
    std::vector<int> ans;
    for (const auto & i : x)
        ans.push_back(i? 1 : 0);
    return ans;
}

template<>
inline std::vector<int> default_seq_hasher<int>(const int & x) {
    return {x};
}

template<>
inline std::vector<int> default_seq_hasher<bool>(const bool & x) {
    return {x ? 1 : 0};
}

/**
 * @brief Default way to write sequences
 * 
 * @tparam TSeq 
 * @param seq 
 * @return std::string 
 */
template<typename TSeq = int>
inline std::string default_seq_writer(const TSeq & seq);

template<>
inline std::string default_seq_writer<std::vector<int>>(
    const std::vector<int> & seq
) {

    std::string out = "";
    for (const auto & s : seq)
        out = out + std::to_string(s);

    return out;

}

template<>
inline std::string default_seq_writer<std::vector<bool>>(
    const std::vector<bool> & seq
) {

    std::string out = "";
    for (const auto & s : seq)
        out = out + (s ? "1" : "0");

    return out;

}

template<>
inline std::string default_seq_writer<bool>(
    const bool & seq
) {

    return seq ? "1" : "0";

}

template<>
inline std::string default_seq_writer<int>(
    const int & seq
) {

    return std::to_string(seq);

}



#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/seq_processing.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/database-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_DATABASE_BONES_HPP
#define EPIWORLD_DATABASE_BONES_HPP

template<typename TSeq>
class Model;

template<typename TSeq>
class Virus;

template<typename TSeq>
class UserData;

template<typename TSeq>
inline void default_add_virus(Action<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_add_tool(Action<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_rm_virus(Action<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_rm_tool(Action<TSeq> & a, Model<TSeq> * m);

/**
 * @brief Statistical data about the process
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class DataBase {
    friend class Model<TSeq>;
    friend void default_add_virus<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_add_tool<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_virus<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_tool<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
private:
    Model<TSeq> * model;

    // Variants information 
    MapVec_type<int,int> variant_id; ///< The squence is the key
    std::vector< std::string > variant_name;
    std::vector< TSeq> variant_sequence;
    std::vector< int > variant_origin_date;
    std::vector< int > variant_parent_id;

    MapVec_type<int,int> tool_id; ///< The squence is the key
    std::vector< std::string > tool_name;
    std::vector< TSeq> tool_sequence;
    std::vector< int > tool_origin_date;

    std::function<std::vector<int>(const TSeq&)> seq_hasher = default_seq_hasher<TSeq>;
    std::function<std::string(const TSeq &)> seq_writer = default_seq_writer<TSeq>;

    // {Variant 1: {Status 1, Status 2, etc.}, Variant 2: {...}, ...}
    std::vector< std::vector<int> > today_variant;

    // {Variant 1: {Status 1, Status 2, etc.}, Variant 2: {...}, ...}
    std::vector< std::vector<int> > today_tool;

    // {Susceptible, Infected, etc.}
    std::vector< int > today_total;

    // Totals
    int today_total_nvariants_active = 0;
    
    int sampling_freq = 1;

    // Variants history
    std::vector< int > hist_variant_date;
    std::vector< int > hist_variant_id;
    std::vector< epiworld_fast_uint > hist_variant_status;
    std::vector< int > hist_variant_counts;

    // Tools history
    std::vector< int > hist_tool_date;
    std::vector< int > hist_tool_id;
    std::vector< epiworld_fast_uint > hist_tool_status;
    std::vector< int > hist_tool_counts;

    // Overall hist
    std::vector< int > hist_total_date;
    std::vector< int > hist_total_nvariants_active;
    std::vector< epiworld_fast_uint > hist_total_status;
    std::vector< int > hist_total_counts;
    std::vector< int > hist_transition_matrix;

    // Transmission network
    std::vector< int > transmission_date;
    std::vector< int > transmission_source;
    std::vector< int > transmission_target;
    std::vector< int > transmission_variant;

    std::vector< int > transition_matrix;

    UserData<TSeq> user_data;

    void update_state(
        epiworld_fast_uint prev_status,
        epiworld_fast_uint new_status
    );

    void update_virus(
        epiworld_fast_uint virus_id,
        epiworld_fast_uint prev_status,
        epiworld_fast_uint new_status
    );

    void update_tool(
        epiworld_fast_uint tool_id,
        epiworld_fast_uint prev_status,
        epiworld_fast_uint new_status
    );

    void record_transition(epiworld_fast_uint from, epiworld_fast_uint to);

public:


    DataBase() = delete;
    DataBase(Model<TSeq> & m) : model(&m), user_data(m) {};

    /**
     * @brief Registering a new variant
     * 
     * @param v Pointer to the new variant.
     * Since variants are originated in the agent, the numbers simply move around.
     * From the parent variant to the new variant. And the total number of infected
     * does not change.
     */
    void record_variant(Virus<TSeq> & v); 
    void record_tool(Tool<TSeq> & t); 
    void set_seq_hasher(std::function<std::vector<int>(TSeq)> fun);
    void set_model(Model<TSeq> & m);
    Model<TSeq> * get_model();
    void record();

    const std::vector< TSeq > & get_sequence() const;
    const std::vector< int > & get_nexposed() const;
    size_t size() const;

    /**
     * @name Get recorded information from the model
     * 
     * @param what std::string, The status, e.g., 0, 1, 2, ...
     * @return In `get_today_total`, the current counts of `what`.
     * @return In `get_today_variant`, the current counts of `what` for
     * each variant.
     * @return In `get_hist_total`, the time series of `what`
     * @return In `get_hist_variant`, the time series of what for each variant.
     * @return In `get_hist_total_date` and `get_hist_variant_date` the
     * corresponding dates
     */
    ///@{
    int get_today_total(std::string what) const;
    int get_today_total(epiworld_fast_uint what) const;
    void get_today_total(
        std::vector< std::string > * status = nullptr,
        std::vector< int > * counts = nullptr
    ) const;

    void get_today_variant(
        std::vector< std::string > & status,
        std::vector< int > & id,
        std::vector< int > & counts
    ) const;

    void get_hist_total(
        std::vector< int > * date,
        std::vector< std::string > * status,
        std::vector< int > * counts
    ) const;

    void get_hist_variant(
        std::vector< int > & date,
        std::vector< int > & id,
        std::vector< std::string > & status,
        std::vector< int > & counts
    ) const;
    ///@}

    void write_data(
        std::string fn_variant_info,
        std::string fn_variant_hist,
        std::string fn_tool_info,
        std::string fn_tool_hist,
        std::string fn_total_hist,
        std::string fn_transmission,
        std::string fn_transition
        ) const;
    
    void record_transmission(int i, int j, int variant);

    size_t get_n_variants() const;
    size_t get_n_tools() const;

    void reset();

    
    void set_user_data(std::vector< std::string > names);
    void add_user_data(std::vector< epiworld_double > x);
    void add_user_data(unsigned int j, epiworld_double x);
    UserData<TSeq> & get_user_data();


};


#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/database-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/database-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_DATABASE_MEAT_HPP
#define EPIWORLD_DATABASE_MEAT_HPP

template<typename TSeq>
inline void DataBase<TSeq>::set_model(Model<TSeq> & m)
{
    model           = &m;
    user_data.model = &m;

    reset();

    // Initializing the counts
    today_total.resize(m.nstatus);
    std::fill(today_total.begin(), today_total.end(), 0);
    for (auto & p : *m.get_agents())
        ++today_total[p.get_status()];
    
    transition_matrix.resize(m.nstatus * m.nstatus);
    std::fill(transition_matrix.begin(), transition_matrix.end(), 0);


    return;

}

template<typename TSeq>
inline Model<TSeq> * DataBase<TSeq>::get_model() {
    return model;
}

template<typename TSeq>
inline const std::vector< TSeq > & DataBase<TSeq>::get_sequence() const {
    return variant_sequence;
}

template<typename TSeq>
inline void DataBase<TSeq>::record() 
{

    ////////////////////////////////////////////////////////////////////////////
    // DEBUGGING BLOCK
    ////////////////////////////////////////////////////////////////////////////
    EPI_DEBUG_SUM_INT(today_total, model->size())
    EPI_DEBUG_ALL_NON_NEGATIVE(today_total)

    #ifdef EPI_DEBUG
    // Checking whether the sums correspond
    std::vector< int > _today_total_cp(today_total.size(), 0);
    for (auto & p : model->population)
        _today_total_cp[p.get_status()]++;
    
    EPI_DEBUG_VECTOR_MATCH_INT(_today_total_cp, today_total)
    #endif
    ////////////////////////////////////////////////////////////////////////////

    // Only store every now and then
    if ((model->today() % sampling_freq) == 0)
    {

        // Recording variant's history
        for (auto & p : variant_id)
        {

            for (unsigned int s = 0u; s < model->nstatus; ++s)
            {

                hist_variant_date.push_back(model->today());
                hist_variant_id.push_back(p.second);
                hist_variant_status.push_back(s);
                hist_variant_counts.push_back(today_variant[p.second][s]);

            }

        }

        // Recording tool's history
        for (auto & p : tool_id)
        {

            for (unsigned int s = 0u; s < model->nstatus; ++s)
            {

                hist_tool_date.push_back(model->today());
                hist_tool_id.push_back(p.second);
                hist_tool_status.push_back(s);
                hist_tool_counts.push_back(today_tool[p.second][s]);

            }

        }

        // Recording the overall history
        for (unsigned int s = 0u; s < model->nstatus; ++s)
        {
            hist_total_date.push_back(model->today());
            hist_total_nvariants_active.push_back(today_total_nvariants_active);
            hist_total_status.push_back(s);
            hist_total_counts.push_back(today_total[s]);
        }

        for (auto cell : transition_matrix)
            hist_transition_matrix.push_back(cell);

        std::fill(transition_matrix.begin(), transition_matrix.end(), 0);

    }

}

template<typename TSeq>
inline void DataBase<TSeq>::record_variant(Virus<TSeq> & v)
{

    // Updating registry
    std::vector< int > hash = seq_hasher(*v.get_sequence());
    unsigned int old_id = v.get_id();
    unsigned int new_id;
    if (variant_id.find(hash) == variant_id.end())
    {

        new_id = variant_id.size();
        variant_id[hash] = new_id;
        variant_name.push_back(v.get_name());
        variant_sequence.push_back(*v.get_sequence());
        variant_origin_date.push_back(model->today());
        
        variant_parent_id.push_back(old_id);
        
        today_variant.push_back({});
        today_variant[new_id].resize(model->nstatus, 0);
       
        // Updating the variant
        v.set_id(new_id);
        v.set_date(model->today());

        today_total_nvariants_active++;

    } else {

        // Finding the id
        new_id = variant_id[hash];

        // Reflecting the change
        v.set_id(new_id);
        v.set_date(variant_origin_date[new_id]);

    }

    // Moving statistics (only if we are affecting an individual)
    if (v.get_agent() != nullptr)
    {
        // Correcting math
        epiworld_fast_uint tmp_status = v.get_agent()->get_status();
        today_variant[old_id][tmp_status]--;
        today_variant[new_id][tmp_status]++;

    }
    
    return;
} 

template<typename TSeq>
inline void DataBase<TSeq>::record_tool(Tool<TSeq> & t)
{

    // Updating registry
    std::vector< int > hash = seq_hasher(*t.get_sequence());
    unsigned int old_id = t.get_id();
    unsigned int new_id;
    if (tool_id.find(hash) == tool_id.end())
    {

        new_id = tool_id.size();
        tool_id[hash] = new_id;
        tool_name.push_back(t.get_name());
        tool_sequence.push_back(*t.get_sequence());
        tool_origin_date.push_back(model->today());
                
        today_tool.push_back({});
        today_tool[new_id].resize(model->nstatus, 0);

        // Updating the tool
        t.set_id(new_id);
        t.set_date(model->today());

    } else {

        // Finding the id
        new_id = tool_id[hash];

        // Reflecting the change
        t.set_id(new_id);
        t.set_date(tool_origin_date[new_id]);

    }

    // Moving statistics (only if we are affecting an individual)
    if (t.get_agent() != nullptr)
    {
        // Correcting math
        epiworld_fast_uint tmp_status = t.get_agent()->get_status();
        today_tool[old_id][tmp_status]--;
        today_tool[new_id][tmp_status]++;

    }
    
    return;
} 

template<typename TSeq>
inline size_t DataBase<TSeq>::size() const
{
    return variant_id.size();
}

template<typename TSeq>
inline void DataBase<TSeq>::update_state(
        epiworld_fast_uint prev_status,
        epiworld_fast_uint new_status
) {

    today_total[prev_status]--;
    today_total[new_status]++;

    record_transition(prev_status, new_status);
    
    return;
}

template<typename TSeq>
inline void DataBase<TSeq>::update_virus(
        epiworld_fast_uint virus_id,
        epiworld_fast_uint prev_status,
        epiworld_fast_uint new_status
) {

    today_variant[virus_id][prev_status]--;
    today_variant[virus_id][new_status]++;

    return;
    
}

template<typename TSeq>
inline void DataBase<TSeq>::update_tool(
        epiworld_fast_uint tool_id,
        epiworld_fast_uint prev_status,
        epiworld_fast_uint new_status
) {


    today_tool[tool_id][prev_status]--;    
    today_tool[tool_id][new_status]++;

    return;

}

template<typename TSeq>
inline void DataBase<TSeq>::record_transition(
    epiworld_fast_uint from,
    epiworld_fast_uint to
) {

    transition_matrix[to * model->nstatus + from]++;

}

template<typename TSeq>
inline int DataBase<TSeq>::get_today_total(
    std::string what
) const
{

    for (auto i = 0u; i < model->status_labels.size(); ++i)
    {
        if (model->status_labels[i] == what)
            return today_total[i];
    }

    throw std::range_error("The value '" + what + "' is not in the model.");

}

template<typename TSeq>
inline void DataBase<TSeq>::get_today_total(
    std::vector< std::string > * status,
    std::vector< int > * counts
) const
{
    if (status != nullptr)
        (*status) = model->status_labels;

    if (counts != nullptr)
        *counts = today_total;

}

template<typename TSeq>
inline void DataBase<TSeq>::get_today_variant(
    std::vector< std::string > & status,
    std::vector< int > & id,
    std::vector< int > & counts
    ) const
{
      
    status.resize(today_variant.size(), "");
    id.resize(today_variant.size(), 0);
    counts.resize(today_variant.size(),0);

    int n = 0u;
    for (unsigned int v = 0u; v < today_variant.size(); ++v)
        for (unsigned int s = 0u; s < model->status_labels.size(); ++s)
        {
            status[n]   = model->status_labels[s];
            id[n]       = static_cast<int>(v);
            counts[n++] = today_variant[v][s];

        }

}

template<typename TSeq>
inline void DataBase<TSeq>::get_hist_total(
    std::vector< int > * date,
    std::vector< std::string > * status,
    std::vector< int > * counts
) const
{

    if (date != nullptr)
        *date = hist_total_date;

    if (status != nullptr)
    {
        status->resize(hist_total_status.size(), "");
        for (unsigned int i = 0u; i < hist_total_status.size(); ++i)
            status->operator[](i) = model->status_labels[hist_total_status[i]];
    }

    if (counts != nullptr)
        *counts = hist_total_counts;

    return;

}

template<typename TSeq>
inline void DataBase<TSeq>::get_hist_variant(
    std::vector< int > & date,
    std::vector< int > & id,
    std::vector< std::string > & status,
    std::vector< int > & counts
) const {

    date = hist_variant_date;
    std::vector< std::string > labels;
    labels = model->status_labels;
    
    id = hist_variant_id;
    status.resize(hist_variant_status.size(), "");
    for (unsigned int i = 0u; i < hist_variant_status.size(); ++i)
        status[i] = labels[hist_variant_status[i]];

    counts = hist_variant_counts;

    return;

}

template<typename TSeq>
inline void DataBase<TSeq>::write_data(
    std::string fn_variant_info,
    std::string fn_variant_hist,
    std::string fn_tool_info,
    std::string fn_tool_hist,
    std::string fn_total_hist,
    std::string fn_transmission,
    std::string fn_transition
) const
{

    if (fn_variant_info != "")
    {
        std::ofstream file_variant_info(fn_variant_info, std::ios_base::out);

        file_variant_info <<
            "id " << "variant_name " << "variant_sequence " << "date_recorded " << "parent\n";

        for (const auto & v : variant_id)
        {
            int id = v.second;
            file_variant_info <<
                id << " \"" <<
                variant_name[id] << "\" " <<
                seq_writer(variant_sequence[id]) << " " <<
                variant_origin_date[id] << " " <<
                variant_parent_id[id] << "\n";
        }

    }

    if (fn_variant_hist != "")
    {
        std::ofstream file_variant(fn_variant_hist, std::ios_base::out);
        
        file_variant <<
            "date " << "id " << "status " << "n\n";

        for (unsigned int i = 0; i < hist_variant_id.size(); ++i)
            file_variant <<
                hist_variant_date[i] << " " <<
                hist_variant_id[i] << " " <<
                model->status_labels[hist_variant_status[i]] << " " <<
                hist_variant_counts[i] << "\n";
    }

    if (fn_tool_info != "")
    {
        std::ofstream file_tool_info(fn_tool_info, std::ios_base::out);

        file_tool_info <<
            "id " << "tool_name " << "tool_sequence " << "date_recorded\n";

        for (const auto & t : tool_id)
        {
            int id = t.second;
            file_tool_info <<
                id << " \"" <<
                tool_name[id] << "\" " <<
                seq_writer(tool_sequence[id]) << " " <<
                tool_origin_date[id] << "\n";
        }

    }

    if (fn_tool_hist != "")
    {
        std::ofstream file_tool_hist(fn_tool_hist, std::ios_base::out);
        
        file_tool_hist <<
            "date " << "id " << "status " << "n\n";

        for (unsigned int i = 0; i < hist_tool_id.size(); ++i)
            file_tool_hist <<
                hist_tool_date[i] << " " <<
                hist_tool_id[i] << " " <<
                model->status_labels[hist_tool_status[i]] << " " <<
                hist_tool_counts[i] << "\n";
    }

    if (fn_total_hist != "")
    {
        std::ofstream file_total(fn_total_hist, std::ios_base::out);

        file_total <<
            "date " << "nvariants " << "status " << "counts\n";

        for (unsigned int i = 0; i < hist_total_date.size(); ++i)
            file_total <<
                hist_total_date[i] << " " <<
                hist_total_nvariants_active[i] << " \"" <<
                model->status_labels[hist_total_status[i]] << "\" " << 
                hist_total_counts[i] << "\n";
    }

    if (fn_transmission != "")
    {
        std::ofstream file_transmission(fn_transmission, std::ios_base::out);
        file_transmission <<
            "date " << "variant " << "source " << "target\n";

        for (unsigned int i = 0; i < transmission_target.size(); ++i)
            file_transmission <<
                transmission_date[i] << " " <<
                transmission_variant[i] << " " <<
                transmission_source[i] << " " <<
                transmission_target[i] << "\n";
                
    }

    if (fn_transition != "")
    {
        std::ofstream file_transition(fn_transition, std::ios_base::out);
        file_transition <<
            "date " << "from " << "to " << "counts\n";

        int ns = model->nstatus;

        for (int i = 0; i <= model->today(); ++i)
        {

            for (int from = 0u; from < ns; ++from)
                for (int to = 0u; to < ns; ++to)
                    file_transition <<
                        i << " " <<
                        model->status_labels[from] << " " <<
                        model->status_labels[to] << " " <<
                        hist_transition_matrix[i * (ns * ns) + to * ns + from] << "\n";
                
        }
                
    }

}

template<typename TSeq>
inline void DataBase<TSeq>::record_transmission(
    int i,
    int j,
    int variant
) {

    transmission_date.push_back(model->today());
    transmission_source.push_back(i);
    transmission_target.push_back(j);
    transmission_variant.push_back(variant);

}

template<typename TSeq>
inline size_t DataBase<TSeq>::get_n_variants() const
{
    return variant_id.size();
}

template<typename TSeq>
inline size_t DataBase<TSeq>::get_n_tools() const
{
    return tool_id.size();
}

template<typename TSeq>
inline void DataBase<TSeq>::reset()
{

    variant_id.clear();
    variant_name.clear();
    variant_sequence.clear();
    variant_origin_date.clear();
    variant_parent_id.clear();
    
    hist_variant_date.clear();
    hist_variant_id.clear();
    hist_variant_status.clear();
    hist_variant_counts.clear();

    tool_id.clear();
    tool_name.clear();
    tool_sequence.clear();
    tool_origin_date.clear();

    hist_tool_date.clear();
    hist_tool_id.clear();
    hist_tool_status.clear();
    hist_tool_counts.clear();
    
    hist_total_date.clear();
    hist_total_nvariants_active.clear();
    hist_total_status.clear();
    hist_total_counts.clear();
    
    transmission_date.clear();
    transmission_source.clear();
    transmission_target.clear();
    transmission_variant.clear();

    today_total_nvariants_active = 0;

    today_total.clear();
    
    today_variant.clear();

    today_tool.clear();

}

template<typename TSeq>
inline void DataBase<TSeq>::set_user_data(
    std::vector< std::string > names
)
{
    user_data = UserData<TSeq>(names);
    user_data.model = model;
}

template<typename TSeq>
inline void DataBase<TSeq>::add_user_data(
    std::vector< epiworld_double > x
)
{

    user_data.add(x);

}

template<typename TSeq>
inline void DataBase<TSeq>::add_user_data(
    unsigned int k,
    epiworld_double x
)
{

    user_data.add(k, x);

}

template<typename TSeq>
inline UserData<TSeq> & DataBase<TSeq>::get_user_data()
{
    return user_data;
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/database-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/adjlist-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_ADJLIST_BONES_HPP
#define EPIWORLD_ADJLIST_BONES_HPP

class AdjList {
private:

    std::vector<std::map<unsigned int, unsigned int>> dat;
    bool directed;
    unsigned int N = 0;
    unsigned int E = 0;

public:

    AdjList() {};

    /**
     * @brief Construct a new Adj List object
     * 
     * @details 
     * Ids in the network are assume to range from `0` to `size - 1`.
     * 
     * @param source Unsigned int vector with the source
     * @param target Unsigned int vector with the target
     * @param size Number of vertices in the network.
     * @param directed Bool true if the network is directed
     */
    AdjList(
        const std::vector< unsigned int > & source,
        const std::vector< unsigned int > & target,
        int size,
        bool directed
        );

    /**
     * @brief Read an edgelist
     * 
     * Ids in the network are assume to range from `0` to `size - 1`.
     * 
     * @param fn Path to the file
     * @param skip Number of lines to skip (e.g., 1 if there's a header)
     * @param directed `true` if the network is directed
     * @param size Number of vertices in the network.
     */
    void read_edgelist(
        std::string fn,
        int size,
        int skip = 0,
        bool directed = true
        );

    std::map<unsigned int, unsigned int> operator()(
        unsigned int i
        ) const;
        
    void print(unsigned int limit = 20u) const;
    size_t vcount() const; ///< Number of vertices/nodes in the network.
    size_t ecount() const; ///< Number of edges/arcs/ties in the network.
    
    std::vector<std::map<unsigned int,unsigned int>> & get_dat() {
        return dat;
    };

    bool is_directed() const; ///< `true` if the network is directed.

};


#endif

/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/adjlist-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/adjlist-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_ADJLIST_MEAT_HPP
#define EPIWORLD_ADJLIST_MEAT_HPP

inline AdjList::AdjList(
    const std::vector< unsigned int > & source,
    const std::vector< unsigned int > & target,
    int size,
    bool directed
) : directed(directed) {


    dat.resize(size, std::map<unsigned int,unsigned int>({}));
    int max_id = size - 1;

    int i,j;
    for (unsigned int m = 0; m < source.size(); ++m)
    {

        i = source[m];
        j = target[m];

        if (i > max_id)
            throw std::range_error(
                "The source["+std::to_string(m)+"] = " + std::to_string(i) +
                " is above the max_id " + std::to_string(max_id)
                );

        if (j > max_id)
            throw std::range_error(
                "The target["+std::to_string(m)+"] = " + std::to_string(j) +
                " is above the max_id " + std::to_string(max_id)
                );

        // Adding nodes
        if (dat[i].find(j) == dat[i].end())
            dat[i].insert(std::pair<unsigned int, unsigned int>(j, 1u));
        else
            dat[i][j]++; 
        
        if (!directed)
        {

            if (dat[j].find(i) == dat[j].end())
                dat[j].insert(std::pair<unsigned int, unsigned int>(j, 1u));
            else
                dat[j][i]++;

        }

        E++;

    }

    N = size;

    return;

}

inline void AdjList::read_edgelist(
    std::string fn,
    int size,
    int skip,
    bool directed
) {

    int i,j;
    std::ifstream filei(fn);

    if (!filei)
        throw std::logic_error("The file " + fn + " was not found.");

    int linenum = 0;
    std::vector< unsigned int > source_;
    std::vector< unsigned int > target_;

    int max_id = size - 1;

    while (!filei.eof())
    {

        if (linenum++ < skip)
            continue;

        filei >> i >> j;

        // Looking for exceptions
        if (filei.bad())
            throw std::logic_error(
                "I/O error while reading the file " +
                fn
            );

        if (filei.fail())
            break;

        if (i > max_id)
            throw std::range_error(
                "The source["+std::to_string(linenum)+"] = " + std::to_string(i) +
                " is above the max_id " + std::to_string(max_id)
                );

        if (j > max_id)
            throw std::range_error(
                "The target["+std::to_string(linenum)+"] = " + std::to_string(j) +
                " is above the max_id " + std::to_string(max_id)
                );

        source_.push_back(i);
        target_.push_back(j);

    }

    // Now using the right constructor
    *this = AdjList(source_, target_, size, directed);

    return;

}

inline std::map<unsigned int,unsigned int> AdjList::operator()(
    unsigned int i
    ) const {

    if (i >= N)
        throw std::range_error(
            "The vertex id " + std::to_string(i) + " is not in the network."
            );

    dat[i];

}
inline void AdjList::print(unsigned int limit) const {


    unsigned int counter = 0;
    printf_epiworld("Nodeset:\n");
    int i = -1;
    for (auto & n : dat)
    {

        if (counter++ > limit)
            break;

        printf_epiworld("  % 3i: {", ++i);
        int niter = 0;
        for (auto n_n : n)
            if (++niter < static_cast<int>(n.size()))
            {    
                printf_epiworld("%i, ", n_n.first);
            }
            else {
                printf_epiworld("%i}\n", n_n.first);
            }
    }

    if (limit < dat.size())
    {
        printf_epiworld(
            "  (... skipping %i records ...)\n",
            static_cast<int>(dat.size() - limit)
            );
    }

}

inline size_t AdjList::vcount() const 
{
    return N;
}

inline size_t AdjList::ecount() const 
{
    return E;
}

inline bool AdjList::is_directed() const {

    if (dat.size() == 0u)
        throw std::logic_error("The edgelist is empty.");
    
    return directed;
    
}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/adjlist-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/randgraph.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_RANDGRA
#define EPIWORLD_RANDGRA

template<typename TSeq>
class Model;

template<typename TSeq>
class Agent;

class AdjList;


template<typename TSeq, typename TDat>
inline void rewire_degseq(
    TDat * agents,
    Model<TSeq> * model,
    epiworld_double proportion
    );

template<typename TSeq = int>
inline void rewire_degseq(
    std::vector< Agent<TSeq> > * agents,
    Model<TSeq> * model,
    epiworld_double proportion
    )
{

    // Identifying individuals with degree > 0
    std::vector< unsigned int > non_isolates;
    std::vector< epiworld_double > weights;
    epiworld_double nedges = 0.0;
    // std::vector< Agent<TSeq> > * agents = model->get_agents();
    for (unsigned int i = 0u; i < agents->size(); ++i)
    {
        if (agents->operator[](i).get_neighbors().size() > 0u)
        {
            non_isolates.push_back(i);
            epiworld_double wtemp = static_cast<epiworld_double>(agents->operator[](i).get_neighbors().size());
            weights.push_back(wtemp);
            nedges += wtemp;
        }
    }

    if (non_isolates.size() == 0u)
        throw std::logic_error("The graph is completely disconnected.");

    // Cumulative probs
    weights[0u] /= nedges;
    for (unsigned int i = 1u; i < non_isolates.size(); ++i)
    {
         weights[i] /= nedges;
         weights[i] += weights[i - 1u];
    }

    // Only swap if needed
    unsigned int N = non_isolates.size();
    epiworld_double prob;
    int nrewires = floor(proportion * nedges);
    while (nrewires-- > 0)
    {

        // Picking egos
        prob = model->runif();
        int id0 = N - 1;
        for (unsigned int i = 0u; i < N; ++i)
            if (prob <= weights[i])
            {
                id0 = i;
                break;
            }

        prob = model->runif();
        int id1 = N - 1;
        for (unsigned int i = 0u; i < N; ++i)
            if (prob <= weights[i])
            {
                id1 = i;
                break;
            }

        // Correcting for under or overflow.
        if (id1 == id0)
            id1++;

        if (id1 >= static_cast<int>(N))
            id1 = 0;

        Agent<TSeq> & p0 = agents->operator[](non_isolates[id0]);
        Agent<TSeq> & p1 = agents->operator[](non_isolates[id1]);

        // Picking alters (relative location in their lists)
        // In this case, these are uniformly distributed within the list
        int id01 = std::floor(p0.get_neighbors().size() * model->runif());
        int id11 = std::floor(p1.get_neighbors().size() * model->runif());

        // When rewiring, we need to flip the individuals from the other
        // end as well, since we are dealing withi an undirected graph
        
        // Finding what neighbour is id0
        if (!model->is_directed())
        {
            // Picking 0's alter
            unsigned int n0,n1;
            Agent<TSeq> & p01 = agents->operator[](p0.get_neighbors()[id01]->get_id());
            for (n0 = 0; n0 < p01.get_neighbors().size(); ++n0)
            {

                // And getting the id of ego 0
                if (p0.get_id() == p01.get_neighbors()[n0]->get_id())
                    break;            
            }

            // Picking 1's alter
            Agent<TSeq> & p11 = agents->operator[](p1.get_neighbors()[id11]->get_id());
            for (n1 = 0; n1 < p11.get_neighbors().size(); ++n1)
            {

                // And getting the id of ego 1
                if (p1.get_id() == p11.get_neighbors()[n1]->get_id())
                    break;            
            }

            // Swapping alter's endpoints
            std::swap(p01.get_neighbors()[n0], p11.get_neighbors()[n1]);    
            
        }

        // Moving alter first
        std::swap(p0.get_neighbors()[id01], p1.get_neighbors()[id11]);
        

    }

    return;

}

template<typename TSeq>
inline void rewire_degseq(
    AdjList * agents,
    Model<TSeq> * model,
    epiworld_double proportion
    )
{

    // Identifying individuals with degree > 0
    std::vector< int > nties(agents->vcount(), 0); 
    std::vector< unsigned int > non_isolates;
    std::vector< epiworld_double > weights;
    epiworld_double nedges = 0.0;
    // std::vector< Agent<TSeq> > * agents = model->get_agents();
    auto & dat = agents->get_dat();

    for (size_t i = 0u; i < dat.size(); ++i)
        nties[i] += dat[i].size();
    
    bool directed = agents->is_directed();
    for (size_t i = 0u; i < dat.size(); ++i)
    {
        if (nties[i] > 0)
        {
            non_isolates.push_back(i);
            if (directed)
            {
                weights.push_back( 
                    static_cast<epiworld_double>(nties[i])
                );
                nedges += static_cast<epiworld_double>(nties[i]);
            }
            else {
                weights.push_back( 
                    static_cast<epiworld_double>(nties[i])/2.0
                );
                nedges += static_cast<epiworld_double>(nties[i]) / 2.0;
            }
        }
    }

    if (non_isolates.size() == 0u)
        throw std::logic_error("The graph is completely disconnected.");

    // Cumulative probs
    weights[0u] /= nedges;
    for (unsigned int i = 1u; i < non_isolates.size(); ++i)
    {
         weights[i] /= nedges;
         weights[i] += weights[i - 1u];
    }

    // Only swap if needed
    unsigned int N = non_isolates.size();
    epiworld_double prob;
    int nrewires = floor(proportion * nedges / (
        agents->is_directed() ? 1.0 : 2.0
    ));
    while (nrewires-- > 0)
    {

        // Picking egos
        prob = model->runif();
        int id0 = N - 1;
        for (unsigned int i = 0u; i < N; ++i)
            if (prob <= weights[i])
            {
                id0 = i;
                break;
            }

        prob = model->runif();
        int id1 = N - 1;
        for (unsigned int i = 0u; i < N; ++i)
            if (prob <= weights[i])
            {
                id1 = i;
                break;
            }

        // Correcting for under or overflow.
        if (id1 == id0)
            id1++;

        if (id1 >= static_cast<int>(N))
            id1 = 0;

        std::map<unsigned int,unsigned int> & p0 = agents->get_dat()[non_isolates[id0]];
        std::map<unsigned int,unsigned int> & p1 = agents->get_dat()[non_isolates[id1]];

        // Picking alters (relative location in their lists)
        // In this case, these are uniformly distributed within the list
        int id01 = std::floor(p0.size() * model->runif());
        int id11 = std::floor(p1.size() * model->runif());

        // Since it is a map, we need to find the actual ids (positions)
        // are not good enough.
        int count = 0;
        for (auto & n : p0)
            if (count++ == id01)
                id01 = n.first;

        count = 0;
        for (auto & n : p1)
            if (count++ == id11)
                id11 = n.first;

        // When rewiring, we need to flip the individuals from the other
        // end as well, since we are dealing withi an undirected graph
        
        // Finding what neighbour is id0
        if (!agents->is_directed())
        {

            std::map<unsigned int,unsigned int> & p01 = agents->get_dat()[id01];
            std::map<unsigned int,unsigned int> & p11 = agents->get_dat()[id11];

            std::swap(p01[id0], p11[id1]);
            
        }

        // Moving alter first
        std::swap(p0[id01], p1[id11]);

    }

    return;

}

template<typename TSeq>
inline AdjList rgraph_bernoulli(
    unsigned int n,
    epiworld_double p,
    bool directed,
    Model<TSeq> & model
) {

    std::vector< unsigned int > source;
    std::vector< unsigned int > target;

    // Checking the density (how many)
    std::binomial_distribution<> d(
        n * (n - 1.0) / (directed ? 1.0 : 2.0),
        p
    );

    unsigned int m = d(*model.get_rand_endgine());

    source.resize(m);
    target.resize(m);

    int a,b;
    for (unsigned int i = 0u; i < m; ++i)
    {
        a = floor(model.runif() * n);

        if (!directed)
            b = floor(model.runif() * a);
        else
        {
            b = floor(model.runif() * n);
            if (b == a)
                b++;
            
            if (b >= n)
                b = 0u;
        }

        source[i] = a;
        target[i] = b;

    }

    AdjList al(source, target, directed, 0, static_cast<int>(n) - 1);

    return al;
    
}

template<typename TSeq>
inline AdjList rgraph_bernoulli2(
    unsigned int n,
    epiworld_double p,
    bool directed,
    Model<TSeq> & model
) {

    std::vector< unsigned int > source;
    std::vector< unsigned int > target;

    // Checking the density (how many)
    std::binomial_distribution<> d(
        n * (n - 1.0) / (directed ? 1.0 : 2.0),
        p
    );

    // Need to compensate for the possible number of diagonal
    // elements sampled. If n * n, then each diag element has
    // 1/(n^2) chance of sampling

    unsigned int m = d(*model.get_rand_endgine());

    source.resize(m);
    target.resize(m);

    double n2 = static_cast<double>(n * n);

    int loc,row,col;
    for (unsigned int i = 0u; i < m; ++i)
    {
        loc = floor(model.runif() * n2);
        col = floor(static_cast<double>(loc)/static_cast<double>(n));
        row = loc - row * n;

        // Undirected needs to swap
        if (!directed && (col > row))
            std::swap(col, row);

        source[i] = row;
        target[i] = col;

    }

    AdjList al(source, target, directed, 0, static_cast<int>(n) - 1);

    return al;
    
}

inline AdjList rgraph_ring_lattice(
    unsigned int n,
    unsigned int k,
    bool directed = false
) {

    if ((n - 1u) < k)
        throw std::logic_error("k can be at most n - 1.");

    std::vector< unsigned int > source;
    std::vector< unsigned int > target;

    // if (!directed)
    //     if (k > 1u) k = static_cast< unsigned int >(floor(k / 2.0));

    for (unsigned int i = 0; i < n; ++i)
    {

        for (unsigned int j = 1u; j <= k; ++j)
        {

            // Next neighbor
            unsigned int l = i + j;
            if (l >= n) l = l - n;

            source.push_back(i);
            target.push_back(l);

        }

    }

    return AdjList(source, target, n, directed);

}

template<typename TSeq>
inline AdjList rgraph_smallworld(
    unsigned int n,
    unsigned int k,
    epiworld_double p,
    bool directed,
    Model<TSeq> & model
) {

    // Creating the ring lattice
    AdjList ring = rgraph_ring_lattice(n,k,directed);
    
    // Rewiring and returning
    if (k > 0u)
        rewire_degseq(&ring, &model, p);
        
    return ring;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/randgraph.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/queue-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_QUEUE_BONES_HPP
#define EPIWORLD_QUEUE_BONES_HPP

/**
 * @brief Controls which agents are verified at each step
 * 
 * @details The idea is that only agents who are either in
 * an infected state or have an infected neighbor should be
 * checked. Otherwise it makes no sense (no chance to recover
 * or capture the disease).
 * 
 * @tparam TSeq 
 */
template<typename TSeq = int>
class Queue
{

private:

    /**
     * @brief Count of ego's neighbors in queue (including ego)
     */
    std::vector< epiworld_fast_int > active;
    Model<TSeq> * model = nullptr;

    // Auxiliary variable that checks how many steps
    // left are there
    // int n_steps_left;
    // bool queuing_started   = false;

public:

    void operator+=(Agent<TSeq> * p);
    void operator-=(Agent<TSeq> * p);
    epiworld_fast_int operator[](unsigned int i) const;

    // void initialize(Model<TSeq> * m, Agent<TSeq> * p);
    void set_model(Model<TSeq> * m);

};

template<typename TSeq>
inline void Queue<TSeq>::operator+=(Agent<TSeq> * p)
{

    active[p->id]++;
    for (auto * n : p->neighbors)
        active[n->id]++;

}

template<typename TSeq>
inline void Queue<TSeq>::operator-=(Agent<TSeq> * p)
{

    active[p->id]--;
    for (auto * n : p->neighbors)
        active[n->id]--;

}

template<typename TSeq>
inline epiworld_fast_int Queue<TSeq>::operator[](unsigned int i) const
{
    return active[i];
}

template<typename TSeq>
inline void Queue<TSeq>::set_model(Model<TSeq> * m)
{

    model = m;
    active.resize(m->size(), 0);

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/queue-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/model-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODEL_HPP
#define EPIWORLD_MODEL_HPP

template<typename TSeq>
class Agent;

template<typename TSeq>
class Virus;

template<typename TSeq>
class Tool;

class AdjList;

template<typename TSeq>
class DataBase;

template<typename TSeq>
class Queue;

template<typename TSeq>
struct Action;

template<typename TSeq>
inline epiworld_double susceptibility_reduction_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq>* m
    );
template<typename TSeq>
inline epiworld_double transmission_reduction_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq>* m
    );
template<typename TSeq>
inline epiworld_double recovery_enhancer_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq>* m
    );
template<typename TSeq>
inline epiworld_double death_reduction_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq>* m
    );

// template<typename TSeq>
// class VirusPtr;

// template<typename TSeq>
// class ToolPtr;

/**
 * @brief Core class of epiworld.
 * 
 * The model class provides the wrapper that puts together `Agent`, `Virus`, and
 * `Tools`.
 * 
 * @tparam TSeq Type of sequence. In principle, users can build models in which
 * virus and human sequence is represented as numeric vectors (if needed.)
 */
template<typename TSeq = int>
class Model {
    friend class Agent<TSeq>;
    friend class DataBase<TSeq>;
    friend class Queue<TSeq>;
private:

    DataBase<TSeq> db = DataBase<TSeq>(*this);

    std::vector< Agent<TSeq> > population;
    bool directed = false;
    
    std::vector< VirusPtr<TSeq> > viruses;
    std::vector< epiworld_double > prevalence_virus; ///< Initial prevalence_virus of each virus
    std::vector< bool > prevalence_virus_as_proportion;
    
    std::vector< ToolPtr<TSeq> > tools;
    std::vector< epiworld_double > prevalence_tool;
    std::vector< bool > prevalence_tool_as_proportion;

    std::shared_ptr< std::mt19937 > engine =
        std::make_shared< std::mt19937 >();
    
    std::shared_ptr< std::uniform_real_distribution<> > runifd =
        std::make_shared< std::uniform_real_distribution<> >(0.0, 1.0);

    std::shared_ptr< std::normal_distribution<> > rnormd =
        std::make_shared< std::normal_distribution<> >(0.0);

    std::shared_ptr< std::gamma_distribution<> > rgammad = 
        std::make_shared< std::gamma_distribution<> >();

    std::function<void(std::vector<Agent<TSeq>>*,Model<TSeq>*,epiworld_double)> rewire_fun;
    epiworld_double rewire_prop;
        
    std::map<std::string, epiworld_double > parameters;
    unsigned int ndays;
    Progress pb;

    std::vector< UpdateFun<TSeq> >    status_fun = {};
    std::vector< std::string >        status_labels = {};
    epiworld_fast_uint nstatus = 0u;
    
    bool verbose     = true;
    bool initialized = false;
    int current_date = 0;

    void dist_tools();
    void dist_virus();

    std::chrono::time_point<std::chrono::steady_clock> time_start;
    std::chrono::time_point<std::chrono::steady_clock> time_end;

    // std::chrono::milliseconds
    std::chrono::duration<epiworld_double,std::micro> time_elapsed = 
        std::chrono::duration<epiworld_double,std::micro>::zero();
    unsigned int n_replicates = 0u;
    void chrono_start();
    void chrono_end();

    std::unique_ptr< Model<TSeq> > backup = nullptr;

    std::vector<std::function<void(Model<TSeq>*)>> global_action_functions;
    std::vector< int > global_action_dates;

    Queue<TSeq> queue;
    bool use_queuing   = true;

    /**
     * @brief Variables used to keep track of the actions
     * to be made regarding viruses.
     */
    std::vector< Action<TSeq> > actions = {};
    epiworld_fast_uint nactions = 0u;

    /**
     * @brief Construct a new Action object
     * 
     * @param agent_ Agent over which the action will be called
     * @param new_status_ New state of the agent
     * @param call_ Function the action will call
     * @param queue_ Change in the queue
     */
    void actions_add(
        Agent<TSeq> * agent_,
        VirusPtr<TSeq> virus_,
        ToolPtr<TSeq> tool_,
        epiworld_fast_uint new_status_,
        epiworld_fast_int queue_,
        ActionFun<TSeq> call_
        );

    /**
     * @brief Executes the stored action
     * 
     * @param model_ Model over which it will be executed.
     */
    void actions_run();

    /**
     * @name Tool Mixers
     * 
     * These functions combine the effects tools have to deliver
     * a single effect. For example, wearing a mask, been vaccinated,
     * and the immune system combine together to jointly reduce
     * the susceptibility for a given virus.
     * 
     */
    MixerFun<TSeq> susceptibility_reduction_mixer = susceptibility_reduction_mixer_default<TSeq>;
    MixerFun<TSeq> transmission_reduction_mixer = transmission_reduction_mixer_default<TSeq>;
    MixerFun<TSeq> recovery_enhancer_mixer = recovery_enhancer_mixer_default<TSeq>;
    MixerFun<TSeq> death_reduction_mixer = death_reduction_mixer_default<TSeq>;

public:

    std::vector<epiworld_double> array_double_tmp;
    std::vector<Virus<TSeq> * > array_virus_tmp;

    Model() {};
    Model(const Model<TSeq> & m);
    Model(Model<TSeq> && m);
    Model<TSeq> & operator=(const Model<TSeq> & m);

    void clone_population(
        std::vector< Agent<TSeq> > & p,
        bool & d,
        Model<TSeq> * m = nullptr
    ) const ;

    void clone_population(
        const Model<TSeq> & m
    );

    /**
     * @name Set the backup object
     * @details `backup` can be used to restore the entire object
     * after a run. This can be useful if the user wishes to have
     * individuals start with the same network from the beginning.
     * 
     */
    ///@{
    void set_backup();
    void restore_backup();
    ///@}

    DataBase<TSeq> & get_db();
    epiworld_double & operator()(std::string pname);

    size_t size() const;

    /**
     * @name Random number generation
     * 
     * @param eng Random number generator
     * @param s Seed
     */
    ///@{
    void set_rand_engine(std::mt19937 & eng);
    std::mt19937 * get_rand_endgine();
    void seed(unsigned int s);
    void set_rand_gamma(epiworld_double alpha, epiworld_double beta);
    epiworld_double runif();
    epiworld_double rnorm();
    epiworld_double rnorm(epiworld_double mean, epiworld_double sd);
    epiworld_double rgamma();
    epiworld_double rgamma(epiworld_double alpha, epiworld_double beta);
    ///@}

    /**
     * @name Add Virus/Tool to the model
     * 
     * This is done before the model has been initialized.
     * 
     * @param v Virus to be added
     * @param t Tool to be added
     * @param preval Initial prevalence (initial state.) It can be
     * specified as a proportion (between zero and one,) or an integer
     * indicating number of individuals.
     */
    ///@{
    void add_virus(Virus<TSeq> v, epiworld_double preval);
    void add_virus_n(Virus<TSeq> v, unsigned int preval);
    void add_tool(Tool<TSeq> t, epiworld_double preval);
    void add_tool_n(Tool<TSeq> t, unsigned int preval);
    ///@}

    /**
     * @name Accessing population of the model
     * 
     * @param fn std::string Filename of the edgelist file.
     * @param skip int Number of lines to skip in `fn`.
     * @param directed bool Whether the graph is directed or not.
     * @param size Size of the network.
     * @param al AdjList to read into the model.
     */
    ///@{
    void agents_from_adjlist(
        std::string fn,
        int size,
        int skip = 0,
        bool directed = false
        );
    void agents_from_adjlist(AdjList al);
    bool is_directed() const;
    std::vector< Agent<TSeq> > * get_agents();
    void agents_smallworld(
        unsigned int n = 1000,
        unsigned int k = 5,
        bool d = false,
        epiworld_double p = .01
        );
    ///@}

    /**
     * @name Functions to run the model
     * 
     * @param seed Seed to be used for Pseudo-RNG.
     * @param ndays Number of days (steps) of the simulation.
     * @param fun In the case of `run_multiple`, a function that is called
     * after each experiment.
     * 
     */
    ///@{
    void init(unsigned int ndays, unsigned int seed);
    void update_status();
    void mutate_variant();
    void next();
    void run(); ///< Runs the simulation (after initialization)
    void run_multiple( ///< Multiple runs of the simulation
        unsigned int nexperiments,
        std::function<void(Model<TSeq>*)> fun,
        bool reset,
        bool verbose
        );
    ///@}

    size_t get_n_variants() const;
    size_t get_n_tools() const;
    unsigned int get_ndays() const;
    unsigned int get_n_replicates() const;
    void set_ndays(unsigned int ndays);
    bool get_verbose() const;
    void verbose_off();
    void verbose_on();
    int today() const; ///< The current time of the model

    /**
     * @name Rewire the network preserving the degree sequence.
     *
     * @details This implementation assumes an undirected network,
     * thus if {(i,j), (k,l)} -> {(i,l), (k,j)}, the reciprocal
     * is also true, i.e., {(j,i), (l,k)} -> {(j,k), (l,i)}.
     * 
     * @param proportion Proportion of ties to be rewired.
     * 
     * @result A rewired version of the network.
     */
    ///@{
    void set_rewire_fun(std::function<void(std::vector<Agent<TSeq>>*,Model<TSeq>*,epiworld_double)> fun);
    void set_rewire_prop(epiworld_double prop);
    epiworld_double get_rewire_prop() const;
    void rewire();
    ///@}

    /**
     * @brief Wrapper of `DataBase::write_data`
     * 
     * @param fn_variant_info Filename. Information about the variant.
     * @param fn_variant_hist Filename. History of the variant.
     * @param fn_tool_info Filename. Information about the tool.
     * @param fn_tool_hist Filename. History of the tool.
     * @param fn_total_hist   Filename. Aggregated history (status)
     * @param fn_transmission Filename. Transmission history.
     * @param fn_transition   Filename. Markov transition history.
     */
    void write_data(
        std::string fn_variant_info,
        std::string fn_variant_hist,
        std::string fn_tool_info,
        std::string fn_tool_hist,
        std::string fn_total_hist,
        std::string fn_transmission,
        std::string fn_transition
        ) const;

    /**
     * @name Export the network data in edgelist form
     * 
     * @param fn std::string. File name.
     * @param source Integer vector
     * @param target Integer vector
     * 
     * @details When passing the source and target, the function will
     * write the edgelist on those.
     */
    ///@{
    void write_edgelist(
        std::string fn
        ) const;

    void write_edgelist(
        std::vector< unsigned int > & source,
        std::vector< unsigned int > & target
        ) const;
    ///@}

    std::map<std::string, epiworld_double> & params();

    /**
     * @brief Reset the model
     * 
     * @details Resetting the model will:
     * - clear the database
     * - restore the population (if `set_backup()` was called before)
     * - re-distribute tools
     * - re-distribute viruses
     * - set the date to 0
     * 
     */
    void reset();
    void print() const;

    Model<TSeq> && clone() const;

    /**
     * @name Manage status (states) in the model
     * 
     * @details
     * 
     * The functions `get_status` return the current values for the 
     * statuses included in the model.
     * 
     * @param lab `std::string` Name of the status.
     * 
     * @return `add_status*` returns nothing.
     * @return `get_status_*` returns a vector of pairs with the 
     * statuses and their labels.
     */
    ///@{
    void add_status(std::string lab, UpdateFun<TSeq> fun = nullptr);
    const std::vector< std::string > & get_status() const;
    const std::vector< UpdateFun<TSeq> > & get_status_fun() const;
    void print_status_codes() const;
    ///@}

    /**
     * @name Setting and accessing parameters from the model
     * 
     * @details Tools can incorporate parameters included in the model.
     * Internally, parameters in the tool are stored as pointers to
     * an std::map<> of parameters in the model. Using the `unsigned int`
     * method directly fetches the parameters in the order these were
     * added to the tool. Accessing parameters via the `std::string` method
     * involves searching the parameter directly in the std::map<> member
     * of the model (so it is not recommended.)
     * 
     * The function `set_param()` can be used when the parameter already
     * exists in the model.
     * 
     * The `par()` function members are aliases for `get_param()`.
     * 
     * @param initial_val 
     * @param pname Name of the parameter to add or to fetch
     * @return The current value of the parameter
     * in the model.
     * 
     */
    ///@{
    epiworld_double add_param(epiworld_double initial_val, std::string pname);
    epiworld_double set_param(std::string pname);
    epiworld_double get_param(unsigned int k);
    epiworld_double get_param(std::string pname);
    epiworld_double par(unsigned int k);
    epiworld_double par(std::string pname);
    epiworld_double 
        *p0,*p1,*p2,*p3,*p4,*p5,*p6,*p7,*p8,*p9,
        *p10,*p11,*p12,*p13,*p14,*p15,*p16,*p17,*p18,*p19,
        *p20,*p21,*p22,*p23,*p24,*p25,*p26,*p27,*p28,*p29,
        *p30,*p31,*p32,*p33,*p34,*p35,*p36,*p37,*p38,*p39;
    unsigned int npar_used = 0u;
    ///@}

    void get_elapsed(
        std::string unit = "auto",
        epiworld_double * last_elapsed = nullptr,
        epiworld_double * total_elapsed = nullptr,
        std::string * unit_abbr = nullptr,
        bool print = true
    ) const;

    /**
     * @name Set the user data object
     * 
     * @param names string vector with the names of the variables.
     */
    ///[@
    void set_user_data(std::vector< std::string > names);
    void add_user_data(unsigned int j, epiworld_double x);
    void add_user_data(std::vector< epiworld_double > x);
    UserData<TSeq> & get_user_data();
    ///@}

    /**
     * @brief Set a global action
     * 
     * @param fun A function to be called on the prescribed dates
     * @param date Integer indicating when the function is called (see details)
     * 
     * @details When date is less than zero, then the function is called
     * at the end of every day. Otherwise, the function will be called only
     * at the end of the indicated date.
     */
    void add_global_action(
        std::function<void(Model<TSeq>*)> fun,
        int date = -99
        );

    void run_global_actions();

    void clear_status_set();

    /**
     * @name Queuing system
     * @details When queueing is on, the model will keep track of which agents
     * are either in risk of exposure or exposed. This then is used at each 
     * step to act only on the aforementioned agents.
     * 
     */
    ////@{
    void queuing_on(); ///< Activates the queuing system (default.)
    void queuing_off(); ///< Deactivates the queuing system.
    bool is_queuing_on() const; ///< Query if the queuing system is on.
    Queue<TSeq> & get_queue(); ///< Retrieve the `Queue` object.
    ///@}

    /**
     * @name Get the susceptibility reduction object
     * 
     * @param v 
     * @return epiworld_double 
     */
    ///@{
    void set_susceptibility_reduction_mixer(MixerFun<TSeq> fun);
    void set_transmission_reduction_mixer(MixerFun<TSeq> fun);
    void set_recovery_enhancer_mixer(MixerFun<TSeq> fun);
    void set_death_reduction_mixer(MixerFun<TSeq> fun);
    ///@}

    const std::vector< VirusPtr<TSeq> > & get_viruses() const;
    const std::vector< ToolPtr<TSeq> > & get_tools() const;

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/model-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/model-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODEL_MEAT_HPP
#define EPIWORLD_MODEL_MEAT_HPP

#define CHECK_INIT() if (!initialized) \
        throw std::logic_error("Model not initialized.");

template<typename TSeq>
inline void Model<TSeq>::actions_add(
    Agent<TSeq> * agent_,
    VirusPtr<TSeq> virus_,
    ToolPtr<TSeq> tool_,
    epiworld_fast_uint new_status_,
    epiworld_fast_int queue_,
    ActionFun<TSeq> call_
) {
    
    ++nactions;

    #ifdef EPI_DEBUG
    if (nactions == 0)
        throw std::logic_error("Actions cannot be zero!!");
    #endif

    if (nactions > actions.size())
    {

        actions.push_back(
            Action<TSeq>(
                agent_, virus_, tool_, new_status_, queue_, call_
            ));

    }
    else 
    {
        Action<TSeq> & A = actions.at(nactions - 1u);
        A.agent = agent_;
        A.virus = virus_;
        A.tool = tool_;
        A.new_status = new_status_;
        A.queue = queue_;
        A.call = call_;
    }

    return;

}

template<typename TSeq>
inline void Model<TSeq>::actions_run()
{
    // Making the call
    while (nactions != 0u)
    {

        Action<TSeq>   a = actions[--nactions];
        Agent<TSeq> * p = a.agent;

        // Applying function
        if (a.call)
        {
            a.call(a, this);
        }

        // Updating status
        if (static_cast<epiworld_fast_int>(p->status) != a.new_status)
        {

            if (a.new_status >= static_cast<epiworld_fast_int>(nstatus))
                throw std::range_error(
                    "The proposed status " + std::to_string(a.new_status) + " is out of range. " +
                    "The model currently has " + std::to_string(nstatus - 1) + " statuses.");

            // Updating accounting
            db.update_state(p->status, a.new_status);

            for (size_t v = 0u; v < p->n_viruses; ++v)
                db.update_virus(p->viruses[v]->id, p->status, a.new_status);

            for (size_t t = 0u; t < p->n_tools; ++t)
                db.update_tool(p->tools[t]->id, p->status, a.new_status);

            p->status = a.new_status;
            
        }

        // Updating queue
        if (a.queue > 0)
            queue += p;
        else if (a.queue < 0)
            queue -= p;

    }

    return;
    
}

/**
 * @name Default function for combining susceptibility_reduction levels
 * 
 * @tparam TSeq 
 * @param pt 
 * @return epiworld_double 
 */
///@{
template<typename TSeq>
inline epiworld_double susceptibility_reduction_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq> * m
)
{
    epiworld_double total = 1.0;
    for (auto & tool : p->get_tools())
        total *= (1.0 - tool->get_susceptibility_reduction(v));

    return 1.0 - total;
    
}

template<typename TSeq>
inline epiworld_double transmission_reduction_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq>* m
)
{
    epiworld_double total = 1.0;
    for (auto & tool : p->get_tools())
        total *= (1.0 - tool->get_transmission_reduction(v));

    return (1.0 - total);
    
}

template<typename TSeq>
inline epiworld_double recovery_enhancer_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq>* m
)
{
    epiworld_double total = 1.0;
    for (auto & tool : p->get_tools())
        total *= (1.0 - tool->get_recovery_enhancer(v));

    return 1.0 - total;
    
}

template<typename TSeq>
inline epiworld_double death_reduction_mixer_default(
    Agent<TSeq>* p,
    VirusPtr<TSeq> v,
    Model<TSeq>* m
)
{
    epiworld_double total = 1.0;
    for (auto & tool : p->get_tools())
    {
        total *= (1.0 - tool->get_death_reduction(v));
    } 

    return 1.0 - total;
    
}
///@}

template<typename TSeq>
inline Model<TSeq>::Model(const Model<TSeq> & model) :
    db(model.db),
    viruses(model.viruses),
    prevalence_virus(model.prevalence_virus),
    tools(model.tools),
    prevalence_tool(model.prevalence_tool),
    engine(model.engine),
    runifd(model.runifd),
    parameters(model.parameters),
    ndays(model.ndays),
    pb(model.pb),
    status_fun(model.status_fun),
    status_labels(model.status_labels),
    nstatus(model.nstatus),
    verbose(model.verbose),
    initialized(model.initialized),
    current_date(model.current_date),
    global_action_functions(model.global_action_functions),
    global_action_dates(model.global_action_dates),
    queue(model.queue),
    use_queuing(model.use_queuing)
{

    // Pointing to the right place
    db.set_model(*this);

    // Removing old neighbors
    model.clone_population(
        population,
        directed,
        this
        );

    // Figure out the queuing
    if (use_queuing)
        queue.set_model(this);

    // Finally, seeds are resetted automatically based on the original
    // engine
    seed(floor(runif() * UINT_MAX));

}

template<typename TSeq>
inline Model<TSeq>::Model(Model<TSeq> && model) :
    db(std::move(model.db)),
    viruses(std::move(model.viruses)),
    prevalence_virus(std::move(model.prevalence_virus)),
    tools(std::move(model.tools)),
    prevalence_tool(std::move(model.prevalence_tool)),
    engine(std::move(model.engine)),
    runifd(std::move(model.runifd)),
    parameters(std::move(model.parameters)),
    ndays(std::move(model.ndays)),
    pb(std::move(model.pb)),
    verbose(std::move(model.verbose)),
    initialized(std::move(model.initialized)),
    current_date(std::move(model.current_date)),
    population(std::move(model.population)),
    directed(std::move(model.directed)),
    global_action_functions(std::move(model.global_action_functions)),
    global_action_dates(std::move(model.global_action_dates)),
    status_fun(std::move(model.status_fun)),
    status_labels(std::move(model.status_labels)),
    nstatus(model.nstatus),
    queue(std::move(model.queue)),
    use_queuing(model.use_queuing)
{

    // // Pointing to the right place
    // db.set_model(*this);

    // // Removing old neighbors
    // model.clone_population(
    //     population,
    //     population_ids,
    //     directed,
    //     this
    //     );

}

template<typename TSeq>
inline void Model<TSeq>::clone_population(
    std::vector< Agent<TSeq> > & p,
    bool & d,
    Model<TSeq> * model
) const {

    // Copy and clean
    p     = population;
    d     = directed;

    for (auto & p: p)
        p.neighbors.clear();
    
    // Relinking individuals
    for (unsigned int i = 0u; i < size(); ++i)
    {
        // Making room
        const Agent<TSeq> & agent_this = population[i];
        Agent<TSeq> & agent_res        = p[i];

        // Readding
        std::vector< Agent<TSeq> * > neigh = agent_this.neighbors;
        for (unsigned int n = 0u; n < neigh.size(); ++n)
        {
            // Point to the right neighbors
            int loc = p[neigh[n]->get_id()].get_id();
            agent_res.add_neighbor(&p[loc], true, true);

        }

    }
}

template<typename TSeq>
inline void Model<TSeq>::clone_population(const Model<TSeq> & m)
{
    m.clone_population(
        population,
        directed,
        this
    );
}

template<typename TSeq>
inline DataBase<TSeq> & Model<TSeq>::get_db()
{
    return db;
}

template<typename TSeq>
inline std::vector<Agent<TSeq>> * Model<TSeq>::get_agents()
{
    return &population;
}

template<typename TSeq>
inline void Model<TSeq>::agents_smallworld(
    unsigned int n,
    unsigned int k,
    bool d,
    epiworld_double p
)
{
    agents_from_adjlist(
        rgraph_smallworld(n, k, p, d, *this)
    );
}

template<typename TSeq>
inline void Model<TSeq>::set_rand_engine(std::mt19937 & eng)
{
    engine = std::make_shared< std::mt19937 >(eng);
}

template<typename TSeq>
inline void Model<TSeq>::set_rand_gamma(epiworld_double alpha, epiworld_double beta)
{
    rgammad = std::make_shared<std::gamma_distribution<>>(alpha,beta);
}

template<typename TSeq>
inline epiworld_double & Model<TSeq>::operator()(std::string pname) {

    if (parameters.find(pname) == parameters.end())
        throw std::range_error("The parameter "+ pname + "is not in the model.");

    return parameters[pname];

}

template<typename TSeq>
inline size_t Model<TSeq>::size() const {
    return population.size();
}

template<typename TSeq>
inline void Model<TSeq>::init(
    unsigned int ndays,
    unsigned int seed
    ) {

    if (initialized) 
        throw std::logic_error("Model already initialized.");

    if (nstatus == 0u)
        throw std::logic_error(
            std::string("No statuses registered in this model. ") +
            std::string("At least one status should be included. See the function -Model::add_status()-")
            );

    // Setting up the number of steps
    this->ndays = ndays;

    // Initializing population
    for (auto & p : population)
        p.model = this;

    engine->seed(seed);
    array_double_tmp.resize(size()/2, 0.0);
    array_virus_tmp.resize(size()/2);

    initialized = true;

    queue.set_model(this);

    // Checking whether the proposed status in/out/removed
    // are valid
    epiworld_fast_int _init, _end, _removed;
    int nstatus_int = static_cast<int>(nstatus);
    for (auto & v : viruses)
    {
        v->get_status(&_init, &_end, &_removed);
        
        // Negative unspecified status
        if (((_init != -99) && (_init < 0)) || (_init >= nstatus_int))
            throw std::range_error("Statuses must be between 0 and " +
                std::to_string(nstatus - 1));

        // Negative unspecified status
        if (((_end != -99) && (_end < 0)) || (_end >= nstatus_int))
            throw std::range_error("Statuses must be between 0 and " +
                std::to_string(nstatus - 1));

        if (((_removed != -99) && (_removed < 0)) || (_removed >= nstatus_int))
            throw std::range_error("Statuses must be between 0 and " +
                std::to_string(nstatus - 1));

    }

    for (auto & t : tools)
    {
        t->get_status(&_init, &_end);
        
        // Negative unspecified status
        if (((_init != -99) && (_init < 0)) || (_init >= nstatus_int))
            throw std::range_error("Statuses must be between 0 and " +
                std::to_string(nstatus - 1));

        // Negative unspecified status
        if (((_end != -99) && (_end < 0)) || (_end >= nstatus_int))
            throw std::range_error("Statuses must be between 0 and " +
                std::to_string(nstatus - 1));

    }

    // Starting first infection and tools
    reset();



}

template<typename TSeq>
inline void Model<TSeq>::dist_virus()
{


    // Starting first infection
    int n = size();
    std::vector< size_t > idx(n);

    int n_left = n;
    std::iota(idx.begin(), idx.end(), 0);

    for (unsigned int v = 0; v < viruses.size(); ++v)
    {
        // Picking how many
        int nsampled;
        if (prevalence_virus_as_proportion[v])
        {
            nsampled = static_cast<int>(std::floor(prevalence_virus[v] * size()));
        }
        else
        {
            nsampled = static_cast<int>(prevalence_virus[v]);
        }

        if (nsampled > static_cast<int>(size()))
            throw std::range_error("There are only " + std::to_string(size()) + 
            " individuals in the population. Cannot add the virus to " + std::to_string(nsampled));


        VirusPtr<TSeq> virus = viruses[v];
        
        while (nsampled > 0)
        {

            int loc = static_cast<unsigned int>(floor(runif() * (n_left--)));

            Agent<TSeq> & agent = population[idx[loc]];
            
            // Adding action
            agent.add_virus(virus, virus->status_init, virus->queue_init);

            // Adjusting sample
            nsampled--;
            std::swap(idx[loc], idx[n_left]);


        }

        // Apply the actions
        actions_run();
    }      

}

template<typename TSeq>
inline void Model<TSeq>::dist_tools()
{

    // Starting first infection
    int n = size();
    std::vector< size_t > idx(n);
    for (unsigned int t = 0; t < tools.size(); ++t)
    {
        // Picking how many
        int nsampled;
        if (prevalence_tool_as_proportion[t])
        {
            nsampled = static_cast<int>(std::floor(prevalence_tool[t] * size()));
        }
        else
        {
            nsampled = static_cast<int>(prevalence_tool[t]);
        }

        if (nsampled > static_cast<int>(size()))
            throw std::range_error("There are only " + std::to_string(size()) + 
            " individuals in the population. Cannot add the tool to " + std::to_string(nsampled));
        
        ToolPtr<TSeq> tool = tools[t];

        int n_left = n;
        std::iota(idx.begin(), idx.end(), 0);
        while (nsampled > 0)
        {
            int loc = static_cast<unsigned int>(floor(runif() * n_left--));
            
            population[idx[loc]].add_tool(tool, tool->status_init, tool->queue_init);
            
            nsampled--;

            std::swap(idx[loc], idx[n_left]);

        }

        // Apply the actions
        actions_run();

    }

}

template<typename TSeq>
inline void Model<TSeq>::chrono_start() {
    time_start = std::chrono::steady_clock::now();
}

template<typename TSeq>
inline void Model<TSeq>::chrono_end() {
    time_end = std::chrono::steady_clock::now();
    time_elapsed += (time_end - time_start);
    n_replicates++;
}

template<typename TSeq>
inline void Model<TSeq>::set_backup()
{

    backup = std::unique_ptr<Model<TSeq>>(new Model<TSeq>(*this));

}

template<typename TSeq>
inline void Model<TSeq>::restore_backup()
{

    if (backup != nullptr)
    {

        clone_population(*backup);

        db = backup->db;
        db.set_model(*this);

    }

}

template<typename TSeq>
inline std::mt19937 * Model<TSeq>::get_rand_endgine()
{
    return engine.get();
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::runif() {
    // CHECK_INIT()
    return runifd->operator()(*engine);
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rnorm() {
    // CHECK_INIT()
    return (rnormd->operator()(*engine));
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rnorm(epiworld_double mean, epiworld_double sd) {
    // CHECK_INIT()
    return (rnormd->operator()(*engine)) * sd + mean;
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rgamma() {
    return rgammad->operator()(*engine);
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::rgamma(epiworld_double alpha, epiworld_double beta) {
    auto old_param = rgammad->param();
    rgammad->param(std::gamma_distribution<>::param_type(alpha, beta));
    epiworld_double ans = rgammad->operator()(*engine);
    rgammad->param(old_param);
    return ans;
}

template<typename TSeq>
inline void Model<TSeq>::seed(unsigned int s) {
    this->engine->seed(s);
}

template<typename TSeq>
inline void Model<TSeq>::add_virus(Virus<TSeq> v, epiworld_double preval)
{

    if (preval > 1.0)
        throw std::range_error("Prevalence of virus cannot be above 1.0");

    if (preval < 0.0)
        throw std::range_error("Prevalence of virus cannot be negative");

    // Checking the status
    epiworld_fast_int init_, post_, rm_;
    v.get_status(&init_, &post_, &rm_);

    if (init_ == -99)
        throw std::logic_error(
            "The virus \"" + v.get_name() + "\" has no -init- status."
            );
    else if (post_ == -99)
        throw std::logic_error(
            "The virus \"" + v.get_name() + "\" has no -post- status."
            );
    // else if (rm_ == -99)
    //     throw std::logic_error(
    //         "The virus \"" + v.get_name() + "\" has no -rm- status."
    //         );

    // Setting the id
    v.set_id(viruses.size());
    
    // Adding new virus
    viruses.push_back(std::make_shared< Virus<TSeq> >(v));
    prevalence_virus.push_back(preval);
    prevalence_virus_as_proportion.push_back(true);

}

template<typename TSeq>
inline void Model<TSeq>::add_virus_n(Virus<TSeq> v, unsigned int preval)
{

    // Checking the ids
    epiworld_fast_int init_, post_, rm_;
    v.get_status(&init_, &post_, &rm_);

    if (init_ == -99)
        throw std::logic_error(
            "The virus \"" + v.get_name() + "\" has no -init- status."
            );
    else if (post_ == -99)
        throw std::logic_error(
            "The virus \"" + v.get_name() + "\" has no -post- status."
            );
    // else if (rm_ == -99)
    //     throw std::logic_error(
    //         "The virus \"" + v.get_name() + "\" has no -rm- status."
    //         );


    // Setting the id
    v.set_id(viruses.size());

    // Adding new virus
    viruses.push_back(std::make_shared< Virus<TSeq> >(v));
    prevalence_virus.push_back(preval);
    prevalence_virus_as_proportion.push_back(false);

}

template<typename TSeq>
inline void Model<TSeq>::add_tool(Tool<TSeq> t, epiworld_double preval)
{

    if (preval > 1.0)
        throw std::range_error("Prevalence of tool cannot be above 1.0");

    if (preval < 0.0)
        throw std::range_error("Prevalence of tool cannot be negative");

    // Adding the tool to the model (and database.)
    tools.push_back(std::make_shared< Tool<TSeq> >(t));
    prevalence_tool.push_back(preval);
    prevalence_tool_as_proportion.push_back(true);

}

template<typename TSeq>
inline void Model<TSeq>::add_tool_n(Tool<TSeq> t, unsigned int preval)
{
    t.id = tools.size();
    tools.push_back(std::make_shared<Tool<TSeq> >(t));
    prevalence_tool.push_back(preval);
    prevalence_tool_as_proportion.push_back(false);
}

template<typename TSeq>
inline void Model<TSeq>::agents_from_adjlist(
    std::string fn,
    int size,
    int skip,
    bool directed
    ) {

    AdjList al;
    al.read_edgelist(fn, size, skip, directed);
    this->agents_from_adjlist(al);

}

template<typename TSeq>
inline void Model<TSeq>::agents_from_adjlist(AdjList al) {

    // Resizing the people
    population.clear();
    population.resize(al.vcount(), Agent<TSeq>());

    const auto & tmpdat = al.get_dat();

    // Filling the model and ids
    size_t i = 0u;
    for (auto & p : population)
    {
        p.model = this;
        p.id    = i++;
    }
    
    for (size_t i = 0u; i < tmpdat.size(); ++i)
    {

        population[i].id    = i;
        population[i].model = this;

        for (const auto & link: tmpdat[i])
        {

            population[i].add_neighbor(
                &population[link.first],
                true, true
                );

        }

    }

    #ifdef EPI_DEBUG
    for (auto & p: population)
    {
        if (p.id >= static_cast<int>(al.vcount()))
            throw std::logic_error(
                "Agent's id cannot be negative above or equal to the number of agents!");

        for (const auto & n : p.neighbors)
        {
            if (n == nullptr)
                throw std::logic_error("A neighbor cannot be nullptr!");
        }
    }
    #endif

}

template<typename TSeq>
inline bool Model<TSeq>::is_directed() const
{
    if (population.size() == 0u)
        throw std::logic_error("The population hasn't been initialized.");

    return directed;
}

template<typename TSeq>
inline int Model<TSeq>::today() const {
    return this->current_date;
}

template<typename TSeq>
inline void Model<TSeq>::next() {

    ++this->current_date;
    db.record();
    
    // Advancing the progress bar
    if (verbose)
        pb.next();

    #ifdef EPI_DEBUG
    // A possible check here
    #endif

    return ;
}

template<typename TSeq>
inline void Model<TSeq>::run() 
{

    // Initializing the simulation
    chrono_start();
    EPIWORLD_RUN((*this))
    {

        // We can execute these components in whatever order the
        // user needs.
        this->update_status();
    
        // We start with the global actions
        this->run_global_actions();

        // In this case we are applying degree sequence rewiring
        // to change the network just a bit.
        this->rewire();

        // This locks all the changes
        this->next();

        // Mutation must happen at the very end of all
        this->mutate_variant();

    }
    chrono_end();

}

template<typename TSeq>
inline void Model<TSeq>::run_multiple(
    unsigned int nexperiments,
    std::function<void(Model<TSeq>*)> fun,
    bool reset,
    bool verbose
)
{

    if (reset)
        set_backup();

    bool old_verb = this->verbose;
    verbose_off();

    Progress pb_multiple(
        nexperiments,
        EPIWORLD_PROGRESS_BAR_WIDTH
        )
        ;
    if (verbose)
    {

        printf_epiworld(
            "Starting multiple runs (%i)\n", 
            static_cast<int>(nexperiments)
        );

        pb_multiple.start();

    }

    for (unsigned int n = 0u; n < nexperiments; ++n)
    {
        
        run();

        fun(this);

        if (n < (nexperiments - 1u) && reset)
            this->reset();

        if (verbose)
            pb_multiple.next();
    
    }

    if (verbose)
        pb_multiple.end();

    if (old_verb)
        verbose_on();

    return;

}

template<typename TSeq>
inline void Model<TSeq>::update_status() {

    // Next status
    if (use_queuing)
    {
        int i = -1;
        for (auto & p: population)
            if (queue[++i] > 0)
            {
                if (status_fun[p.status])
                    status_fun[p.status](&p, this);
            }

    }
    else
    {

        for (auto & p: population)
            if (status_fun[p.status])
                    status_fun[p.status](&p, this);

    }

    actions_run();
    
}

template<typename TSeq>
inline void Model<TSeq>::mutate_variant() {

    for (auto & p: population)
    {

        if (p.n_viruses > 0u)
            for (auto & v : p.viruses)
                v->mutate();

    }

}

template<typename TSeq>
inline size_t Model<TSeq>::get_n_variants() const {
    return db.size();
}

template<typename TSeq>
inline size_t Model<TSeq>::get_n_tools() const {
    return tools.size();
}

template<typename TSeq>
inline unsigned int Model<TSeq>::get_ndays() const {
    return ndays;
}

template<typename TSeq>
inline unsigned int Model<TSeq>::get_n_replicates() const
{
    return n_replicates;
}

template<typename TSeq>
inline void Model<TSeq>::set_ndays(unsigned int ndays) {
    this->ndays = ndays;
}

template<typename TSeq>
inline bool Model<TSeq>::get_verbose() const {
    return verbose;
}

template<typename TSeq>
inline void Model<TSeq>::verbose_on() {
    verbose = true;
}

template<typename TSeq>
inline void Model<TSeq>::verbose_off() {
    verbose = false;
}

template<typename TSeq>
inline void Model<TSeq>::set_rewire_fun(
    std::function<void(std::vector<Agent<TSeq>>*,Model<TSeq>*,epiworld_double)> fun
    ) {
    rewire_fun = fun;
}

template<typename TSeq>
inline void Model<TSeq>::set_rewire_prop(epiworld_double prop)
{

    if (prop < 0.0)
        throw std::range_error("Proportions cannot be negative.");

    if (prop > 1.0)
        throw std::range_error("Proportions cannot be above 1.0.");

    rewire_prop = prop;
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::get_rewire_prop() const {
    return rewire_prop;
}

template<typename TSeq>
inline void Model<TSeq>::rewire() {

    if (rewire_fun)
        rewire_fun(&population, this, rewire_prop);
}


template<typename TSeq>
inline void Model<TSeq>::write_data(
    std::string fn_variant_info,
    std::string fn_variant_hist,
    std::string fn_tool_info,
    std::string fn_tool_hist,
    std::string fn_total_hist,
    std::string fn_transmission,
    std::string fn_transition
    ) const
{

    db.write_data(
        fn_variant_info, fn_variant_hist,
        fn_tool_info, fn_tool_hist,
        fn_total_hist, fn_transmission, fn_transition);

}

template<typename TSeq>
inline void Model<TSeq>::write_edgelist(
    std::string fn
    ) const
{

    // Figuring out the writing sequence
    std::vector< const Agent<TSeq> * > wseq(size());
    for (const auto & p: population)
        wseq[p.id] = &p;

    std::ofstream efile(fn, std::ios_base::out);
    efile << "source target\n";
    if (this->is_directed())
    {

        for (const auto & p : wseq)
        {
            for (auto & n : p->neighbors)
                efile << p->id << " " << n->id << "\n";
        }

    } else {

        for (const auto & p : wseq)
        {
            for (auto & n : p->neighbors)
                if (p->id <= n->id)
                    efile << p->id << " " << n->id << "\n";
        }

    }

}

template<typename TSeq>
inline std::map<std::string,epiworld_double> & Model<TSeq>::params()
{
    return parameters;
}

template<typename TSeq>
inline void Model<TSeq>::reset() {
    
    // Restablishing people
    pb = Progress(ndays, 80);

    if (backup != nullptr)
    {
        backup->clone_population(
            population,
            directed,
            this
        );
    }

    for (auto & p : population)
        p.reset();
    
    current_date = 0;

    db.set_model(*this);

    // Recording variants
    for (auto & v : viruses)
        db.record_variant(*v);

    // Recording tools
    for (auto & t : tools)
        db.record_tool(*t);

    if (use_queuing)
        queue.set_model(this);

    // Re distributing tools and virus
    dist_virus();
    dist_tools();

    // Recording the original state
    db.record();

}

// Too big to keep here
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld//model-meat-print.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_MODEL_MEAT_PRINT_HPP
#define EPIWORLD_MODEL_MEAT_PRINT_HPP

template<typename TSeq>
inline void Model<TSeq>::print() const
{

    // Horizontal line
    std::string line = "";
    for (unsigned int i = 0u; i < 80u; ++i)
        line += "_";

    // Prints a message if debugging is on
    EPI_DEBUG_NOTIFY_ACTIVE()

    printf_epiworld("\n%s\n%s\n\n",line.c_str(), "SIMULATION STUDY");
    printf_epiworld("Population size    : %i\n", static_cast<int>(size()));
    printf_epiworld("Days (duration)    : %i (of %i)\n", today(), ndays);
    printf_epiworld("Number of variants : %i\n", static_cast<int>(db.get_n_variants()));
    if (n_replicates > 0u)
    {
        std::string abbr;
        epiworld_double elapsed;
        epiworld_double total;
        get_elapsed("auto", &elapsed, &total, &abbr, false);
        printf_epiworld("Last run elapsed t : %.2f%s\n", elapsed, abbr.c_str());
        if (n_replicates > 1u)
        {
            printf_epiworld("Total elapsed t    : %.2f%s (%i runs)\n", total, abbr.c_str(), n_replicates);
        }

    } else {
        printf_epiworld("Last run elapsed t : -\n");
    }
    
    
    if (rewire_fun)
    {
        printf_epiworld("Rewiring           : on (%.2f)\n\n", rewire_prop);
    } else {
        printf_epiworld("Rewiring           : off\n\n");
    }
    

    printf_epiworld("Virus(es):\n");
    size_t n_variants_model = viruses.size();
    for (size_t i = 0u; i < db.get_n_variants(); ++i)
    {    

        if ((n_variants_model > 10) && (i >= 10))
        {
            printf_epiworld(" ...and %li more variants...\n", n_variants_model - i);
            break;
        }

        if (i < n_variants_model)
        {

            if (prevalence_virus_as_proportion[i])
            {

                printf_epiworld(
                    " - %s (baseline prevalence: %.2f%%)\n",
                    db.variant_name[i].c_str(),
                    prevalence_virus[i] * 100.00
                );

            }
            else
            {

                printf_epiworld(
                    " - %s (baseline prevalence: %i seeds)\n",
                    db.variant_name[i].c_str(),
                    static_cast<int>(prevalence_virus[i])
                );

            }

        } else {

            printf_epiworld(
                " - %s (originated in the model...)\n",
                db.variant_name[i].c_str()
            );

        }

    }

    printf_epiworld("\nTool(s):\n");
    size_t n_tools_model = tools.size();
    for (size_t i = 0u; i < db.tool_id.size(); ++i)
    {   

        if ((n_tools_model > 10) && (i >= 10))
        {
            printf_epiworld(" ...and %li more tools...\n", n_tools_model - i);
            break;
        }

        if (i < n_tools_model)
        {
            if (prevalence_tool_as_proportion[i])
            {

                printf_epiworld(
                    " - %s (baseline prevalence: %.2f%%)\n",
                    db.tool_name[i].c_str(),
                    prevalence_tool[i] * 100.0
                    );

            }
            else
            {

                printf_epiworld(
                    " - %s (baseline prevalence: %i seeds)\n",
                    db.tool_name[i].c_str(),
                    static_cast<int>(prevalence_tool[i])
                    );

            }

        } else {
            printf_epiworld(
                " - %s (originated in the model...)\n",
                db.tool_name[i].c_str()
            );
        }
        

    }

    if (db.tool_id.size() == 0u)
    {
        printf_epiworld(" (none)\n");
    }

    // Information about the parameters included
    printf_epiworld("\nModel parameters:\n");
    unsigned int nchar = 0u;
    for (auto & p : parameters)
        if (p.first.length() > nchar)
            nchar = p.first.length();

    std::string fmt = " - %-" + std::to_string(nchar + 1) + "s: ";
    for (auto & p : parameters)
    {
        std::string fmt_tmp = fmt;
        if (std::fabs(p.second) < 0.0001)
            fmt_tmp += "%.1e\n";
        else
            fmt_tmp += "%.4f\n";

        printf_epiworld(
            fmt_tmp.c_str(),
            p.first.c_str(),
            p.second
        );
        
    }

    if (parameters.size() == 0u)
    {
        printf_epiworld(" (none)\n");
    }

    nchar = 0u;
    for (auto & p : status_labels)
        if (p.length() > nchar)
            nchar = p.length();

    

    if (initialized) 
    {
        
        if (today() != 0)
            fmt = " - (%" + std::to_string(nstatus).length() +
                std::string("d) %-") + std::to_string(nchar) + "s : %" +
                std::to_string(std::to_string(size()).length()) + "i -> %i\n";
        else
            fmt = " - (%" + std::to_string(nstatus).length() +
                std::string("d) %-") + std::to_string(nchar) + "s : %i\n";

    }
    else
        fmt = " - (%" + std::to_string(nstatus).length() +
            std::string("d) %-") + std::to_string(nchar) + "s : %s\n";
        
    printf_epiworld("\nDistribution of the population at time %i:\n", today());
    for (size_t s = 0u; s < nstatus; ++s)
    {
        if (initialized)
        {
            
            if (today() != 0)
            {

                printf_epiworld(
                    fmt.c_str(),
                    s,
                    status_labels[s].c_str(),
                    db.hist_total_counts[s],
                    db.today_total[ s ]
                    );

            }
            else
            {

                printf_epiworld(
                    fmt.c_str(),
                    s,
                    status_labels[s].c_str(),
                    db.today_total[ s ]
                    );

            }
            

        }
        else
        {

            printf_epiworld(
                fmt.c_str(),
                s,
                status_labels[s].c_str(),
                " - "
                );

        }
    }

    return;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld//model-meat-print.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



template<typename TSeq>
inline Model<TSeq> && Model<TSeq>::clone() const {

    // Step 1: Regen the individuals and make sure that:
    //  - Neighbors point to the right place
    //  - DB is pointing to the right place
    Model<TSeq> res(*this);

    // Pointing to the right place
    res.get_db().set_model(res);

    // Removing old neighbors
    for (auto & p: res.population)
        p.neighbors.clear();
    
    // Rechecking individuals
    for (unsigned int p = 0u; p < size(); ++p)
    {
        // Making room
        const Agent<TSeq> & agent_this = population[p];
        Agent<TSeq> & agent_res  = res.population[p];

        // Agent pointing to the right model and agent
        agent_res.model        = &res;
        agent_res.viruses.agent = &agent_res;
        agent_res.tools.agent = &agent_res;

        // Readding
        std::vector< Agent<TSeq> * > neigh = agent_this.neighbors;
        for (unsigned int n = 0u; n < neigh.size(); ++n)
        {
            // Point to the right neighbors
            int loc = res.population_ids[neigh[n]->get_id()];
            agent_res.add_neighbor(&res.population[loc], true, true);

        }

    }

    return res;

}



template<typename TSeq>
inline void Model<TSeq>::add_status(
    std::string lab, 
    UpdateFun<TSeq> fun
)
{
    if (this->initialized)
        throw std::logic_error("Cannot add status once the model has been initialized.");

    // Checking it doesn't match
    for (auto & s : status_labels)
        if (s == lab)
            throw std::logic_error("Status \"" + s + "\" already registered.");

    status_labels.push_back(lab);
    status_fun.push_back(fun);
    nstatus++;

}


template<typename TSeq>
inline const std::vector< std::string > &
Model<TSeq>::get_status() const
{
    return status_labels;
}

template<typename TSeq>
inline const std::vector< UpdateFun<TSeq> > &
Model<TSeq>::get_status_fun() const
{
    return status_fun;
}

template<typename TSeq>
inline void Model<TSeq>::print_status_codes() const
{

    // Horizontal line
    std::string line = "";
    for (unsigned int i = 0u; i < 80u; ++i)
        line += "_";

    printf_epiworld("\n%s\nSTATUS CODES\n\n", line.c_str());

    unsigned int nchar = 0u;
    for (auto & p : status_labels)
        if (p.length() > nchar)
            nchar = p.length();
    
    std::string fmt = " %2i = %-" + std::to_string(nchar + 1 + 4) + "s\n";
    for (unsigned int i = 0u; i < nstatus; ++i)
    {

        printf_epiworld(
            fmt.c_str(),
            i,
            (status_labels[i] + " (S)").c_str()
        );

    }

}

#define CASE_PAR(a,b) case a: b = &(parameters[pname]);break;
#define CASES_PAR(a) \
    switch (a) \
    { \
    CASE_PAR(0u, p0) CASE_PAR(1u, p1) CASE_PAR(2u, p2) CASE_PAR(3u, p3) \
    CASE_PAR(4u, p4) CASE_PAR(5u, p5) CASE_PAR(6u, p6) CASE_PAR(7u, p7) \
    CASE_PAR(8u, p8) CASE_PAR(9u, p9) \
    CASE_PAR(10u, p10) CASE_PAR(11u, p11) CASE_PAR(12u, p12) CASE_PAR(13u, p13) \
    CASE_PAR(14u, p14) CASE_PAR(15u, p15) CASE_PAR(16u, p16) CASE_PAR(17u, p17) \
    CASE_PAR(18u, p18) CASE_PAR(19u, p19) \
    CASE_PAR(20u, p20) CASE_PAR(21u, p21) CASE_PAR(22u, p22) CASE_PAR(23u, p23) \
    CASE_PAR(24u, p24) CASE_PAR(25u, p25) CASE_PAR(26u, p26) CASE_PAR(27u, p27) \
    CASE_PAR(28u, p28) CASE_PAR(29u, p29) \
    CASE_PAR(30u, p30) CASE_PAR(31u, p31) CASE_PAR(32u, p22) CASE_PAR(33u, p23) \
    CASE_PAR(34u, p34) CASE_PAR(35u, p35) CASE_PAR(36u, p26) CASE_PAR(37u, p27) \
    CASE_PAR(38u, p38) CASE_PAR(39u, p39) \
    default: \
        break; \
    }

template<typename TSeq>
inline epiworld_double Model<TSeq>::add_param(
    epiworld_double initial_value,
    std::string pname
    ) {

    if (parameters.find(pname) == parameters.end())
        parameters[pname] = initial_value;

    CASES_PAR(npar_used++)
    
    return initial_value;

}

template<typename TSeq>
inline epiworld_double Model<TSeq>::set_param(
    std::string pname
    ) {

    if (parameters.find(pname) == parameters.end())
        throw std::logic_error("The parameter " + pname + " does not exists.");

    CASES_PAR(npar_used++)

    return parameters[pname];
    
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::get_param(std::string pname)
{
    if (parameters.find(pname) == parameters.end())
        throw std::logic_error("The parameter " + pname + " does not exists.");

    return parameters[pname];
}

template<typename TSeq>
inline epiworld_double Model<TSeq>::par(std::string pname)
{
    return parameters[pname];
}

#define DURCAST(tunit,txtunit) {\
        elapsed       = std::chrono::duration_cast<std::chrono:: tunit>(\
            time_end - time_start).count(); \
        elapsed_total = std::chrono::duration_cast<std::chrono:: tunit>(time_elapsed).count(); \
        abbr_unit     = txtunit;}

template<typename TSeq>
inline void Model<TSeq>::get_elapsed(
    std::string unit,
    epiworld_double * last_elapsed,
    epiworld_double * total_elapsed,
    std::string * unit_abbr,
    bool print
) const {

    // Preparing the result
    epiworld_double elapsed, elapsed_total;
    std::string abbr_unit;

    // Figuring out the length
    if (unit == "auto")
    {

        size_t tlength = std::to_string(
            static_cast<int>(floor(time_elapsed.count()))
            ).length();
        
        if (tlength <= 1)
            unit = "nanoseconds";
        else if (tlength <= 3)
            unit = "microseconds";
        else if (tlength <= 6)
            unit = "milliseconds";
        else if (tlength <= 8)
            unit = "seconds";
        else if (tlength <= 9)
            unit = "minutes";
        else 
            unit = "hours";

    }

    if (unit == "nanoseconds")       DURCAST(nanoseconds,"ns")
    else if (unit == "microseconds") DURCAST(microseconds,"\xC2\xB5s")
    else if (unit == "milliseconds") DURCAST(milliseconds,"ms")
    else if (unit == "seconds")      DURCAST(seconds,"s")
    else if (unit == "minutes")      DURCAST(minutes,"m")
    else if (unit == "hours")        DURCAST(hours,"h")
    else
        throw std::range_error("The time unit " + unit + " is not supported.");


    if (last_elapsed != nullptr)
        *last_elapsed = elapsed;
    if (total_elapsed != nullptr)
        *total_elapsed = elapsed_total;
    if (unit_abbr != nullptr)
        *unit_abbr = abbr_unit;

    if (!print)
        return;

    if (n_replicates > 1u)
    {
        printf_epiworld("last run elapsed time : %.2f%s\n",
            elapsed, abbr_unit.c_str());
        printf_epiworld("total elapsed time    : %.2f%s\n",
            elapsed_total, abbr_unit.c_str());
        printf_epiworld("total runs            : %i\n",
            static_cast<int>(n_replicates));
        printf_epiworld("mean run elapsed time : %.2f%s\n",
            elapsed_total/static_cast<epiworld_double>(n_replicates), abbr_unit.c_str());

    } else {
        printf_epiworld("last run elapsed time : %.2f%s.\n", elapsed, abbr_unit.c_str());
    }
}

template<typename TSeq>
inline void Model<TSeq>::set_user_data(std::vector< std::string > names)
{
    db.set_user_data(names);
}

template<typename TSeq>
inline void Model<TSeq>::add_user_data(unsigned int j, epiworld_double x)
{
    db.add_user_data(j, x);
}

template<typename TSeq>
inline void Model<TSeq>::add_user_data(std::vector<epiworld_double> x)
{
    db.add_user_data(x);
}

template<typename TSeq>
inline UserData<TSeq> & Model<TSeq>::get_user_data()
{
    return db.get_user_data();
}

template<typename TSeq>
inline void Model<TSeq>::add_global_action(
    std::function<void(Model<TSeq>*)> fun,
    int date
)
{

    global_action_functions.push_back(fun);
    global_action_dates.push_back(date);

}

template<typename TSeq>
inline void Model<TSeq>::run_global_actions()
{

    for (unsigned int i = 0u; i < global_action_dates.size(); ++i)
    {

        if (global_action_dates[i] < 0)
        {

            global_action_functions[i](this);

        }
        else if (global_action_dates[i] == today())
        {

            global_action_functions[i](this);

        }

        actions_run();

    }

}

template<typename TSeq>
inline void Model<TSeq>::queuing_on()
{
    use_queuing = true;
}

template<typename TSeq>
inline void Model<TSeq>::queuing_off()
{
    use_queuing = false;
}

template<typename TSeq>
inline bool Model<TSeq>::is_queuing_on() const
{
    return use_queuing;
}

template<typename TSeq>
inline Queue<TSeq> & Model<TSeq>::get_queue()
{
    return queue;
}

template<typename TSeq>
inline const std::vector< VirusPtr<TSeq> > & Model<TSeq>::get_viruses() const
{
    return viruses;
}

template<typename TSeq>
const std::vector< ToolPtr<TSeq> > & Model<TSeq>::get_tools() const
{
    return tools;
}

#undef DURCAST

#undef CASES_PAR
#undef CASE_PAR

#undef CHECK_INIT
#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/model-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/viruses-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_VIRUSES_BONES_HPP
#define EPIWORLD_VIRUSES_BONES_HPP

template<typename TSeq>
class Virus;

template<typename TSeq>
class Agent;

#define VIRUSPTR std::shared_ptr< Virus<TSeq> >

/**
 * @brief Set of viruses (useful for building iterators)
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class Viruses {
    friend class Virus<TSeq>;
    friend class Agent<TSeq>;
private:
    std::vector< VIRUSPTR > * dat;
    const epiworld_fast_uint * n_viruses;

public:

    Viruses() = delete;
    Viruses(Agent<TSeq> & p) : dat(&p.viruses), n_viruses(&p.n_viruses) {};

    typename std::vector< VIRUSPTR >::iterator begin();
    typename std::vector< VIRUSPTR >::iterator end();

    VIRUSPTR & operator()(size_t i);
    VIRUSPTR & operator[](size_t i);

    size_t size() const noexcept;

};

template<typename TSeq>
inline typename std::vector< VIRUSPTR >::iterator Viruses<TSeq>::begin()
{

    if (*n_viruses == 0u)
        return dat->end();
    
    return dat->begin();
}

template<typename TSeq>
inline typename std::vector< VIRUSPTR >::iterator Viruses<TSeq>::end()
{
     
    return begin() + *n_viruses;
}

template<typename TSeq>
inline VIRUSPTR & Viruses<TSeq>::operator()(size_t i)
{

    if (i >= *n_viruses)
        throw std::range_error("Virus index out of range.");

    return dat->operator[](i);

}

template<typename TSeq>
inline VIRUSPTR & Viruses<TSeq>::operator[](size_t i)
{

    return dat->operator[](i);

}

template<typename TSeq>
inline size_t Viruses<TSeq>::size() const noexcept 
{
    return *n_viruses;
}

/**
 * @brief Set of Viruses (const) (useful for iterators)
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class Viruses_const {
    friend class Virus<TSeq>;
    friend class Agent<TSeq>;
private:
    const std::vector< VIRUSPTR > * dat;
    const epiworld_fast_uint * n_viruses;

public:

    Viruses_const() = delete;
    Viruses_const(const Agent<TSeq> & p) : dat(&p.viruses), n_viruses(&p.n_viruses) {};

    typename std::vector< VIRUSPTR >::const_iterator begin();
    typename std::vector< VIRUSPTR >::const_iterator end();

    const VIRUSPTR & operator()(size_t i);
    const VIRUSPTR & operator[](size_t i);

    size_t size() const noexcept;

};

template<typename TSeq>
inline typename std::vector< VIRUSPTR >::const_iterator Viruses_const<TSeq>::begin() {

    if (*n_viruses == 0u)
        return dat->end();
    
    return dat->begin();
}

template<typename TSeq>
inline typename std::vector< VIRUSPTR >::const_iterator Viruses_const<TSeq>::end() {
     
    return begin() + *n_viruses;
}

template<typename TSeq>
inline const VIRUSPTR & Viruses_const<TSeq>::operator()(size_t i)
{

    if (i >= *n_viruses)
        throw std::range_error("Virus index out of range.");

    return dat->operator[](i);

}

template<typename TSeq>
inline const VIRUSPTR & Viruses_const<TSeq>::operator[](size_t i)
{

    return dat->operator[](i);

}

template<typename TSeq>
inline size_t Viruses_const<TSeq>::size() const noexcept 
{
    return *n_viruses;
}

#undef VIRUSPTR



#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/viruses-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/virus-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_VIRUS_HPP
#define EPIWORLD_VIRUS_HPP

template<typename TSeq>
class Agent;

template<typename TSeq>
class Virus;


template<typename TSeq>
class Model;

/**
 * @brief Virus
 * 
 * @tparam TSeq 
 * @details
 * Raw transmisibility of a virus should be a function of its genetic
 * sequence. Nonetheless, transmisibility can be reduced as a result of
 * having one or more tools to fight the virus. Because of this, transmisibility
 * should be a function of the agent.
 */
template<typename TSeq = int>
class Virus {
    friend class Agent<TSeq>;
    friend class Model<TSeq>;
    friend class DataBase<TSeq>;
    friend void default_add_virus<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_add_tool<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_virus<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_tool<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
private:
    Agent<TSeq> * agent = nullptr;
    int        agent_idx = -99;
    std::shared_ptr<TSeq> baseline_sequence = std::make_shared<TSeq>(default_sequence<TSeq>());
    std::shared_ptr<std::string> virus_name = nullptr;
    int date = -99;
    int id   = -99;
    bool active = true;
    MutFun<TSeq>          mutation_fun                 = nullptr;
    PostRecoveryFun<TSeq> post_recovery_fun            = nullptr;
    VirusFun<TSeq>        probability_of_infecting_fun = nullptr;
    VirusFun<TSeq>        probability_of_recovery_fun  = nullptr;
    VirusFun<TSeq>        probability_of_death_fun     = nullptr;

    // Setup parameters
    std::vector< epiworld_double * > params;
    std::vector< epiworld_double > data;

    epiworld_fast_int status_init    = -99; ///< Change of status when added to agent.
    epiworld_fast_int status_post    = -99; ///< Change of status when removed from agent.
    epiworld_fast_int status_removed = -99; ///< Change of status when agent is removed

    epiworld_fast_int queue_init    = 1; ///< Change of status when added to agent.
    epiworld_fast_int queue_post    = -1; ///< Change of status when removed from agent.
    epiworld_fast_int queue_removed = -99; ///< Change of status when agent is removed

public:
    Virus(std::string name = "unknown virus");

    void mutate();
    void set_mutation(MutFun<TSeq> fun);
    
    const TSeq* get_sequence();
    void set_sequence(TSeq sequence);
    
    Agent<TSeq> * get_agent();
    void set_agent(Agent<TSeq> * p, epiworld_fast_uint idx);
    Model<TSeq> * get_model();
    
    void set_date(int d);
    int get_date() const;

    void set_id(int idx);
    int get_id() const;

    /**
     * @name Get and set the tool functions
     * 
     * @param v The virus over which to operate
     * @param fun the function to be used
     * 
     * @return epiworld_double 
     */
    ///@{
    epiworld_double get_prob_infecting();
    epiworld_double get_prob_recovery();
    epiworld_double get_prob_death();
    
    void post_recovery();
    void set_post_recovery(PostRecoveryFun<TSeq> fun);
    void set_post_immunity(epiworld_double prob);
    void set_post_immunity(epiworld_double * prob);

    void set_prob_infecting_fun(VirusFun<TSeq> fun);
    void set_prob_recovery_fun(VirusFun<TSeq> fun);
    void set_prob_death_fun(VirusFun<TSeq> fun);
    
    void set_prob_infecting(epiworld_double * prob);
    void set_prob_recovery(epiworld_double * prob);
    void set_prob_death(epiworld_double * prob);
    
    void set_prob_infecting(epiworld_double prob);
    void set_prob_recovery(epiworld_double prob);
    void set_prob_death(epiworld_double prob);
    ///@}


    void set_name(std::string name);
    std::string get_name() const;

    std::vector< epiworld_double > & get_data();

    /**
     * @name Get and set the status and queue
     * 
     * After applied, viruses can change the status and affect
     * the queue of agents. These function sets the default values,
     * which are retrieved when adding or removing a virus does not
     * specify a change in status or in queue.
     * 
     * @param init After the virus/tool is added to the agent.
     * @param end After the virus/tool is removed.
     * @param removed After the agent (Agent) is removed.
     */
    ///@{
    void set_status(
        epiworld_fast_int init,
        epiworld_fast_int end,
        epiworld_fast_int removed = -99
        );
        
    void set_queue(
        epiworld_fast_int init,
        epiworld_fast_int end,
        epiworld_fast_int removed = -99
        );

    void get_status(
        epiworld_fast_int * init,
        epiworld_fast_int * end,
        epiworld_fast_int * removed = -99
        );

    void get_queue(
        epiworld_fast_int * init,
        epiworld_fast_int * end,
        epiworld_fast_int * removed = -99
        );
    ///@}

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/virus-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/virus-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


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
inline void Virus<TSeq>::mutate() {

    if (mutation_fun)
        if (mutation_fun(agent, *this, this->get_model()))
            agent->get_model()->get_db().record_variant(*this);

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
inline Model<TSeq> * Virus<TSeq>::get_model() {
    return agent->get_model();
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
inline epiworld_double Virus<TSeq>::get_prob_infecting()
{

    if (probability_of_infecting_fun)
        return probability_of_infecting_fun(agent, *this, agent->get_model());
        
    return EPI_DEFAULT_VIRUS_PROB_INFECTION;

}



template<typename TSeq>
inline epiworld_double Virus<TSeq>::get_prob_recovery()
{

    if (probability_of_recovery_fun)
        return probability_of_recovery_fun(agent, *this, agent->get_model());
        
    return EPI_DEFAULT_VIRUS_PROB_RECOVERY;

}



template<typename TSeq>
inline epiworld_double Virus<TSeq>::get_prob_death()
{

    if (probability_of_death_fun)
        return probability_of_death_fun(agent, *this, agent->get_model());
        
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
        [prob](Agent<TSeq> * p, Virus<TSeq> & v, Model<TSeq> * m)
        {
            return *prob;
        };
    
    probability_of_infecting_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_recovery(epiworld_double * prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Agent<TSeq> * p, Virus<TSeq> & v, Model<TSeq> * m)
        {
            return *prob;
        };
    
    probability_of_recovery_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_death(epiworld_double * prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Agent<TSeq> * p, Virus<TSeq> & v, Model<TSeq> * m)
        {
            return *prob;
        };
    
    probability_of_death_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_infecting(epiworld_double prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Agent<TSeq> * p, Virus<TSeq> & v, Model<TSeq> * m)
        {
            return prob;
        };
    
    probability_of_infecting_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_recovery(epiworld_double prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Agent<TSeq> * p, Virus<TSeq> & v, Model<TSeq> * m)
        {
            return prob;
        };
    
    probability_of_recovery_fun = tmpfun;
}

template<typename TSeq>
inline void Virus<TSeq>::set_prob_death(epiworld_double prob)
{
    VirusFun<TSeq> tmpfun = 
        [prob](Agent<TSeq> * p, Virus<TSeq> & v, Model<TSeq> * m)
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
inline void Virus<TSeq>::post_recovery()
{

    if (post_recovery_fun)
        post_recovery_fun(agent, *this, agent->get_model());    

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
            Agent<TSeq> * p, Virus<TSeq> & v, Model<TSeq> * m
            )
        {
            
            // Have we registered the tool?
            if (__no_reinfect->get_id() == -99)
                m->get_db().record_tool(*__no_reinfect);

            p->add_tool(__no_reinfect);

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
        [__no_reinfect](Agent<TSeq> * p, Virus<TSeq> & v, Model<TSeq> * m)
        {

            // Have we registered the tool?
            if (__no_reinfect->get_id() == -99)
                m->get_db().record_tool(*__no_reinfect);

            p->add_tool(__no_reinfect);

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
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/virus-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


    
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/tools-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_TOOLS_BONES_HPP
#define EPIWORLD_TOOLS_BONES_HPP

template<typename TSeq>
class Tool;

template<typename TSeq>
class Agent;

#define TOOLPTR std::shared_ptr< Tool<TSeq> >

/**
 * @brief Set of tools (useful for building iterators)
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class Tools {
    friend class Tool<TSeq>;
    friend class Agent<TSeq>;
private:
    std::vector< TOOLPTR > * dat;
    const epiworld_fast_uint * n_tools;

public:

    Tools() = delete;
    Tools(Agent<TSeq> & p) : dat(&p.tools), n_tools(&p.n_tools) {};

    typename std::vector< TOOLPTR >::iterator begin();
    typename std::vector< TOOLPTR >::iterator end();

    TOOLPTR & operator()(size_t i);
    TOOLPTR & operator[](size_t i);

    size_t size() const noexcept;

};

template<typename TSeq>
inline typename std::vector< TOOLPTR >::iterator Tools<TSeq>::begin()
{

    if (*n_tools == 0u)
        return dat->end();
    
    return dat->begin();
}

template<typename TSeq>
inline typename std::vector< TOOLPTR >::iterator Tools<TSeq>::end()
{
     
    return begin() + *n_tools;
}

template<typename TSeq>
inline TOOLPTR & Tools<TSeq>::operator()(size_t i)
{

    if (i >= *n_tools)
        throw std::range_error("Tool index out of range.");

    return dat->operator[](i);

}

template<typename TSeq>
inline TOOLPTR & Tools<TSeq>::operator[](size_t i)
{

    return dat->operator[](i);

}

template<typename TSeq>
inline size_t Tools<TSeq>::size() const noexcept 
{
    return *n_tools;
}

/**
 * @brief Set of Tools (const) (useful for iterators)
 * 
 * @tparam TSeq 
 */
template<typename TSeq>
class Tools_const {
    friend class Tool<TSeq>;
    friend class Agent<TSeq>;
private:
    const std::vector< TOOLPTR > * dat;
    const epiworld_fast_uint * n_tools;

public:

    Tools_const() = delete;
    Tools_const(const Agent<TSeq> & p) : dat(&p.tools), n_tools(&p.n_tools) {};

    typename std::vector< TOOLPTR >::const_iterator begin();
    typename std::vector< TOOLPTR >::const_iterator end();

    const TOOLPTR & operator()(size_t i);
    const TOOLPTR & operator[](size_t i);

    size_t size() const noexcept;

};

template<typename TSeq>
inline typename std::vector< TOOLPTR >::const_iterator Tools_const<TSeq>::begin() {

    if (*n_tools == 0u)
        return dat->end();
    
    return dat->begin();
}

template<typename TSeq>
inline typename std::vector< TOOLPTR >::const_iterator Tools_const<TSeq>::end() {
     
    return begin() + *n_tools;
}

template<typename TSeq>
inline const TOOLPTR & Tools_const<TSeq>::operator()(size_t i)
{

    if (i >= *n_tools)
        throw std::range_error("Tool index out of range.");

    return dat->operator[](i);

}

template<typename TSeq>
inline const TOOLPTR & Tools_const<TSeq>::operator[](size_t i)
{

    return dat->operator[](i);

}

template<typename TSeq>
inline size_t Tools_const<TSeq>::size() const noexcept 
{
    return *n_tools;
}

#undef TOOLPTR



#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/tools-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/tool-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



#ifndef EPIWORLD_TOOL_BONES_HPP
#define EPIWORLD_TOOL_BONES_HPP

template<typename TSeq>
class Virus;

template<typename TSeq>
class Agent;

template<typename TSeq>
class Model;

template<typename TSeq>
class Tool;

/**
 * @brief Tools for defending the agent against the virus
 * 
 * @tparam TSeq Type of sequence
 */
template<typename TSeq = int> 
class Tool {
    friend class Agent<TSeq>;
    friend class Model<TSeq>;
    friend void default_add_virus<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_add_tool<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_virus<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_tool<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
private:

    Agent<TSeq> * agent = nullptr;
    int agent_idx        = -99;

    int date = -99;
    int id   = -99;
    std::shared_ptr<std::string> tool_name = nullptr;
    std::shared_ptr<TSeq> sequence = std::make_shared<TSeq>(default_sequence<TSeq>());
    TSeq sequence_unique  = default_sequence<TSeq>();
    ToolFun<TSeq> susceptibility_reduction_fun = nullptr;
    ToolFun<TSeq> transmission_reduction_fun   = nullptr;
    ToolFun<TSeq> recovery_enhancer_fun        = nullptr;
    ToolFun<TSeq> death_reduction_fun          = nullptr;

    // Setup parameters
    std::vector< epiworld_double * > params;  

    epiworld_fast_int status_init = -99;
    epiworld_fast_int status_post = -99;

    epiworld_fast_int queue_init = 0; ///< Change of status when added to agent.
    epiworld_fast_int queue_post = 0; ///< Change of status when removed from agent.

    void set_agent(Agent<TSeq> * p, size_t idx);

public:
    Tool(std::string name = "unknown tool");
    // Tool(TSeq d, std::string name = "unknown tool");

    void set_sequence(TSeq d);
    void set_sequence_unique(TSeq d);
    void set_sequence(std::shared_ptr<TSeq> d);
    std::shared_ptr<TSeq> get_sequence();
    TSeq & get_sequence_unique();

    /**
     * @name Get and set the tool functions
     * 
     * @param v The virus over which to operate
     * @param fun the function to be used
     * 
     * @return epiworld_double 
     */
    ///@{
    epiworld_double get_susceptibility_reduction(VirusPtr<TSeq> v);
    epiworld_double get_transmission_reduction(VirusPtr<TSeq> v);
    epiworld_double get_recovery_enhancer(VirusPtr<TSeq> v);
    epiworld_double get_death_reduction(VirusPtr<TSeq> v);
    
    void set_susceptibility_reduction_fun(ToolFun<TSeq> fun);
    void set_transmission_reduction_fun(ToolFun<TSeq> fun);
    void set_recovery_enhancer_fun(ToolFun<TSeq> fun);
    void set_death_reduction_fun(ToolFun<TSeq> fun);

    void set_susceptibility_reduction(epiworld_double * prob);
    void set_transmission_reduction(epiworld_double * prob);
    void set_recovery_enhancer(epiworld_double * prob);
    void set_death_reduction(epiworld_double * prob);

    void set_susceptibility_reduction(epiworld_double prob);
    void set_transmission_reduction(epiworld_double prob);
    void set_recovery_enhancer(epiworld_double prob);
    void set_death_reduction(epiworld_double prob);
    ///@}

    void set_name(std::string name);
    std::string get_name() const;

    Agent<TSeq> * get_agent();
    int get_id() const;
    void set_id(int id);
    void set_date(int d);
    int get_date() const;

    void set_status(epiworld_fast_int init, epiworld_fast_int post);
    void set_queue(epiworld_fast_int init, epiworld_fast_int post);
    void get_status(epiworld_fast_int * init, epiworld_fast_int * post);
    void get_queue(epiworld_fast_int * init, epiworld_fast_int * post);

};

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/tool-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/tool-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



#ifndef EPIWORLD_TOOLS_MEAT_HPP
#define EPIWORLD_TOOLS_MEAT_HPP

template<typename TSeq>
inline Tool<TSeq>::Tool(std::string name)
{
    set_name(name);
}

// template<typename TSeq>
// inline Tool<TSeq>::Tool(TSeq d, std::string name) {
//     sequence = std::make_shared<TSeq>(d);
//     tool_name = std::make_shared<std::string>(name);
// }

template<typename TSeq>
inline void Tool<TSeq>::set_sequence(TSeq d) {
    sequence = std::make_shared<TSeq>(d);
}

template<typename TSeq>
inline void Tool<TSeq>::set_sequence_unique(TSeq d) {
    sequence_unique = d;
}

template<typename TSeq>
inline void Tool<TSeq>::set_sequence(std::shared_ptr<TSeq> d) {
    sequence = d;
}

template<typename TSeq>
inline std::shared_ptr<TSeq> Tool<TSeq>::get_sequence() {
    return sequence;
}

template<typename TSeq>
inline TSeq & Tool<TSeq>::get_sequence_unique() {
    return sequence_unique;
}

template<typename TSeq>
inline epiworld_double Tool<TSeq>::get_susceptibility_reduction(
    VirusPtr<TSeq> v
)
{

    if (susceptibility_reduction_fun)
        return susceptibility_reduction_fun(*this, this->agent, v, agent->get_model());

    return DEFAULT_TOOL_CONTAGION_REDUCTION;

}

template<typename TSeq>
inline epiworld_double Tool<TSeq>::get_transmission_reduction(
    VirusPtr<TSeq> v
)
{

    if (transmission_reduction_fun)
        return transmission_reduction_fun(*this, this->agent, v, agent->get_model());

    return DEFAULT_TOOL_TRANSMISSION_REDUCTION;

}

template<typename TSeq>
inline epiworld_double Tool<TSeq>::get_recovery_enhancer(
    VirusPtr<TSeq> v
)
{

    if (recovery_enhancer_fun)
        return recovery_enhancer_fun(*this, this->agent, v, agent->get_model());

    return DEFAULT_TOOL_RECOVERY_ENHANCER;

}

template<typename TSeq>
inline epiworld_double Tool<TSeq>::get_death_reduction(
    VirusPtr<TSeq> v
)
{

    if (death_reduction_fun)
        return death_reduction_fun(*this, this->agent, v, agent->get_model());

    return DEFAULT_TOOL_DEATH_REDUCTION;

}

template<typename TSeq>
inline void Tool<TSeq>::set_susceptibility_reduction_fun(
    ToolFun<TSeq> fun
)
{
    susceptibility_reduction_fun = fun;
}

template<typename TSeq>
inline void Tool<TSeq>::set_transmission_reduction_fun(
    ToolFun<TSeq> fun
)
{
    transmission_reduction_fun = fun;
}

template<typename TSeq>
inline void Tool<TSeq>::set_recovery_enhancer_fun(
    ToolFun<TSeq> fun
)
{
    recovery_enhancer_fun = fun;
}

template<typename TSeq>
inline void Tool<TSeq>::set_death_reduction_fun(
    ToolFun<TSeq> fun
)
{
    death_reduction_fun = fun;
}

template<typename TSeq>
inline void Tool<TSeq>::set_susceptibility_reduction(epiworld_double * prob)
{

    ToolFun<TSeq> tmpfun =
        [prob](Tool<TSeq> &  t, Agent<TSeq> * p, VirusPtr<TSeq> v, Model<TSeq> * m)
        {
            return *prob;
        };

    susceptibility_reduction_fun = tmpfun;

}

// EPIWORLD_SET_LAMBDA(susceptibility_reduction)
template<typename TSeq>
inline void Tool<TSeq>::set_transmission_reduction(epiworld_double * prob)
{
    
    ToolFun<TSeq> tmpfun =
        [prob](Tool<TSeq> &  t, Agent<TSeq> * p, VirusPtr<TSeq> v, Model<TSeq> * m)
        {
            return *prob;
        };

    transmission_reduction_fun = tmpfun;

}

// EPIWORLD_SET_LAMBDA(transmission_reduction)
template<typename TSeq>
inline void Tool<TSeq>::set_recovery_enhancer(epiworld_double * prob)
{

    ToolFun<TSeq> tmpfun =
        [prob](Tool<TSeq> & t, Agent<TSeq> * p, VirusPtr<TSeq> v, Model<TSeq> * m)
        {
            return *prob;
        };

    recovery_enhancer_fun = tmpfun;

}

// EPIWORLD_SET_LAMBDA(recovery_enhancer)
template<typename TSeq>
inline void Tool<TSeq>::set_death_reduction(epiworld_double * prob)
{

    ToolFun<TSeq> tmpfun =
        [prob](Tool<TSeq> &  t, Agent<TSeq> * p, VirusPtr<TSeq> v, Model<TSeq> * m)
        {
            return *prob;
        };

    death_reduction_fun = tmpfun;

}

// EPIWORLD_SET_LAMBDA(death_reduction)

// #undef EPIWORLD_SET_LAMBDA
template<typename TSeq>
inline void Tool<TSeq>::set_susceptibility_reduction(
    epiworld_double prob
)
{

    ToolFun<TSeq> tmpfun = 
        [prob](Tool<TSeq> &  t, Agent<TSeq> * p, VirusPtr<TSeq> v, Model<TSeq> * m)
        {
            return prob;
        };

    susceptibility_reduction_fun = tmpfun;

}

template<typename TSeq>
inline void Tool<TSeq>::set_transmission_reduction(
    epiworld_double prob
)
{

    ToolFun<TSeq> tmpfun = 
        [prob](Tool<TSeq> &  t, Agent<TSeq> * p, VirusPtr<TSeq> v, Model<TSeq> * m)
        {
            return prob;
        };

    transmission_reduction_fun = tmpfun;

}

template<typename TSeq>
inline void Tool<TSeq>::set_recovery_enhancer(
    epiworld_double prob
)
{

    ToolFun<TSeq> tmpfun = 
        [prob](Tool<TSeq> &  t, Agent<TSeq> * p, VirusPtr<TSeq> v, Model<TSeq> * m)
        {
            return prob;
        };

    recovery_enhancer_fun = tmpfun;

}

template<typename TSeq>
inline void Tool<TSeq>::set_death_reduction(
    epiworld_double prob
)
{

    ToolFun<TSeq> tmpfun = 
        [prob](Tool<TSeq> & t, Agent<TSeq> * p, VirusPtr<TSeq> v, Model<TSeq> * m)
        {
            return prob;
        };

    death_reduction_fun = tmpfun;

}

template<typename TSeq>
inline void Tool<TSeq>::set_name(std::string name)
{
    if (name != "")
        tool_name = std::make_shared<std::string>(name);
}

template<typename TSeq>
inline std::string Tool<TSeq>::get_name() const {

    if (tool_name)
        return *tool_name;

    return "unknown tool";

}

template<typename TSeq>
inline Agent<TSeq> * Tool<TSeq>::get_agent()
{
    return this->agent;
}

template<typename TSeq>
inline void Tool<TSeq>::set_agent(Agent<TSeq> * p, size_t idx)
{
    agent = p;
    agent_idx = static_cast<int>(idx);
}

template<typename TSeq>
inline int Tool<TSeq>::get_id() const {
    return id;
}


template<typename TSeq>
inline void Tool<TSeq>::set_id(int id)
{
    this->id = id;
}

template<typename TSeq>
inline void Tool<TSeq>::set_date(int d)
{
    this->date = d;
}

template<typename TSeq>
inline int Tool<TSeq>::get_date() const
{
    return date;
}

template<typename TSeq>
inline void Tool<TSeq>::set_status(
    epiworld_fast_int init,
    epiworld_fast_int end
)
{
    status_init = init;
    status_post = end;
}

template<typename TSeq>
inline void Tool<TSeq>::set_queue(
    epiworld_fast_int init,
    epiworld_fast_int end
)
{
    queue_init = init;
    queue_post = end;
}

template<typename TSeq>
inline void Tool<TSeq>::get_status(
    epiworld_fast_int * init,
    epiworld_fast_int * post
)
{
    if (init != nullptr)
        *init = status_init;

    if (post != nullptr)
        *post = status_post;

}

template<typename TSeq>
inline void Tool<TSeq>::get_queue(
    epiworld_fast_int * init,
    epiworld_fast_int * post
)
{
    if (init != nullptr)
        *init = queue_init;

    if (post != nullptr)
        *post = queue_post;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/tool-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/location-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_LOCATION_BONES_HPP
#define EPIWORLD_LOCATION_BONES_HPP

template<typename TSeq>
class Agent;

template<typename TSeq>
class Location {
private:
    
    int capacity = 0;
    std::string location_name = "Unknown Location";

    std::vector< Agent<TSeq> * > agents;
    size_t n_agents = 0u;

    /**
     * @brief Spatial location parameters
     * 
     */
    ///@{
    epiworld_double longitude = 0.0;
    epiworld_double latitude  = 0.0;
    epiworld_double altitude  = 0.0;
    ///@}

public:

    Location() {};

    void add_agent(Agent<TSeq> & p);
    void add_agent(Agent<TSeq> * p);
    void rm_agent(size_t idx);
    size_t size() const noexcept;
    void set_location(int lon, int lat, int alt = 0);

    typename std::vector< Agent<TSeq> * >::iterator begin();
    typename std::vector< Agent<TSeq> * >::iterator end();

};

template<typename TSeq>
inline void Location<TSeq>::add_agent(Agent<TSeq> & p)
{
    if (++n_agents <= agents.size())
        agents.push_back(&p);
    else
        agents[n_agents - 1] = &p;
}

template<typename TSeq>
inline void Location<TSeq>::add_agent(Agent<TSeq> * p)
{
    if (++n_agents <= agents.size())
        agents.push_back(p);
    else
        agents[n_agents - 1] = p;
}

template<typename TSeq>
inline void Location<TSeq>::rm_agent(size_t idx)
{
    if (idx >= n_agents)
        throw std::out_of_range(
            "Trying to remove agent "+ std::to_string(idx) +
            " out of " + std::to_string(n_agents)
            );

    if (--n_agents > 0)
        std::swap(agents[idx], agents[n_agents]);

    return;
}

template<typename TSeq>
inline size_t Location<TSeq>::size() const noexcept
{
    return n_agents;
}

template<typename TSeq>
inline void Location<TSeq>::set_location(int lon, int lat, int alt)
{
    longitude = lon;
    latitude  = lat;
    altitude  = alt;
}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::iterator Location<TSeq>::begin()
{

    if (n_agents == 0)
        return agents.end();

    return agents.begin();

}

template<typename TSeq>
inline typename std::vector< Agent<TSeq> * >::iterator Location<TSeq>::end()
{
    return agents.begin() + n_agents;
}


#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/location-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


    
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/agent-meat-status.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_PERSON_MEAT_STATUS_HPP 
#define EPIWORLD_PERSON_MEAT_STATUS_HPP

template<typename TSeq>
class Model;

template<typename TSeq>
class Agent;

template<typename TSeq = int>
inline void default_update_susceptible(
    Agent<TSeq> * p,
    Model<TSeq> * m
    )
{

    if (p->get_n_viruses() > 0u)
        throw std::logic_error(
            std::string("Using the -default_update_susceptible- on agents WITH viruses makes no sense! ") +
            std::string("Agent id ") + std::to_string(p->get_id()) +
            std::string(" has ") + std::to_string(p->get_n_viruses()) +
            std::string(" viruses.")
            );

    // This computes the prob of getting any neighbor variant
    size_t nvariants_tmp = 0u;
    for (auto & neighbor: p->get_neighbors()) 
    {
                 
        for (const VirusPtr<TSeq> & v : neighbor->get_viruses()) 
        { 

            #ifdef EPI_DEBUG
            if (nvariants_tmp >= m->array_virus_tmp.size())
                throw std::logic_error("Trying to add an extra element to a temporal array outside of the range.");
                // printf_epiworld("N used %d\n", v.use_count());
            #endif
                
            /* And it is a function of susceptibility_reduction as well */ 
            m->array_double_tmp[nvariants_tmp] =
                (1.0 - p->get_susceptibility_reduction(v)) * 
                v->get_prob_infecting() * 
                (1.0 - neighbor->get_transmission_reduction(v)) 
                ; 
        
            m->array_virus_tmp[nvariants_tmp++] = &(*v);
            
        } 
    }

    // No virus to compute
    if (nvariants_tmp == 0u)
        return;

    // Running the roulette
    int which = roulette(nvariants_tmp, m);

    if (which < 0)
        return;

    p->add_virus(*m->array_virus_tmp[which]); 

    return;

}

template<typename TSeq = int>
inline void default_update_exposed(Agent<TSeq> * p, Model<TSeq> * m) {

    if (p->get_n_viruses() == 0u)
        throw std::logic_error(
            std::string("Using the -default_update_exposed- on agents WITHOUT viruses makes no sense! ") +
            std::string("Agent id ") + std::to_string(p->get_id()) + std::string(" has no virus registered.")
            );

    // Odd: Die, Even: Recover
    epiworld_fast_uint n_events = 0u;
    for (const auto & v : p->get_viruses())
    {

        // Die
        m->array_double_tmp[n_events++] = 
            v->get_prob_death() * (1.0 - p->get_death_reduction(v)); 

        // Recover
        m->array_double_tmp[n_events++] = 
            v->get_prob_recovery() * (1.0 - p->get_recovery_enhancer(v)); 

    }
    
    if (n_events == 0u)
        return;

    // Running the roulette
    int which = roulette(n_events, m);

    if (which < 0)
        return;

    // Which roulette happen?
    if ((which % 2) == 0) // If odd
    {

        size_t which_v = std::ceil(which / 2);
        
        // Retrieving the default values of the virus
        epiworld::VirusPtr<TSeq> & v = p->get_viruses()[which_v];
        epiworld_fast_int dead_status, dead_queue;
        v->get_status(nullptr, nullptr, &dead_status);
        v->get_queue(nullptr, nullptr, &dead_queue);

        // Applying change of status
        p->change_status(
            // Either preserve the current status or apply a new one
            (dead_status < 0) ? p->get_status() : static_cast<epiworld_fast_uint>(dead_status),

            // By default, it will be removed from the queue... unless the user
            // says the contrary!
            (dead_queue == -99) ? -m->get_queue()[p->get_id()] : dead_queue
            );

    } else {

        size_t which_v = std::floor(which / 2);
        p->rm_virus(which_v);

    }

    return ;

}

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/agent-meat-status.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/agent-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_PERSON_BONES_HPP
#define EPIWORLD_PERSON_BONES_HPP

template<typename TSeq>
class Virus;

template<typename TSeq>
class Viruses;

template<typename TSeq>
class Viruses_const;

template<typename TSeq>
class Tool;

template<typename TSeq>
class Tools;

template<typename TSeq>
class Tools_const;

template<typename TSeq>
class Queue;

template<typename TSeq>
struct Action;

template<typename TSeq>
inline void default_add_virus(Action<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_add_tool(Action<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_rm_virus(Action<TSeq> & a, Model<TSeq> * m);

template<typename TSeq>
inline void default_rm_tool(Action<TSeq> & a, Model<TSeq> * m);

/**
 * @brief Agent (agents)
 * 
 * @tparam TSeq Sequence type (should match `TSeq` across the model)
 */
template<typename TSeq = int>
class Agent {
    friend class Model<TSeq>;
    friend class Virus<TSeq>;
    friend class Viruses<TSeq>;
    friend class Viruses_const<TSeq>;
    friend class Tool<TSeq>;
    friend class Tools<TSeq>;
    friend class Queue<TSeq>;
    friend void default_add_virus<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_add_tool<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_virus<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
    friend void default_rm_tool<TSeq>(Action<TSeq> & a, Model<TSeq> * m);
private:
    Model<TSeq> * model;
    
    std::vector< Agent<TSeq> * > neighbors;
    std::vector< Location<TSeq> *> locations;

    epiworld_fast_uint status = 0u;
    int id = -1;
    
    bool in_queue = false;
    // size_t actions_queued = 0u;
    
    std::vector< VirusPtr<TSeq> > viruses;
    epiworld_fast_uint n_viruses = 0u;

    std::vector< ToolPtr<TSeq> > tools;
    epiworld_fast_uint n_tools = 0u;

    ActionFun<TSeq> add_virus_ = default_add_virus<TSeq>;
    ActionFun<TSeq> add_tool_  = default_add_tool<TSeq>;
    ActionFun<TSeq> rm_virus_  = default_rm_virus<TSeq>;
    ActionFun<TSeq> rm_tool_   = default_rm_tool<TSeq>;

    epiworld_fast_uint action_counter = 0u;

public:

    Agent();
    Agent(const Agent<TSeq> & p);

    /**
     * @name Add/Remove Virus/Tool
     * 
     * Any of these is ultimately reflected at the end of the iteration.
     * 
     * @param tool Tool to add
     * @param virus Virus to add
     * @param status_new Status after the change
     * @param queue 
     */
    ///@{
    void add_tool(
        ToolPtr<TSeq> tool,
        epiworld_fast_int status_new = -99,
        epiworld_fast_int queue = -99
        );

    void add_tool(
        Tool<TSeq> tool,
        epiworld_fast_int status_new = -99,
        epiworld_fast_int queue = -99
        );

    void add_virus(
        VirusPtr<TSeq> virus,
        epiworld_fast_int status_new = -99,
        epiworld_fast_int queue = -99
        );

    void add_virus(
        Virus<TSeq> virus,
        epiworld_fast_int status_new = -99,
        epiworld_fast_int queue = -99
        );

    void rm_tool(
        epiworld_fast_uint tool_idx,
        epiworld_fast_int status_new = -99,
        epiworld_fast_int queue = -99
    );

    void rm_tool(
        ToolPtr<TSeq> & tool,
        epiworld_fast_int status_new = -99,
        epiworld_fast_int queue = -99
    );

    void rm_virus(
        epiworld_fast_uint virus_idx,
        epiworld_fast_int status_new = -99,
        epiworld_fast_int queue = -99
    );

    void rm_virus(
        VirusPtr<TSeq> & virus,
        epiworld_fast_int status_new = -99,
        epiworld_fast_int queue = -99
    );
    ///@}
    
    /**
     * @name Get the rates (multipliers) for the agent
     * 
     * @param v A pointer to a virus.
     * @return epiworld_double 
     */
    ///@{
    epiworld_double get_susceptibility_reduction(VirusPtr<TSeq> v);
    epiworld_double get_transmission_reduction(VirusPtr<TSeq> v);
    epiworld_double get_recovery_enhancer(VirusPtr<TSeq> v);
    epiworld_double get_death_reduction(VirusPtr<TSeq> v);
    ///@}

    int get_id() const; ///< Id of the individual
    
    std::mt19937 * get_rand_endgine();
    Model<TSeq> * get_model(); 

    VirusPtr<TSeq> & get_virus(int i);
    Viruses<TSeq> get_viruses();
    const Viruses_const<TSeq> get_viruses() const;
    size_t get_n_viruses() const noexcept;

    ToolPtr<TSeq> & get_tool(int i);
    Tools<TSeq> get_tools();
    const Tools_const<TSeq> get_tools() const;
    size_t get_n_tools() const noexcept;

    void mutate_variant();
    void add_neighbor(
        Agent<TSeq> * p,
        bool check_source = true,
        bool check_target = true
        );

    std::vector< Agent<TSeq> * > & get_neighbors();

    void change_status(
        epiworld_fast_uint new_status,
        epiworld_fast_int queue = 0
        );

    const epiworld_fast_uint & get_status() const;

    void reset();

    bool has_tool(unsigned int t) const;
    bool has_tool(std::string name) const;
    bool has_virus(unsigned int t) const;
    bool has_virus(std::string name) const;

};



#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/agent-bones.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 Start of -include/epiworld/agent-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/


#ifndef EPIWORLD_PERSON_MEAT_HPP
#define EPIWORLD_PERSON_MEAT_HPP

// template<typename Ta>
// inline bool IN(Ta & a, std::vector< Ta > & b);

#define CHECK_COALESCE_(proposed_, virus_tool_, alt_) \
    if (static_cast<int>(proposed_) == -99) {\
        if (static_cast<int>(virus_tool_) == -99) \
            (proposed_) = (alt_);\
        else (proposed_) = (virus_tool_);}

template<typename TSeq>
inline void default_add_virus(Action<TSeq> & a, Model<TSeq> * m)
{

    Agent<TSeq> * p = a.agent;
    VirusPtr<TSeq> v = a.virus;

    CHECK_COALESCE_(a.new_status, v->status_init, p->get_status())
    CHECK_COALESCE_(a.queue, v->queue_init, 1)

    // Has a agent? If so, we need to register the transmission
    if (v->get_agent())
    {

        // ... only if not the same agent
        if (v->get_agent()->get_id() != v->get_id())
            m->get_db().record_transmission(
                v->get_agent()->get_id(), p->get_id(), v->get_id() 
            );

    }
    
    // Update virus accounting
    p->n_viruses++;
    size_t n_viruses = p->n_viruses;

    if (n_viruses <= p->viruses.size())
        p->viruses[n_viruses - 1] = std::make_shared< Virus<TSeq> >(*v);
    else
        p->viruses.push_back(std::make_shared< Virus<TSeq> >(*v));

    n_viruses--;

    // Notice that both agent and date can be changed in this case
    // as only the sequence is a shared_ptr itself.
    p->viruses[n_viruses]->set_agent(p, n_viruses);
    p->viruses[n_viruses]->set_date(m->today());

    m->get_db().today_variant[v->get_id()][p->status]++;

}

template<typename TSeq>
inline void default_add_tool(Action<TSeq> & a, Model<TSeq> * m)
{

    Agent<TSeq> * p = a.agent;
    ToolPtr<TSeq> t = a.tool;

    CHECK_COALESCE_(a.new_status, t->status_init, p->get_status())
    CHECK_COALESCE_(a.queue, t->queue_init, 0)
    
    // Update tool accounting
    p->n_tools++;
    size_t n_tools = p->n_tools;

    if (n_tools <= p->tools.size())
        p->tools[n_tools - 1] = std::make_shared< Tool<TSeq> >(*t);
    else
        p->tools.push_back(std::make_shared< Tool<TSeq> >(*t));

    n_tools--;

    p->tools[n_tools]->set_date(m->today());
    p->tools[n_tools]->set_agent(p, n_tools);

    m->get_db().today_tool[t->get_id()][p->status]++;

}

template<typename TSeq>
inline void default_rm_virus(Action<TSeq> & a, Model<TSeq> * m)
{

    Agent<TSeq> * p   = a.agent;    
    VirusPtr<TSeq> & v = a.agent->viruses[a.virus->agent_idx];
    
    CHECK_COALESCE_(a.new_status, v->status_post, p->get_status())
    CHECK_COALESCE_(a.queue, v->queue_post, -1)

    if (--p->n_viruses > 0)
    {
        // The new virus will change positions
        p->viruses[p->n_viruses]->agent_idx = v->agent_idx;
        std::swap(v, p->viruses[p->n_viruses]);
    }
    
    // Calling the virus action over the removed virus
    v->post_recovery();

    return;

}

template<typename TSeq>
inline void default_rm_tool(Action<TSeq> & a, Model<TSeq> * m)
{

    Agent<TSeq> * p  = a.agent;    
    ToolPtr<TSeq> & t = a.agent->tools[a.tool->agent_idx];

    CHECK_COALESCE_(a.new_status, t->status_post, p->get_status())
    CHECK_COALESCE_(a.queue, t->queue_post, 0)

    if (--p->n_tools > 0)
    {
        p->tools[p->n_tools]->agent_idx = t->agent_idx;
        std::swap(t, p->tools[p->n_tools - 1]);
    }

    return;

}

template<typename TSeq>
inline Agent<TSeq>::Agent()
{
    
}

template<typename TSeq>
inline Agent<TSeq>::Agent(const Agent<TSeq> & p)
{

    model = p.model;
    
    // We can't do anything with the neighbors
    neighbors.reserve(p.neighbors.size());

    status = p.status;
    id     = p.id;

    // #ifdef EPI_DEBUG
    // if (id < 0)
    //     throw std::logic_error("Ids in agents cannot be negative.");
    // #endif
    
    in_queue = p.in_queue;

    // Dealing with the virus
    viruses.reserve(p.n_viruses);
    for (auto & v : p.viruses)
    {
        // Will create a copy of the virus, with the exeption of
        // the virus code
        viruses.push_back(std::make_shared<Virus<TSeq>>(*v));
        viruses[n_viruses++]->agent = this;

    }

    n_viruses = p.n_viruses;

    tools.reserve(p.n_tools);
    for (auto & t : p.tools)
    {
        // Will create a copy of the virus, with the exeption of
        // the virus code
        tools.push_back(std::make_shared<Tool<TSeq>>(*t));

    }
    
    n_tools = p.n_tools;

    add_virus_ = p.add_virus_;
    add_tool_  = p.add_tool_;
    rm_virus_  = p.rm_virus_;
    rm_tool_   = p.rm_tool_;
    
}

template<typename TSeq>
inline void Agent<TSeq>::add_tool(
    ToolPtr<TSeq> tool,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
) {

    // Checking the virus exists
    if (tool->get_id() >= static_cast<int>(model->get_db().get_n_tools()))
        throw std::range_error("The tool with id: " + std::to_string(tool->get_id()) + 
            " has not been registered. There are only " + std::to_string(model->get_n_tools()) + 
            " included in the model.");
    

    model->actions_add(
        this, nullptr, tool, status_new, queue, add_tool_
        );

}

template<typename TSeq>
inline void Agent<TSeq>::add_tool(
    Tool<TSeq> tool,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
)
{
    ToolPtr<TSeq> tool_ptr = std::make_shared< Tool<TSeq> >(tool);
    add_tool(tool_ptr, status_new, queue);
}

template<typename TSeq>
inline void Agent<TSeq>::add_virus(
    VirusPtr<TSeq> virus,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
)
{

    // Checking the virus exists
    if (virus->get_id() >= static_cast<int>(model->get_db().get_n_variants()))
        throw std::range_error("The virus with id: " + std::to_string(virus->get_id()) + 
            " has not been registered. There are only " + std::to_string(model->get_n_variants()) + 
            " included in the model.");

    model->actions_add(
        this, virus, nullptr, status_new, queue, add_virus_
        );

}

template<typename TSeq>
inline void Agent<TSeq>::add_virus(
    Virus<TSeq> virus,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
)
{
    VirusPtr<TSeq> virus_ptr = std::make_shared< Virus<TSeq> >(virus);
    add_virus(virus_ptr, status_new, queue);
}

template<typename TSeq>
inline void Agent<TSeq>::rm_tool(
    epiworld_fast_uint tool_idx,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
)
{

    if (tool_idx >= n_tools)
        throw std::range_error(
            "The Tool you want to remove is out of range. This Agent only has " +
            std::to_string(n_tools) + " tools."
        );

    model->actions_add(
        this, nullptr, tools[tool_idx] , status_new, queue, rm_tool_
        );

}

template<typename TSeq>
inline void Agent<TSeq>::rm_tool(
    ToolPtr<TSeq> & tool,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
)
{

    if (tool->agent != this)
        throw std::logic_error("Cannot remove a virus from another agent!");

    model->actions_add(
        this, tool, nullptr , status_new, queue, rm_tool_
        );

}

template<typename TSeq>
inline void Agent<TSeq>::rm_virus(
    epiworld_fast_uint virus_idx,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
)
{
    if (virus_idx >= n_viruses)
        throw std::range_error(
            "The Virus you want to remove is out of range. This Agent only has " +
            std::to_string(n_viruses) + " viruses."
        );
    else if (n_viruses == 0u)
        throw std::logic_error(
            "There is no virus to remove here!"
        );


    model->actions_add(
        this, viruses[virus_idx], nullptr, status_new, queue, default_rm_virus<TSeq>
        );
    
}

template<typename TSeq>
inline void Agent<TSeq>::rm_virus(
    VirusPtr<TSeq> & virus,
    epiworld_fast_int status_new,
    epiworld_fast_int queue
)
{

    if (virus->agent != this)
        throw std::logic_error("Cannot remove a virus from another agent!");

    model->actions_add(
        this, virus, nullptr, status_new, queue, default_rm_virus<TSeq>
        );


}

template<typename TSeq>
inline epiworld_double Agent<TSeq>::get_susceptibility_reduction(
    VirusPtr<TSeq> v
) {

    return model->susceptibility_reduction_mixer(this, v, model);
}

template<typename TSeq>
inline epiworld_double Agent<TSeq>::get_transmission_reduction(
    VirusPtr<TSeq> v
) {
    return model->transmission_reduction_mixer(this, v, model);
}

template<typename TSeq>
inline epiworld_double Agent<TSeq>::get_recovery_enhancer(
    VirusPtr<TSeq> v
) {
    return model->recovery_enhancer_mixer(this, v, model);
}

template<typename TSeq>
inline epiworld_double Agent<TSeq>::get_death_reduction(
    VirusPtr<TSeq> v
) {
    return model->death_reduction_mixer(this, v, model);
}

template<typename TSeq>
inline int Agent<TSeq>::get_id() const
{
    return id;
}

template<typename TSeq>
inline std::mt19937 * Agent<TSeq>::get_rand_endgine() {
    return model->get_rand_endgine();
}

template<typename TSeq>
inline Model<TSeq> * Agent<TSeq>::get_model() {
    return model;
}

template<typename TSeq>
inline Viruses<TSeq> Agent<TSeq>::get_viruses() {

    return Viruses<TSeq>(*this);

}

template<typename TSeq>
inline const Viruses_const<TSeq> Agent<TSeq>::get_viruses() const {

    return Viruses_const<TSeq>(*this);
    
}

template<typename TSeq>
inline VirusPtr<TSeq> & Agent<TSeq>::get_virus(int i) {
    return viruses.at(i);
}

template<typename TSeq>
inline size_t Agent<TSeq>::get_n_viruses() const noexcept
{
    return n_viruses;
}

template<typename TSeq>
inline Tools<TSeq> Agent<TSeq>::get_tools() {
    return Tools<TSeq>(*this);
}

template<typename TSeq>
inline const Tools_const<TSeq> Agent<TSeq>::get_tools() const {
    return Tools_const<TSeq>(*this);
}

template<typename TSeq>
inline ToolPtr<TSeq> & Agent<TSeq>::get_tool(int i)
{
    return tools.at(i);
}

template<typename TSeq>
inline size_t Agent<TSeq>::get_n_tools() const noexcept
{
    return n_tools;
}

template<typename TSeq>
inline void Agent<TSeq>::mutate_variant()
{

    for (auto & v : viruses)
        v->mutate();

}

template<typename TSeq>
inline void Agent<TSeq>::add_neighbor(
    Agent<TSeq> * p,
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
inline std::vector< Agent<TSeq> *> & Agent<TSeq>::get_neighbors()
{
    return neighbors;
}

template<typename TSeq>
inline void Agent<TSeq>::change_status(
    epiworld_fast_uint new_status,
    epiworld_fast_int queue
    )
{

    model->actions_add(
        this, nullptr, nullptr, new_status, queue, nullptr
    );
    
    return;

}

template<typename TSeq>
inline const epiworld_fast_uint & Agent<TSeq>::get_status() const {
    return status;
}

template<typename TSeq>
inline void Agent<TSeq>::reset()
{

    this->viruses.clear();
    n_viruses = 0u;

    this->tools.clear();
    n_tools = 0u;

    this->status = 0u;
    
}

template<typename TSeq>
inline bool Agent<TSeq>::has_tool(unsigned int t) const
{

    for (auto & tool : tools)
        if (tool->get_id() == t)
            return true;

    return false;

}

template<typename TSeq>
inline bool Agent<TSeq>::has_tool(std::string name) const
{

    for (auto & tool : tools)
        if (tool->get_name() == name)
            return true;

    return false;

}

template<typename TSeq>
inline bool Agent<TSeq>::has_virus(unsigned int t) const
{
    for (auto & v : viruses)
        if (v->get_id() == t)
            return true;

    return false;
}

template<typename TSeq>
inline bool Agent<TSeq>::has_virus(std::string name) const
{
    
    for (auto & v : viruses)
        if (v->get_name() == name)
            return true;

    return false;

}

#undef CHECK_COALESCE_

#endif
/*//////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

 End of -include/epiworld/agent-meat.hpp-

////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////*/



}

#endif 
