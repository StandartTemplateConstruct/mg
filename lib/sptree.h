/**************************************************************************
 *
 * sptree.h -- Splay tree code
 * Copyright (C) 1994  Gary Eddy and Neil Sharman
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
 * $Id: sptree.h,v 1.2 1994/09/20 04:20:09 tes Exp $
 *
 **************************************************************************/


#ifndef SPTREE_H
#define SPTREE_H

typedef struct set_rec Splay_Tree;

struct set_rec
  {
    int no_of_items;
    unsigned long mem_in_use;
    int (*cmp) (void *, void *);
    void *root;
    void *pool;
    int pool_chunks;
  };

Splay_Tree *SP_createset (int (*cmp) (void *, void *));
void *SP_insert (void *item, Splay_Tree * S);
void *SP_get_first (Splay_Tree * S);
void *SP_get_next (Splay_Tree * S);
void *SP_deletemin (Splay_Tree * S);
void *SP_member (void *dat, Splay_Tree * S);
void SP_freeset (Splay_Tree * S);
#endif
