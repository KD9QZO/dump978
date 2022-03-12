/* Initialize a RS codec
 *
 * Copyright 2002 Phil Karn, KA9Q
 * May be used under the terms of the GNU Lesser General Public License (LGPL)
 */
#include <stdlib.h>

#include "char.h"
#include "rs-common.h"



void free_rs_char(void *p) {
	struct rs *rs = (struct rs*)p;

	free(rs->alpha_to);
	free(rs->index_of);
	free(rs->genpoly);
	free(rs);
}

/**
 * \brief Initialize a Reed-Solomon codec
 *
 * \param[in]	symsize		Symbol size, in bits
 * \param[in]	gfpoly		Field generator polynomial coefficients
 * \param[in]	fcr			First root of RS code generator polynomial, index form
 * \param[in]	prim		Primitive element to generate polynomial roots
 * \param[in]	nroots		RS code generator polynomial degree (number of roots)
 * \param[in]	pad			Padding bytes at front of shortened block
 */
void *init_rs_char(int symsize, int gfpoly, int fcr, int prim, int nroots, int pad) {
	struct rs *rs;

#include "init_rs.h"

	return (rs);
}
