#ifndef EPIWORLD_CONTACTMATRIX_MEAT_HPP
#define EPIWORLD_CONTACTMATRIX_MEAT_HPP

#include "contactmatrix-bones.hpp"
#include <stdexcept>

inline void ContactMatrix::set_contact_matrix(std::vector< double > cmat)
{
    n_groups = static_cast<int>(std::sqrt(cmat.size()));
    if (n_groups * n_groups != static_cast<int>(cmat.size()))
        throw std::invalid_argument(
            "Contact matrix size is not a perfect square, cannot determine number of groups."
        );

    contact_matrix = cmat;
    return;
};

inline const std::vector< double > & ContactMatrix::get_contact_matrix() const
{
    return contact_matrix;
}

inline std::vector< double > & ContactMatrix::get_contact_matrix_ref()
{
    return contact_matrix;
}

inline double ContactMatrix::get_contact_rate(
    size_t i, size_t j, bool check
) const
{
    if (check && (
        (static_cast<int>(i) >= n_groups) ||
        (static_cast<int>(j) >= n_groups)
    ))
        throw std::out_of_range(
            std::string("Group indices out of range. ") +
            std::to_string(i) + ", " + std::to_string(j) +
            std::string(" >= ") + std::to_string(n_groups)
        );
    return contact_matrix[j * n_groups + i];
}

#endif