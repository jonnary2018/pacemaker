/*
 * cib_secrets.h
 *
 * Author: Dejan Muhamedagic <dejan@suse.de>
 * Copyright (c) 2011 SUSE, Attachmate
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef CIB_SECRETS__H
#define CIB_SECRETS__H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * load parameters from an ini file (cib_secrets.c)
 */
int replace_secret_params(char * rsc_id, GHashTable * params);

#ifdef __cplusplus
}
#endif

#endif
