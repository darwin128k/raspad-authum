/**
 * @file fields.h
 * @brief Member fields of ::authum_loader_t.
 */

#ifndef AUTHUM_LOADER_FIELDS_H
#define AUTHUM_LOADER_FIELDS_H

/**
 * @def authum_loader_fields(vector_type)
 * @brief Injects the module table (::lh_vector_t of ::authum_module_t).
 *
 * @param vector_type Type of the table (::lh_vector_t).
 */
#define authum_loader_fields(vector_type) vector_type modules

#endif /* AUTHUM_LOADER_FIELDS_H */
