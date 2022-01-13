#ifndef EPIWORLD_MACROS_HPP
#define EPIWORLD_MACROS_HPP


/**
 * @brief Helper macro to define a new tool
 * 
 */
#define EPI_NEW_TOOL(fname,tseq) inline epiworld_double \
(fname)(\
    epiworld::Tool< tseq > * t, \
    epiworld::Person< tseq > * p, \
    epiworld::Virus< tseq > * v, \
    epiworld::Model< tseq > * m\
    )

/**
 * @brief Create a Tool within a function
 * 
 */
#define EPI_NEW_TOOL_LAMBDA(funname,tseq) \
    epiworld::ToolFun<tseq> funname = \
    [](epiworld::Tool<tseq> * t, \
    epiworld::Person<tseq> * p, \
    epiworld::Virus<tseq> * v, \
    epiworld::Model<tseq> * m)

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
    epiworld::Person<tseq> * p, \
    epiworld::Virus<tseq> * v, \
    epiworld::Model<tseq> * m )

#define EPI_NEW_MUTFUN_LAMBDA(funname,tseq) \
    epiworld::MutFun<tseq> funname = \
    [](epiworld::Person<tseq> * p, \
    epiworld::Virus<tseq> * v, \
    epiworld::Model<tseq> * m)

#define EPI_NEW_POSTRECOVERYFUN(funname,tseq) inline void \
    (funname)( \
    epiworld::Person<tseq> * p, \
    epiworld::Virus<tseq>* v, \
    epiworld::Model<tseq> * m\
    )

#define EPI_NEW_POSTRECOVERYFUN_LAMBDA(funname,tseq) \
    epiworld::PostRecoveryFun<tseq> funname = \
    [](epiworld::Person<tseq> * p, \
    epiworld::Virus<tseq>* v, \
    epiworld::Model<tseq> * m)

#define EPI_NEW_VIRUSFUN(funname,tseq) inline void \
    (funname)( \
    epiworld::Person<tseq> * p, \
    epiworld::Virus<tseq>* v, \
    epiworld::Model<tseq> * m\
    )

#define EPI_NEW_VIRUSFUN_LAMBDA(funname,tseq) \
    epiworld::VirusFun<tseq> funname = \
    [](epiworld::Person<tseq> * p, \
    epiworld::Virus<tseq>* v, \
    epiworld::Model<tseq> * m)

#define EPI_RUNIF() m->runif()

#define EPIWORLD_RUN(a) \
    if (a.get_verbose()) \
    { \
        printf_epiworld("Running the model...\n");\
    } \
    for (unsigned int niter = 0; niter < a.get_ndays(); ++niter)

#define EPI_TOKENPASTE(a,b) a ## b
#define MPAR(num) *(m->EPI_TOKENPASTE(p,num))

#define EPI_NEW_UPDATEFUN(funname,tseq) inline epiworld_fast_uint \
    (funname)(epiworld::Person<tseq> * p, epiworld::Model<tseq> * m)

#define EPI_NEW_UPDATEFUN_LAMBDA(funname,tseq) inline epiworld_fast_uint \
    epiworld::UpdateFun<tseq> funname = \
    [](epiworld::Person<tseq> * p, epiworld::Model<tseq> * m)

#define EPI_NEW_GLOBALFUN(funname,tseq) inline void \
    (funname)(epiworld::Model<tseq>* m)

#define EPI_NEW_GLOBALFUN_LAMBDA(funname,tseq) inline void \
    epiworld::GlobalFun<tseq> funname = \
    [](epiworld::Model<tseq>* m)

#endif