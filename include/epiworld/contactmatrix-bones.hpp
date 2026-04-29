#ifndef EPIWORLD_CONTACTMATRIX_BONES_HPP
#define EPIWORLD_CONTACTMATRIX_BONES_HPP

#include <vector>

class ContactMatrix 
{
private: 

    std::vector< double > contact_matrix;
    int n_groups = -1;

public:
    /**
     * @brief Set the contact matrix for population mixing
     * @param cmat Contact matrix specifying interaction rates between groups
     */
    void set_contact_matrix(std::vector< double > cmat);

    /**
     * @brief Get the current contact matrix
     * @return Vector representing the contact matrix
     */
    const std::vector< double > & get_contact_matrix() const;

    /**
     * @brief Get the current contact matrix
     * @return Vector representing the contact matrix
     */
    std::vector< double > & get_contact_matrix_ref();

    /**
     * @brief Get the contact rate between two groups
     * @param i Index of the first group
     * @param j Index of the second group
     * @return Contact rate between group i and group j
     */
    double get_contact_rate(size_t i, size_t j, bool check = true) const;

    size_t get_contact_matrix_size() const
    {
        return contact_matrix.size();
    }
};

#endif