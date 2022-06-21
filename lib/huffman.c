/**************************************************************************
 *
 * huffman.c -- Huffman coding functions
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
 * $Id: huffman.c,v 1.1 1994/08/22 00:24:44 tes Exp $
 *
 **************************************************************************/

/*
   $Log: huffman.c,v $
   * Revision 1.1  1994/08/22  00:24:44  tes
   * Initial placement under CVS.
   *
 */

static char *RCSID = "$Id: huffman.c,v 1.1 1994/08/22 00:24:44 tes Exp $";

#include "sysfuncs.h"
#include "memlib.h"
#include "huffman.h"
#include "messages.h"

/* ->data on success,  
   NULL on error 
 */
huff_data *
Generate_Huffman_Data (int num, long *freqs, huff_data * data,
		       u_long * mem)
{
  int HNum, i, count;
  unsigned long *heap;
  huff_data *hd = data;
  if (!hd)
    {
      if (!(hd = Xmalloc (sizeof (huff_data))))
	goto error0;
    }
  if (!(heap = Xmalloc (num * 2 * sizeof (*heap))))
    goto error1;
  if (!(hd->clens = Xmalloc (num * sizeof (char))))
      goto error2;
  hd->num_codes = num;
  if (mem)
    {
      if (hd != data)
	*mem += sizeof (huff_data);
      *mem += num * sizeof (char);
    }
  bcopy ((char *) freqs, (char *) &heap[num], num * sizeof (*heap));
  bzero ((char *) heap, num * sizeof (*heap));

  /* Initialise the pointers to the leaves */
  for (count = i = 0; i < num; i++)
    if (heap[num + i])
      heap[count++] = num + i;

  /* Reorganise the pointers so that it is a heap */
  HNum = count;
  for (i = HNum / 2; i > 0; i--)
    {
      register int curr, child;
      curr = i;
      child = curr * 2;
      while (child <= HNum)
	{
	  if (child < HNum && heap[heap[child]] < heap[heap[child - 1]])
	    child++;
	  if (heap[heap[curr - 1]] > heap[heap[child - 1]])
	    {
	      register u_long temp;
	      temp = heap[child - 1];
	      heap[child - 1] = heap[curr - 1];
	      heap[curr - 1] = temp;
	      curr = child;
	      child = 2 * curr;
	    }
	  else
	    break;
	}
    }


  /* Form the huffman tree */
  while (HNum > 1)
    {
      int pos[2];

      for (i = 0; i < 2; i++)
	{
	  register int curr, child;
	  pos[i] = heap[0];
	  heap[0] = heap[--HNum];
	  curr = 1;
	  child = 2;
	  while (child <= HNum)
	    {
	      if (child < HNum &&
		  heap[heap[child]] < heap[heap[child - 1]])
		child++;
	      if (heap[heap[curr - 1]] > heap[heap[child - 1]])
		{
		  register int temp;
		  temp = heap[child - 1];
		  heap[child - 1] = heap[curr - 1];
		  heap[curr - 1] = temp;
		  curr = child;
		  child = 2 * curr;
		}
	      else
		break;
	    }
	}

      heap[HNum + 1] = heap[pos[0]] + heap[pos[1]];
      heap[pos[0]] = heap[pos[1]] = HNum + 1;

      heap[HNum] = HNum + 1;

      {
	register int parent, curr;
	HNum++;
	curr = HNum;
	parent = curr >> 1;
	while (parent && heap[heap[parent - 1]] > heap[heap[curr - 1]])
	  {
	    register int temp;
	    temp = heap[parent - 1];
	    heap[parent - 1] = heap[curr - 1];
	    heap[curr - 1] = temp;
	    curr = parent;
	    parent = curr >> 1;
	  }
      }
    }


  /* Calculate the code lens */
  heap[0] = -1UL;
  heap[1] = 0;
  for (i = 2; i < num * 2; i++)
    heap[i] = heap[heap[i]] + 1;

  /* zero the lencount's array */
  bzero ((char *) hd->lencount, sizeof (hd->lencount));
  hd->maxcodelen = 0;
  hd->mincodelen = 100;

  /* Set the code length of each leaf in the huffman tree */
  for (i = 0; i < num; i++)
    {
      register u_long codelen = heap[i + num];
      hd->clens[i] = (char) codelen;
      if (!codelen)
	continue;
      if (codelen > hd->maxcodelen)
	hd->maxcodelen = codelen;
      if (codelen < hd->mincodelen)
	hd->mincodelen = codelen;
      hd->lencount[codelen]++;
    }


  if (hd->maxcodelen == 0 && hd->mincodelen == 100)
    {
      hd->num_codes = 0;
    }
  else
    {
      /* Calculate the current codes for each different code length */
      hd->min_code[hd->maxcodelen] = 0;
      for (i = hd->maxcodelen - 1; i>=0; i--)
	hd->min_code[i] = (hd->min_code[i + 1] + hd->lencount[i + 1]) >> 1;
    }
  free (heap);
  return (hd);

error2:
  free (heap);
error1:
  if (!data)
    free (hd);
error0:
  return (NULL);

}

unsigned long *
Generate_Huffman_Codes (huff_data * data, u_long * mem)
{
  int i;
  unsigned long *codes;
  unsigned long mc[MAX_HUFFCODE_LEN + 1];

  if (!data)
    return (NULL);
  if (!(codes = Xmalloc (data->num_codes * sizeof (*codes))))
    return (NULL);
  if (mem)
    *mem += data->num_codes * sizeof (*codes);
  bcopy ((char *) data->min_code, (char *) mc, sizeof (mc));
  for (i = 0; i < data->num_codes; i++)
    if (data->clens[i])
      codes[i] = mc[(int) (data->clens[i])]++;

  return (codes);
}


unsigned long **
Generate_Huffman_Vals (huff_data * data, u_long * mem)
{
  int i;
  unsigned long *fcode[MAX_HUFFCODE_LEN + 1];
  unsigned long **values;
  unsigned long *vals;

  if (!data)
    return (NULL);
  if (!(vals = Xmalloc (data->num_codes * sizeof (*vals))))
    return (NULL);
  if (!(values = Xmalloc ((MAX_HUFFCODE_LEN + 1) * sizeof (unsigned long *))))
    {
      free (vals);
      return (NULL);
    }

  bzero ((char *) values, (MAX_HUFFCODE_LEN + 1) * sizeof (unsigned long *));

  if (mem)
    *mem += data->num_codes * sizeof (*vals) +
      (MAX_HUFFCODE_LEN + 1) * sizeof (unsigned long *);

  fcode[0] = values[0] = &vals[0];
  for (i = 1; i <= data->maxcodelen; i++)
    fcode[i] = values[i] = &vals[(values[i - 1] - vals) + data->lencount[i - 1]];

  for (i = 0; i < data->num_codes; i++)
    if (data->clens[i])
      *fcode[(int) (data->clens[i])]++ = i;
  return (values);
}



double 
Calculate_Huffman_Size (int num, long *freqs, long *counts)
{
  double size = 0;
  huff_data hd;
  int i;
  if (!Generate_Huffman_Data (num, freqs, &hd, NULL))
    return -1;
  for (i = 0; i < num; i++)
    size += counts[i] * hd.clens[i];
  Xfree (hd.clens);
  return size;
}



int 
Write_Huffman_Data (FILE * f, huff_data * hd)
{
  if (fwrite (&hd->num_codes, sizeof (hd->num_codes), 1, f) != 1)
    return -1;

  if (fwrite (&hd->mincodelen, sizeof (hd->mincodelen), 1, f) != 1)
    return -1;

  if (fwrite (&hd->maxcodelen, sizeof (hd->maxcodelen), 1, f) != 1)
    return -1;

  if (hd->num_codes)
    {
      if (fwrite (hd->lencount + hd->mincodelen, sizeof (*hd->lencount),
		  hd->maxcodelen - hd->mincodelen + 1, f) !=
	  hd->maxcodelen - hd->mincodelen + 1)
	return -1;


      if (fwrite (hd->min_code, sizeof (*hd->min_code), hd->maxcodelen + 1, f) !=
	  hd->maxcodelen + 1)
	return -1;

      if (fwrite (hd->clens, sizeof (*hd->clens), hd->num_codes, f) != hd->num_codes)
	return -1;
    }
  return 0;
}




static int 
General_Read_Huffman_Data (size_t (*rd) (), void *f,
			   huff_data * hd, u_long * mem, u_long * disk)
{
  if (rd (&hd->num_codes, sizeof (hd->num_codes), 1, f) != 1)
    return -1;

  if (rd (&hd->mincodelen, sizeof (hd->mincodelen), 1, f) != 1)
    return -1;

  if (rd (&hd->maxcodelen, sizeof (hd->maxcodelen), 1, f) != 1)
    return -1;

  if (disk)
    (*disk) += sizeof (hd->num_codes) + sizeof (hd->mincodelen) +
      sizeof (hd->maxcodelen);

  hd->clens = NULL;

  if (hd->num_codes)
    {
      bzero ((char *) (hd->lencount), sizeof (hd->lencount));
      if (rd (hd->lencount + hd->mincodelen, sizeof (*hd->lencount),
	      hd->maxcodelen - hd->mincodelen + 1, f) !=
	  hd->maxcodelen - hd->mincodelen + 1)
	return -1;

      if (disk)
	(*disk) += sizeof (*hd->lencount) * (hd->maxcodelen - hd->mincodelen + 1);

      bzero ((char *) (hd->min_code), sizeof (hd->min_code));

      if (rd (hd->min_code, sizeof (*hd->min_code), hd->maxcodelen + 1, f) !=
	  hd->maxcodelen + 1)
	return -1;

      if (disk)
	(*disk) += sizeof (*hd->min_code) * (hd->maxcodelen + 1);

      if (!(hd->clens = Xmalloc (sizeof (*hd->clens) * hd->num_codes)))
	return -1;


      if (rd (hd->clens, sizeof (*hd->clens), hd->num_codes, f) != hd->num_codes)
	return -1;

      if (disk)
	(*disk) += sizeof (*hd->clens) * hd->num_codes;
    }

  if (mem)
    *mem += sizeof (*hd->clens) * hd->num_codes;
  return 0;
}



int 
Read_Huffman_Data (FILE * f, huff_data * hd, u_long * mem, u_long * disk)
{
  return General_Read_Huffman_Data (fread, f, hd, mem, disk);
}




int 
F_Read_Huffman_Data (File * f, huff_data * hd, u_long * mem, u_long * disk)
{
  return General_Read_Huffman_Data (Fread, f, hd, mem, disk);
}
