/* -*- mode:C++; tab-width:8; c-basic-offset:8; indent-tabs-mode:true -*- */
/*
 * written by Jason Clinton <me@jasonclinton.com>
 * inspired by the cache code writen by Neil Roberts for Aiselriot
 *
 * Copyright (C) 2005 by Callum McKenzie
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * For more details see the file COPYING.
 */

#ifndef __blockcache_h__
#define __blockcache_h__

#include <cogl/cogl.h>

class GnometrisBlockCache
{
public:
	GnometrisBlockCache (int);
	~GnometrisBlockCache ();
	CoglHandle get_block_by_id (int);
	CoglHandle get_block_by_color (int);

private:
	int slots;

};
#endif //__blockcache_h__