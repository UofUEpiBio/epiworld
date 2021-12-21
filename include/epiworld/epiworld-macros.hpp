#ifndef EPIWORLD_MACROS_HPP
#define EPIWORLD_MACROS_HPP


/**
 * @brief Helper macro to define a new tool
 * 
 */
#define EPI_NEW_TOOL(fname,tseq) inline double \
(fname)(\
    epiworld::Tool< tseq > * t, \
    epiworld::Person< tseq > * p, \
    epiworld::Virus< tseq > * v, \
    epiworld::Model< tseq > * m\
    )

/**
 * @brief Helper macro for accessing model parameters
 * 
 */
#define EPI_PARAMS(i) m->operator()(i)

/**
 * @brief Helper macro for defining Mutation Functions
 * 
 */
#define EPI_MUTFUN(fname,tseq) inline bool \
    (fname)(\
    epiworld::Virus<tseq> * v, \
    epiworld::Model<tseq> * m \
    )

#define EPI_RUNIF() m->runif()

#endif