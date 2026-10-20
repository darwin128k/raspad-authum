/**
 * @file capacity.h
 * @brief Storage size for ::authum_path_t.
 *
 * Sized for absolute HLDS paths on one volume (game / addons / module / leaf).
 * Not a growable string — if a path does not fit, setters/joins fail cleanly.
 */

#ifndef AUTHUM_PATH_CAPACITY_H
#define AUTHUM_PATH_CAPACITY_H

enum
{
    authum_path_capacity = 512
};

#endif /* AUTHUM_PATH_CAPACITY_H */
