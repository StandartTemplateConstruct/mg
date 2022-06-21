/**************************************************************************
 *
 * weights.h -- Functions for reading the weights file in mgquery
 * Copyright (C) 1994  Neil Sharman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: weights.h 16583 2008-07-29 10:20:36Z davidb $
 *
 **************************************************************************/

#ifndef H_WEIGHTS
#define H_WEIGHTS

approx_weights_data *LoadDocWeights (File * weight_file,
				     unsigned long num_of_docs);

float GetLowerApproxDocWeight (approx_weights_data * awd, register int DocNum);

void FreeWeights (approx_weights_data * awd);

#endif