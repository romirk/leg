//
// Created by Romir Kulshrestha on 07/05/2026.
//

#pragma once

#include "dev/mmu.h"

// create a new PGD
pgd_t *pgd_new(void);

// clone an existing PGD
pgd_t *pgd_clone(pgd_t *parent);

// map one user VA → fresh PA, allocating a backing page
void *pgd_map_user_page(pgd_t *pgd, void *va);

// PGD destructor
void pgd_free(pgd_t *pgd);
