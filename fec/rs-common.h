/**
 * \file rs-common.h
 * \brief Stuffcommon to all the general-purpose Reed-Solomon codecs
 *
 * \version		0.0.1
 * \author		Gerad Munsch [KD9QZO] <gmunsch@kd9qzo.com>
 * \date		March 2022
 * \author		Phil Karn [KA9Q]
 * \date		2004
 *
 * \copyright Copyright (c) 2022 Gerad Munsch [KD9QZO] \n
 *            Copyright (c) 2004 Phil Karn [KA9Q]
 *
 * \par License
 * May be used under the terms of the GNU Lesser General Public License (LGPL).
 */

#ifndef FEC_RS_COMMON_H_
#define FEC_RS_COMMON_H_


/**
 * \defgroup fec_rs_common FEC Reed-Solomon codec common stuff
 *
 * @{
 */


/*! \brief Reed-Solomon codec control block */
struct rs {
	int mm;				/*!< \f$ BITS_{SYMBOL} \f$ -- bits per symbol */
	int nn;				/*!< \f$ SYM_{BLOCK} = (1 \ll mm) - 1 \f$ -- Symbols per block */
	data_t *alpha_to;	/*!< log lookup table */
	data_t *index_of;	/*!< Antilog lookup table */
	data_t *genpoly;	/*!< Generator polynomial */
	int nroots;			/*!< \f$ QTY_{GENROOTS} = QTY_{PARSYMS} \f$ -- Number of generator roots = number of parity symbols */
	int fcr;			/*!< First consecutive root, index form */
	int prim;			/*!< Primitive element, index form */
	int iprim;			/*!< Prim-th root of 1, index form */
	int pad;			/*!< Padding bytes in shortened block */
};



static inline int modnn(struct rs *rs, int x) {
	while (x >= rs->nn) {
		x -= rs->nn;
		x = (x >> rs->mm) + (x & rs->nn);
	}

	return (x);
}


/**
 * @}
 */


#endif	/* !FEC_RS_COMMON_H_ */
