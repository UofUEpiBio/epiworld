#ifndef EPIWORLD_MISC_HPP
#define EPIWORLD_MISC_HPP

#include <stdexcept>
#include <string>
#include <vector>
#include <cmath>
#include <map>
#include <regex>
#include <fstream>

#include <epiworld/config.hpp>
#include <epiworld/model-bones.hpp>
#include <epiworld/agent-bones.hpp>

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
    ///@brief Operator to hash a vector.
    std::size_t operator()(std::vector< T > const&  dat) const noexcept {
        const auto hash_magic = 0x9e3779b9;
        const auto hash_lshift = 6;
        const auto hash_rshift = 2;

        std::hash< T > hasher;
        std::size_t hash = hasher(dat[0]);

        // ^ makes bitwise XOR
        // 0x9e3779b9 is a 32 bit constant (comes from the golden ratio)
        // << is a shift operator, something like lhs * 2^(rhs)
        if (dat.size() > 1) {
            for (epiworld_fast_uint i = 1; i < dat.size(); ++i) {
                hash ^= hasher(dat[i]) + hash_magic + (hash<<hash_lshift) + (hash>>hash_rshift);
            }
        }

        return hash;
    }
};

template<typename Ta = epiworld_double, typename Tb = epiworld_fast_uint>
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
template<typename TSeq = EPI_DEFAULT_TSEQ>
inline TSeq default_sequence(int seq_count);

// Making it 'static' so that we don't have problems when including the
// header. This is important during the linkage, e.g., in R.
// See https://en.cppreference.com/w/cpp/language/storage_duration#Linkage
// static int _n_sequences_created = 0;

template<>
inline bool default_sequence(int seq_count) {

    if (seq_count == 2) {
        throw std::logic_error("Maximum number of sequence created.");
    }

    return seq_count++ == 0;
}

template<>
inline int default_sequence(int seq_count) {
    return seq_count++;
}

template<>
inline epiworld_double default_sequence(int seq_count) {
    return static_cast<epiworld_double>(seq_count++);
}

template<>
inline std::vector<bool> default_sequence(int seq_count) {

    if (seq_count == 2) {
        throw std::logic_error("Maximum number of sequence created.");
    }

    return {seq_count++ == 0};
}

template<>
inline std::vector<int> default_sequence(int seq_count) {
    return {seq_count++};
}

template<>
inline std::vector<epiworld_double> default_sequence(int seq_count) {
    return {static_cast<epiworld_double>(seq_count++)};
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
inline bool IN(const Ta & sub, const std::vector< Ta > & par) noexcept
{
    return std::find(par.begin(), par.end(), sub) != par.end();
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
template<typename TSeq = EPI_DEFAULT_TSEQ, typename TDbl = epiworld_double >
inline int roulette(
    const std::vector< TDbl > & probs,
    Model<TSeq> * model
    )
{

    // Step 1: Computing the prob on none
    TDbl p_none = 1.0;
    const TDbl epsilon = 1e-100; // TODO: This seems really small.
    std::vector< epiworld_fast_uint > certain_infection;
    certain_infection.reserve(probs.size());

    for (epiworld_fast_uint prob = 0; prob < probs.size(); ++prob)
    {
        p_none *= (1.0 - probs[prob]);

        if (probs[prob] > (1 - epsilon)) {
            certain_infection.push_back(prob);
        }

    }

    TDbl coin = static_cast<TDbl>(model->runif());
    // If there are one or more probs that go close to 1, sample
    // uniformly
    if (certain_infection.size() > 0) {
        return certain_infection[std::floor(coin * certain_infection.size())];
    }

    // Step 2: Calculating the prob of none or single
    std::vector< TDbl > probs_only_p(probs.size());
    TDbl p_none_or_single = p_none;

    for (epiworld_fast_uint prob = 0; prob < probs.size(); ++prob)
    {
        probs_only_p[prob] = probs[prob] * (p_none / (1.0 - probs[prob]));
        p_none_or_single += probs_only_p[prob];
    }

    // Step 3: Roulette
    TDbl cumsum = p_none/p_none_or_single;
    if (coin < cumsum)
    {
        return -1;
    }

    for (epiworld_fast_uint prob = 0; prob < probs.size(); ++prob)
    {
        // If it yield here, then bingo, the individual will acquire the disease
        cumsum += probs_only_p[prob]/(p_none_or_single);
        if (coin < cumsum) {
            return static_cast<int>(prob);
        }
    }


    #ifdef EPI_DEBUG
    printf_epiworld("[epi-debug] roulette::cumsum = %.4f\n", cumsum);
    #endif

    return static_cast<int>(probs.size() - 1);

}

template<typename TSeq>
inline int roulette(std::vector< double > & probs, Model<TSeq> * model)
{
    return roulette<TSeq, double>(probs, model);
}

template<typename TSeq>
inline int roulette(std::vector< float > & probs, Model<TSeq> * model)
{
    return roulette<TSeq, float>(probs, model);
}


template<typename TSeq>
inline int roulette(
    epiworld_fast_uint nelements,
    Model<TSeq> * model
    )
{

    if ((nelements * 2) > model->array_double_tmp.size())
    {
        throw std::logic_error(
            "Trying to sample from more data than there is in roulette!" +
            std::to_string(nelements) + " vs " +
            std::to_string(model->array_double_tmp.size())
        );
    }

    // Step 1: Computing the prob on none
    epiworld_double p_none = 1.0;
    epiworld_fast_uint ncertain = 0;
    const epiworld_double epsilon = 1e-100; // TODO: This seems really small.

    // std::vector< int > certain_infection;
    for (epiworld_fast_uint param = 0; param < nelements; ++param)
    {
        p_none *= (1.0 - model->array_double_tmp[param]);

        if (model->array_double_tmp[param] > (1 - epsilon)) {
            model->array_double_tmp[nelements + ncertain++] = param;
        }

    }

    epiworld_double coin = model->runif();
    // If there are one or more probs that go close to 1, sample
    // uniformly
    if (ncertain > 0) {
        return model->array_double_tmp[nelements + static_cast<epiworld_fast_uint>(static_cast<epiworld_double>(ncertain) * coin)]; //    certain_infection[std::floor(r * certain_infection.size())];
    }

    // Step 2: Calculating the prob of none or single
    // std::vector< epiworld_double > probs_only_p;
    epiworld_double p_none_or_single = p_none;
    for (epiworld_fast_uint param = 0; param < nelements; ++param)
    {
        model->array_double_tmp[nelements + param] =
            model->array_double_tmp[param] * (p_none / (1.0 - model->array_double_tmp[param]));

        p_none_or_single += model->array_double_tmp[nelements + param];
    }

    // Step 3: Roulette
    epiworld_double cumsum = p_none/p_none_or_single;
    if (coin < cumsum) {
        return -1;
    }

    for (epiworld_fast_uint param = 0; param < nelements; ++param)
    {
        // If it yield here, then bingo, the individual will acquire the disease
        cumsum += model->array_double_tmp[nelements + param]/(p_none_or_single);
        if (coin < cumsum) {
            return static_cast<int>(param);
        }

    }

    return static_cast<int>(nelements - 1);

}

/**
 * @brief Read parameters from a yaml file
 *
 * @details
 * The file should have the following structure:
 * ```yaml
 * # Comment
 * [name of parameter 1]: [value in T]
 * [name of parameter 2]: [value in T]
 * ...
 * ```
 *
 * @tparam T Type of the parameter
 * @param source Path to the file containing the parameters
 * @return std::map<std::string, T>
 */
template <typename T>
inline std::map< std::string, T > read_yaml(std::string const& source)
{
    std::ifstream paramsfile(source);

    if (!paramsfile) {
        throw std::logic_error("The file " + source + " was not found.");
    }

    std::regex pattern(R"(^([^:]+)\s*[:]\s*([-]?[0-9]+|[-]?[0-9]*\.[0-9]+)?\s*$)");

    std::string line;
    std::smatch match;
    auto empty = std::sregex_iterator();

    // Making room
    std::map<std::string, T> parameters;

    while (std::getline(paramsfile, line))
    {

        // Is it a comment or an empty line?
        if (std::regex_match(line, std::regex("^([*].+|//.+|#.+|\\s*)$"))) {
            continue;
        }

        // Finding the pattern, if it doesn't match, then error
        std::regex_match(line, match, pattern);

        if (match.empty()) {
            throw std::logic_error("Line has invalid format:\n" + line);
        }

        // Capturing the number
        std::string anumber = match[2].str() + match[3].str();
        T tmp_num = static_cast<T>(
            std::strtod(anumber.c_str(), nullptr)
            );

        std::string pname = std::regex_replace(
            match[1].str(),
            std::regex("^\\s+|\\s+$"),
            "");

        // Adding the parameter to the map
        parameters[pname] = tmp_num;

    }

    return parameters;

}

#endif
