/*  Boolector: Satisfiablity Modulo Theories (SMT) solver.
 *  Copyright (C) 2010  Robert Daniel Brummayer, Armin Biere
 *
 *  This file is part of Boolector.
 *
 *  Boolector is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Boolector is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "btorconst.h"
#include "btormem.h"
#include "btorstack.h"
#include "btorutil.h"

#include <assert.h>
#include <limits.h>
#include <string.h>

#define BTOR_NOT_CONST_3VL(a) ((a) == 'x' ? 'x' : (a) ^ 1)
#define BTOR_AND_CONST_3VL(a, b) \
  (((a) == '0' || (b) == '0')    \
       ? '0'                     \
       : (((a) == 'x' || (b) == 'x') ? 'x' : (a) & (b)))
#define BTOR_OR_CONST_3VL(a, b) \
  (((a) == '1' || (b) == '1')   \
       ? '1'                    \
       : (((a) == 'x' || (b) == 'x') ? 'x' : (a) | (b)))
#define BTOR_XOR_CONST_3VL(a, b) (((a) == 'x' || (b) == 'x') ? 'x' : (a) ^ (b))

static const char *digit2const_table[10] = {
    "",
    "1",
    "10",
    "11",
    "100",
    "101",
    "110",
    "111",
    "1000",
    "1001",
};

static int
is_valid_const (const char *c)
{
  const char *p;
  char ch;

  assert (c != NULL);

  for (p = c; (ch = *p); p++)
    if (ch != '0' && ch != '1') return 0;
  return 1;
}

#ifndef NDEBUG

static int
is_valid_const_3vl (const char *c)
{
  const char *p;
  char ch;

  assert (c != NULL);

  for (p = c; (ch = *p); p++)
    if (ch != '0' && ch != '1' && ch != 'x') return 0;
  return 1;
}

#endif

static const char *
digit2const (char ch)
{
  assert ('0' <= ch);
  assert (ch <= '9');

  return digit2const_table[ch - '0'];
}

static const char *
strip_zeroes (const char *a)
{
  assert (a != NULL);
  assert (is_valid_const (a));

  while (*a == '0') a++;

  return a;
}

#define MSB_INT ((int) (sizeof (int) * 8 - 1))

char *
btor_zero_const (BtorMemMgr *mm, int len)
{
  char *res;
  int i;

  assert (len > 0);

  BTOR_NEWN (mm, res, len + 1);
  for (i = 0; i < len; i++) res[i] = '0';
  res[i] = '\0';

  return res;
}

char *
btor_one_const (BtorMemMgr *mm, int len)
{
  char *res;
  int i;

  assert (mm != NULL);
  assert (len > 0);

  BTOR_NEWN (mm, res, len + 1);
  for (i = 0; i < len - 1; i++) res[i] = '0';
  res[i++] = '1';
  res[i]   = '\0';

  return res;
}

char *
btor_ones_const (BtorMemMgr *mm, int len)
{
  char *res;
  int i;

  assert (mm != NULL);
  assert (len > 0);

  BTOR_NEWN (mm, res, len + 1);
  for (i = 0; i < len; i++) res[i] = '1';
  res[i] = '\0';

  return res;
}

char *
btor_int_to_const (BtorMemMgr *mm, int x, int len)
{
  char msb, *result;
  int i;

  assert (mm != NULL);
  assert (len > 0);

  BTOR_NEWN (mm, result, len + 1);

  msb = (x & (1 << MSB_INT)) ? '1' : '0';
  for (i = len - 1; i >= MSB_INT; i--) result[len - 1 - i] = msb;

  while (i >= 0)
  {
    result[len - 1 - i] = (x & (1 << i)) ? '1' : '0';
    i--;
  }

  result[len] = '\0';

  return result;
}

#define MSB_UNSIGNED ((int) (sizeof (unsigned) * 8 - 1))

char *
btor_unsigned_to_const (BtorMemMgr *mm, unsigned x, int len)
{
  char *result;
  int i;

  assert (mm != NULL);
  assert (len > 0);

  BTOR_NEWN (mm, result, len + 1);

  for (i = len - 1; i > MSB_UNSIGNED; i--) result[len - 1 - i] = '0';

  while (i >= 0)
  {
    result[len - 1 - i] = (x & (1u << i)) ? '1' : '0';
    i--;
  }

  result[len] = '\0';

  return result;
}

char *
btor_decimal_to_const_n (BtorMemMgr *mem, const char *str, int len)
{
  const char *end, *p;
  char *res, *tmp;

  assert (mem != NULL);
  assert (str != NULL);
  assert (len >= 0);

  res = btor_strdup (mem, "");

  end = str + len;
  for (p = str; p < end; p++)
  {
    tmp = btor_mult_unbounded_const (mem, res, "1010"); /* *10 */
    btor_delete_const (mem, res);
    res = tmp;

    tmp = btor_add_unbounded_const (mem, res, digit2const (*p));
    btor_delete_const (mem, res);
    res = tmp;
  }

  assert (strip_zeroes (res) == res);

  return res;
}

char *
btor_decimal_to_const (BtorMemMgr *mem, const char *str)
{
  assert (mem != NULL);
  assert (str != NULL);

  return btor_decimal_to_const_n (mem, str, (int) strlen (str));
}

char *
btor_hex_to_const_n (BtorMemMgr *mem, const char *str, int hlen)
{
  const char *p, *end;
  char *tmp, *res, *q;
  int len;

  assert (mem != NULL);
  assert (str != NULL);
  assert (hlen >= 0);

  len = 4 * hlen;
  BTOR_NEWN (mem, tmp, len + 1);
  q = tmp;

  end = str + hlen;
  for (p = str; p < end; p++) switch (*p)
    {
      case '0':
        *q++ = '0';
        *q++ = '0';
        *q++ = '0';
        *q++ = '0';
        break;
      case '1':
        *q++ = '0';
        *q++ = '0';
        *q++ = '0';
        *q++ = '1';
        break;
      case '2':
        *q++ = '0';
        *q++ = '0';
        *q++ = '1';
        *q++ = '0';
        break;
      case '3':
        *q++ = '0';
        *q++ = '0';
        *q++ = '1';
        *q++ = '1';
        break;
      case '4':
        *q++ = '0';
        *q++ = '1';
        *q++ = '0';
        *q++ = '0';
        break;
      case '5':
        *q++ = '0';
        *q++ = '1';
        *q++ = '0';
        *q++ = '1';
        break;
      case '6':
        *q++ = '0';
        *q++ = '1';
        *q++ = '1';
        *q++ = '0';
        break;
      case '7':
        *q++ = '0';
        *q++ = '1';
        *q++ = '1';
        *q++ = '1';
        break;
      case '8':
        *q++ = '1';
        *q++ = '0';
        *q++ = '0';
        *q++ = '0';
        break;
      case '9':
        *q++ = '1';
        *q++ = '0';
        *q++ = '0';
        *q++ = '1';
        break;
      case 'A':
      case 'a':
        *q++ = '1';
        *q++ = '0';
        *q++ = '1';
        *q++ = '0';
        break;
      case 'B':
      case 'b':
        *q++ = '1';
        *q++ = '0';
        *q++ = '1';
        *q++ = '1';
        break;
      case 'C':
      case 'c':
        *q++ = '1';
        *q++ = '1';
        *q++ = '0';
        *q++ = '0';
        break;
      case 'D':
      case 'd':
        *q++ = '1';
        *q++ = '1';
        *q++ = '0';
        *q++ = '1';
        break;
      case 'E':
      case 'e':
        *q++ = '1';
        *q++ = '1';
        *q++ = '1';
        *q++ = '0';
        break;
      case 'F':
      case 'f':
      default:
        assert (*p == 'f' || *p == 'F');
        *q++ = '1';
        *q++ = '1';
        *q++ = '1';
        *q++ = '1';
        break;
    }

  assert (tmp + len == q);
  *q++ = 0;

  res = btor_strdup (mem, strip_zeroes (tmp));
  btor_freestr (mem, tmp);

  return res;
}

char *
btor_hex_to_const (BtorMemMgr *mem, const char *str)
{
  assert (mem != NULL);
  assert (str != NULL);

  return btor_hex_to_const_n (mem, str, (int) strlen (str));
}

char *
btor_ground_const_3vl (BtorMemMgr *mm, const char *c)
{
  char *res, *q;
  const char *p;
  char ch;

  assert (mm != NULL);
  assert (c != NULL);
  assert ((int) strlen (c) > 0);
  assert (is_valid_const_3vl (c));

  BTOR_NEWN (mm, res, (int) strlen (c) + 1);

  q = res;
  for (p = c; (ch = *p); p++) *q++ = (ch == '1') ? '1' : '0'; /* 'x' -> '0' */

  *q = 0;

  return res;
}

int
btor_is_zero_const (const char *str)
{
  const char *p;

  assert (str != NULL);
  assert ((int) strlen (str) > 0);
  assert (is_valid_const (str));

  for (p = str; *p; p++)
    if (*p != '0') return 0;
  return 1;
}

int
btor_is_one_const (const char *str)
{
  int len, i;

  assert (str != NULL);
  assert ((int) strlen (str) > 0);
  assert (is_valid_const (str));

  len = (int) strlen (str);
  if (str[len - 1] != '1') return 0;
  for (i = 0; i < len - 1; i++)
    if (str[i] != '0') return 0;
  return 1;
}

int
btor_is_ones_const (const char *str)
{
  const char *p;

  assert (str != NULL);
  assert ((int) strlen (str) > 0);
  assert (is_valid_const (str));

  for (p = str; *p; p++)
    if (*p != '1') return 0;
  return 1;
}

BtorSpecialConst
btor_is_special_const (const char *str)
{
  char c;
  const char *p;

  assert (str != NULL);
  assert ((int) strlen (str) > 0);
  assert (is_valid_const (str));

  c = *str;
  p = str + 1;

  while (*p)
  {
    if (*p != c)
    {
      p++;
      if (c == '0' && !*p)
        return BTOR_SPECIAL_CONST_ONE;
      else
        return BTOR_SPECIAL_CONST_NONE;
    }
    else
      p++;
  }

  if (c == '0') return BTOR_SPECIAL_CONST_ZERO;

  assert (c == '1');
  /* bit-width == 1 ? */
  if (p == str + 1)
  {
    assert ((int) strlen (str) == 1);
    return BTOR_SPECIAL_CONST_ONE_ONES;
  }
  return BTOR_SPECIAL_CONST_ONES;
}

char *
btor_copy_const (BtorMemMgr *mm, const char *c)
{
  assert (mm != NULL);
  assert (c != NULL);
  assert (is_valid_const_3vl (c));

  return btor_strdup (mm, c);
}

void
btor_delete_const (BtorMemMgr *mm, char *c)
{
  assert (mm != NULL);
  assert (c != NULL);
  assert (is_valid_const_3vl (c));

  btor_freestr (mm, c);
}

int
btor_get_num_leading_zeros_const (BtorMemMgr *mm, const char *c)
{
  const char *p;
  int result;

  assert (mm != NULL);
  assert (c != NULL);
  assert (*c != '\0');
  assert (is_valid_const_3vl (c));
  (void) mm;

  result = 0;
  for (p = c; *p == '0'; p++) result++;

  return result;
}

int
btor_get_num_leading_ones_const (BtorMemMgr *mm, const char *c)
{
  const char *p;
  int result;

  assert (mm != NULL);
  assert (c != NULL);
  assert (*c != '\0');
  assert (is_valid_const_3vl (c));
  (void) mm;

  result = 0;
  for (p = c; *p == '1'; p++) result++;

  return result;
}

static char *
slice_const (BtorMemMgr *mm, const char *a, int upper, int lower)
{
  const char *p, *eoa;
  char *res, *q;
  int len, delta;

  assert (mm != NULL);
  assert (a != NULL);
  assert (upper < (int) strlen (a));
  assert (upper >= lower);
  assert (lower >= 0);
  assert (is_valid_const_3vl (a));

  len   = (int) strlen (a);
  delta = upper - lower + 1;

  BTOR_NEWN (mm, res, delta + 1);

  p   = a + len - 1 - upper;
  q   = res;
  eoa = a + len - 1 - lower;

  while (p <= eoa) *q++ = *p++;

  *q = 0;

  assert ((int) strlen (res) == delta);

  return res;
}

char *
btor_slice_const (BtorMemMgr *mm, const char *a, int upper, int lower)
{
  assert (mm != NULL);
  assert (a != NULL);
  assert (upper < (int) strlen (a));
  assert (upper >= lower);
  assert (lower >= 0);
  assert (is_valid_const (a));

  return slice_const (mm, a, upper, lower);
}

char *
btor_add_unbounded_const (BtorMemMgr *mm, const char *a, const char *b)
{
  char *res, *r, c, x, y, s, *tmp;
  int alen, blen, rlen;
  const char *p, *q;

  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (is_valid_const (a));
  assert (is_valid_const (b));

  a = strip_zeroes (a);
  b = strip_zeroes (b);

  if (!*a) return btor_strdup (mm, b);

  if (!*b) return btor_strdup (mm, a);

  alen = (int) strlen (a);
  blen = (int) strlen (b);
  rlen = (alen < blen) ? blen : alen;
  rlen++;

  BTOR_NEWN (mm, res, rlen + 1);

  p = a + alen;
  q = b + blen;

  c = '0';

  r  = res + rlen;
  *r = 0;

  while (res < r)
  {
    x    = (a < p) ? *--p : '0';
    y    = (b < q) ? *--q : '0';
    s    = x ^ y ^ c;
    c    = (x & y) | (x & c) | (y & c);
    *--r = s;
  }

  p = strip_zeroes (res);
  if ((p != res))
  {
    tmp = btor_copy_const (mm, p);
    btor_delete_const (mm, res);
    res = tmp;
  }

  return res;
}

char *
btor_mult_unbounded_const (BtorMemMgr *mm, const char *a, const char *b)
{
  char *res, *r, c, x, y, s, m;
  int alen, blen, rlen, i;
  const char *p;

  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (is_valid_const (a));
  assert (is_valid_const (b));

  a = strip_zeroes (a);

  if (!*a) return btor_strdup (mm, "");

  if (a[0] == '1' && !a[1]) return btor_strdup (mm, b);

  b = strip_zeroes (b);

  if (!*b) return btor_strdup (mm, "");

  if (b[0] == '1' && !b[1]) return btor_strdup (mm, a);

  alen = (int) strlen (a);
  blen = (int) strlen (b);
  rlen = alen + blen;
  BTOR_NEWN (mm, res, rlen + 1);
  res[rlen] = 0;

  for (r = res; r < res + blen; r++) *r = '0';

  for (p = a; p < a + alen; p++) *r++ = *p;

  assert (r == res + rlen);

  for (i = 0; i < alen; i++)
  {
    m = res[rlen - 1];
    c = '0';

    if (m == '1')
    {
      p = b + blen;
      r = res + blen;

      while (res < r && b < p)
      {
        assert (b < p);
        x  = *--p;
        y  = *--r;
        s  = x ^ y ^ c;
        c  = (x & y) | (x & c) | (y & c);
        *r = s;
      }
    }

    memmove (res + 1, res, rlen - 1);
    res[0] = c;
  }

  return res;
}

int
btor_cmp_const (const char *a, const char *b)
{
  const char *p, *q, *s;
  int l, k, delta;

  assert (a != NULL);
  assert (b != NULL);
  assert (is_valid_const (a));
  assert (is_valid_const (b));

  a = strip_zeroes (a);
  b = strip_zeroes (b);

  l = (int) strlen (a);
  k = (int) strlen (b);

  delta = (l - k);

  if (delta < 0)
  {
    p = a;
    s = b - delta;

    for (q = b; q < s; q++)
      if (*q == '1') return -1;
  }
  else
  {
    s = a + delta;
    q = b;

    for (p = a; p < s; p++)
      if (*p == '1') return 1;
  }

  assert (strlen (p) == strlen (q));

  return strcmp (p, q);
}

char *
btor_sub_unbounded_const (BtorMemMgr *mem, const char *a, const char *b)
{
  char *res, *tmp, *r, c, x, y, s;
  int alen, blen, rlen;
  const char *p, *q;

  assert (mem != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (is_valid_const (a));
  assert (is_valid_const (b));
  assert (btor_cmp_const (b, a) <= 0);

  a = strip_zeroes (a);
  b = strip_zeroes (b);
  if (!*b) return btor_strdup (mem, a);

  alen = (int) strlen (a);
  blen = (int) strlen (b);

  assert (alen >= blen);
  rlen = alen;

  BTOR_NEWN (mem, res, rlen + 1);

  p = a + alen;
  q = b + blen;

  c  = '0';
  r  = res + rlen;
  *r = 0;

  while (res < r)
  {
    assert (a < p);
    x = *--p;

    y = (b < q) ? *--q : '0';

    s = x ^ y ^ c;
    c = ((1 ^ x) & c) | ((1 ^ x) & y) | (y & c);

    *--r = s;
  }

  assert (c == '0');

#ifndef NDEBUG
  {
    tmp = btor_add_unbounded_const (mem, res, b);
    assert (!btor_cmp_const (tmp, a));
    btor_freestr (mem, tmp);
  }
#endif

  tmp = btor_strdup (mem, strip_zeroes (res));
  btor_freestr (mem, res);
  res = tmp;

  return res;
}

static void
invert_const (BtorMemMgr *mm, char *a)
{
  int len, i;
  assert (mm != NULL);
  assert (a != NULL);
  assert ((int) strlen (a) > 0);
  assert (is_valid_const_3vl (a));
  (void) mm;

  len = (int) strlen (a);
  for (i = 0; i < len; i++)
    if (a[i] != 'x') a[i] = (char) (1 ^ a[i]);
}

void
btor_invert_const (BtorMemMgr *mm, char *a)
{
  assert (mm != NULL);
  assert (a != NULL);
  assert ((int) strlen (a) > 0);
  assert (is_valid_const (a));
  invert_const (mm, a);
}

static char *
not_const (BtorMemMgr *mm, const char *a)
{
  char *result;
  int len, i;

  assert (mm != NULL);
  assert (a != NULL);
  assert ((int) strlen (a) > 0);
  assert (is_valid_const_3vl (a));

  len = (int) strlen (a);
  BTOR_NEWN (mm, result, len + 1);

  for (i = 0; i < len; i++) result[i] = BTOR_NOT_CONST_3VL (a[i]);
  result[len] = '\0';
  return result;
}

char *
btor_not_const (BtorMemMgr *mm, const char *a)
{
  assert (mm != NULL);
  assert (a != NULL);
  assert ((int) strlen (a) > 0);
  assert (is_valid_const (a));

  return not_const (mm, a);
}

static char *
and_const (BtorMemMgr *mm, const char *a, const char *b)
{
  char *result;
  int len, i;

  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (strlen (a) == strlen (b));
  assert ((int) strlen (a) > 0);
  assert (is_valid_const_3vl (a));
  assert (is_valid_const_3vl (b));

  len = (int) strlen (a);
  BTOR_NEWN (mm, result, len + 1);
  for (i = len - 1; i >= 0; i--) result[i] = BTOR_AND_CONST_3VL (a[i], b[i]);
  result[len] = '\0';
  return result;
}

char *
btor_and_const (BtorMemMgr *mm, const char *a, const char *b)
{
  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (strlen (a) == strlen (b));
  assert ((int) strlen (a) > 0);
  assert (is_valid_const (a));
  assert (is_valid_const (b));

  return and_const (mm, a, b);
}

static char *
eq_const (BtorMemMgr *mm, const char *a, const char *b)
{
  char *result;
  int len, i, has_x;

  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (strlen (a) == strlen (b));
  assert ((int) strlen (a) > 0);
  assert (is_valid_const_3vl (a));
  assert (is_valid_const_3vl (b));

  len = (int) strlen (a);
  BTOR_NEWN (mm, result, 2);
  result[0] = '1';
  has_x     = 0;

  for (i = len - 1; i >= 0; i--)
  {
    if (a[i] == 'x' || b[i] == 'x')
      has_x = 1;
    else if (a[i] != b[i])
    {
      result[0] = '0';
      break;
    }
  }
  result[1] = '\0';

  if (result[0] == '1' && has_x) result[0] = 'x';

  return result;
}

char *
btor_eq_const (BtorMemMgr *mm, const char *a, const char *b)
{
  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (strlen (a) == strlen (b));
  assert ((int) strlen (a) > 0);
  assert (is_valid_const (a));
  assert (is_valid_const (b));

  return eq_const (mm, a, b);
}

static char *
add_const (BtorMemMgr *mm, const char *a, const char *b)
{
  char carry, p0, p1, p2, *result;
  int len, i;

  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (strlen (a) == strlen (b));
  assert ((int) strlen (a) > 0);
  assert (is_valid_const_3vl (a));
  assert (is_valid_const_3vl (b));

  carry = '0';
  len   = (int) strlen (a);
  BTOR_NEWN (mm, result, len + 1);
  for (i = len - 1; i >= 0; i--)
  {
    result[i] = BTOR_XOR_CONST_3VL (BTOR_XOR_CONST_3VL (a[i], b[i]), carry);
    p0        = BTOR_AND_CONST_3VL (a[i], b[i]);
    p1        = BTOR_AND_CONST_3VL (a[i], carry);
    p2        = BTOR_AND_CONST_3VL (b[i], carry);
    carry     = BTOR_OR_CONST_3VL (BTOR_OR_CONST_3VL (p0, p1), p2);
  }
  result[len] = '\0';
  return result;
}

char *
btor_add_const (BtorMemMgr *mm, const char *a, const char *b)
{
  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (strlen (a) == strlen (b));
  assert ((int) strlen (a) > 0);
  assert (is_valid_const (a));
  assert (is_valid_const (b));
  return add_const (mm, a, b);
}

char *
btor_neg_const (BtorMemMgr *mm, const char *a)
{
  char *result, *not_a, *one;
  int len;

  assert (mm != NULL);
  assert (a != NULL);
  assert ((int) strlen (a) > 0);
  assert (is_valid_const (a));

  len    = (int) strlen (a);
  not_a  = btor_not_const (mm, a);
  one    = btor_int_to_const (mm, 1, len);
  result = btor_add_const (mm, not_a, one);
  btor_delete_const (mm, not_a);
  btor_delete_const (mm, one);
  return result;
}

char *
btor_sub_const (BtorMemMgr *mm, const char *a, const char *b)
{
  char *result, *neg_b;

  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (strlen (a) == strlen (b));
  assert ((int) strlen (a) > 0);
  assert (is_valid_const (a));
  assert (is_valid_const (b));

  neg_b  = btor_neg_const (mm, b);
  result = btor_add_const (mm, a, neg_b);
  btor_delete_const (mm, neg_b);
  return result;
}

static char *
btor_sll_n_bits_const (BtorMemMgr *mm, const char *a, int n)
{
  char *result;
  int len, i;

  assert (mm != NULL);
  assert (a != NULL);
  assert (is_valid_const_3vl (a));
  assert (n >= 0);
  assert (n < (int) strlen (a));

  len = (int) strlen (a);
  if (len == 0) return btor_strdup (mm, a);
  BTOR_NEWN (mm, result, len + 1);
  for (i = 0; i < len - n; i++) result[i] = a[i + n];
  for (i = len - n; i < len; i++) result[i] = '0';
  result[len] = '\0';
  return result;
}

static char *
mul_const (BtorMemMgr *mm, const char *a, const char *b)
{
  char *result, *and, *add, *shift;
  int i, j, len;

  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (strlen (a) == strlen (b));
  assert ((int) strlen (a) > 0);
  assert (is_valid_const_3vl (a));
  assert (is_valid_const_3vl (b));

  len    = (int) strlen (a);
  result = btor_int_to_const (mm, 0, len);
  for (i = len - 1; i >= 0; i--)
  {
    BTOR_NEWN (mm, and, len + 1);
    for (j = 0; j < len; j++) and[j] = BTOR_AND_CONST_3VL (a[j], b[i]);
    and[len] = '\0';
    shift    = btor_sll_n_bits_const (mm, and, len - 1 - i);
    add      = add_const (mm, result, shift);
    btor_delete_const (mm, result);
    btor_delete_const (mm, and);
    btor_delete_const (mm, shift);
    result = add;
  }
  return result;
}

char *
btor_mul_const (BtorMemMgr *mm, const char *a, const char *b)
{
  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (strlen (a) == strlen (b));
  assert ((int) strlen (a) > 0);
  assert (is_valid_const (a));
  assert (is_valid_const (b));
  return mul_const (mm, a, b);
}

static char *
btor_srl_n_bits_const (BtorMemMgr *mm, const char *a, int n)
{
  char *result;
  int len, i;

  assert (mm != NULL);
  assert (a != NULL);
  assert (is_valid_const_3vl (a));
  assert (n >= 0);
  assert (n < (int) strlen (a));

  len = (int) strlen (a);
  if (len == 0) return btor_strdup (mm, a);
  BTOR_NEWN (mm, result, len + 1);
  for (i = 0; i < n; i++) result[i] = '0';
  for (i = n; i < len; i++) result[i] = a[i - n];
  result[len] = '\0';
  return result;
}

static void
btor_SC_GATE_CO_const (char *CO, char R, char D, char CI)
{
  char D_or_CI, D_and_CI, M;
  D_or_CI  = BTOR_OR_CONST_3VL (D, CI);
  D_and_CI = BTOR_AND_CONST_3VL (D, CI);
  M        = BTOR_AND_CONST_3VL (D_or_CI, R);
  *CO      = BTOR_OR_CONST_3VL (M, D_and_CI);
}

static void
btor_SC_GATE_S_const (char *S, char R, char D, char CI, char Q)
{
  char D_and_CI, D_or_CI;
  char T2_or_R, T2_and_R;
  char T1, T2;
  D_or_CI  = BTOR_OR_CONST_3VL (D, CI);
  D_and_CI = BTOR_AND_CONST_3VL (D, CI);
  T1       = BTOR_AND_CONST_3VL (D_or_CI, BTOR_NOT_CONST_3VL (D_and_CI));
  T2       = BTOR_AND_CONST_3VL (T1, Q);
  T2_or_R  = BTOR_OR_CONST_3VL (T2, R);
  T2_and_R = BTOR_AND_CONST_3VL (T2, R);
  *S       = BTOR_AND_CONST_3VL (T2_or_R, BTOR_NOT_CONST_3VL (T2_and_R));
}

static void
udiv_urem_const (
    BtorMemMgr *mm, const char *Ain, const char *Din, char **Qptr, char **Rptr)
{
  char *A, *nD, **S, **C;
  char *Q, *R;
  int size, i, j;

  assert (mm != NULL);
  assert (Ain != NULL);
  assert (Din != NULL);
  assert (Qptr != NULL);
  assert (Rptr != NULL);
  assert (is_valid_const_3vl (Ain));
  assert (is_valid_const_3vl (Din));
  assert (strlen (Ain) == strlen (Din));

  size = (int) strlen (Ain);
  assert (size > 0);

  BTOR_NEWN (mm, A, size);
  for (i = 0; i < size; i++) A[i] = Ain[size - 1 - i];

  BTOR_NEWN (mm, nD, size);
  for (i = 0; i < size; i++) nD[i] = BTOR_NOT_CONST_3VL (Din[size - 1 - i]);

  BTOR_NEWN (mm, S, size + 1);
  for (j = 0; j <= size; j++)
  {
    BTOR_NEWN (mm, S[j], size + 1);
    for (i = 0; i <= size; i++) S[j][i] = '0';
  }

  BTOR_NEWN (mm, C, size + 1);
  for (j = 0; j <= size; j++)
  {
    BTOR_NEWN (mm, C[j], size + 1);
    for (i = 0; i <= size; i++) C[j][i] = '0';
  }

  BTOR_NEWN (mm, R, size + 1);
  BTOR_NEWN (mm, Q, size + 1);
  R[size] = '\0';
  Q[size] = '\0';

  for (j = 0; j <= size - 1; j++)
  {
    S[j][0] = A[size - j - 1];
    C[j][0] = '1';

    for (i = 0; i <= size - 1; i++)
      btor_SC_GATE_CO_const (&C[j][i + 1], S[j][i], nD[i], C[j][i]);

    Q[j] = BTOR_OR_CONST_3VL (C[j][size], S[j][size]);

    for (i = 0; i <= size - 1; i++)
      btor_SC_GATE_S_const (&S[j + 1][i + 1], S[j][i], nD[i], C[j][i], Q[j]);
  }

  for (i = size; i >= 1; i--) R[size - i] = S[size][i];

  for (j = 0; j <= size; j++) BTOR_DELETEN (mm, C[j], size + 1);
  BTOR_DELETEN (mm, C, size + 1);

  for (j = 0; j <= size; j++) BTOR_DELETEN (mm, S[j], size + 1);
  BTOR_DELETEN (mm, S, size + 1);

  BTOR_DELETEN (mm, nD, size);
  BTOR_DELETEN (mm, A, size);

  *Qptr = Q;
  *Rptr = R;
}

char *
btor_udiv_const (BtorMemMgr *mm, const char *a, const char *b)
{
  char *quotient, *remainder;

  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (strlen (a) == strlen (b));
  assert ((int) strlen (a) > 0);
  assert (is_valid_const (a));
  assert (is_valid_const (b));

  udiv_urem_const (mm, a, b, &quotient, &remainder);
  btor_delete_const (mm, remainder);
  return quotient;
}

char *
btor_urem_const (BtorMemMgr *mm, const char *a, const char *b)
{
  char *quotient, *remainder;

  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (strlen (a) == strlen (b));
  assert ((int) strlen (a) > 0);
  assert (is_valid_const (a));
  assert (is_valid_const (b));

  udiv_urem_const (mm, a, b, &quotient, &remainder);
  btor_delete_const (mm, quotient);
  return remainder;
}

static char *
sll_const (BtorMemMgr *mm, const char *a, const char *b)
{
  char *result, *temp;
  int i, len;
  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert ((int) strlen (a) > 1);
  assert (btor_is_power_of_2_util ((int) strlen (a)));
  assert (btor_log_2_util ((int) strlen (a)) == (int) strlen (b));
  assert (is_valid_const_3vl (a));
  assert (is_valid_const (b));

  len = (int) strlen (b);
  if (b[len - 1] == '1')
    result = btor_sll_n_bits_const (mm, a, 1);
  else
    result = btor_copy_const (mm, a);
  for (i = len - 2; i >= 0; i--)
  {
    temp = result;
    if (b[i] == '1')
      result = btor_sll_n_bits_const (mm, temp, btor_pow_2_util (len - i - 1));
    else
      result = btor_copy_const (mm, temp);
    btor_delete_const (mm, temp);
  }
  return result;
}

char *
btor_sll_const (BtorMemMgr *mm, const char *a, const char *b)
{
  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert ((int) strlen (a) > 1);
  assert (btor_is_power_of_2_util ((int) strlen (a)));
  assert (btor_log_2_util ((int) strlen (a)) == (int) strlen (b));
  assert (is_valid_const (a));
  assert (is_valid_const (b));

  return sll_const (mm, a, b);
}

static char *
srl_const (BtorMemMgr *mm, const char *a, const char *b)
{
  char *result, *temp;
  int i, len;

  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert ((int) strlen (a) > 1);
  assert (btor_is_power_of_2_util ((int) strlen (a)));
  assert (btor_log_2_util ((int) strlen (a)) == (int) strlen (b));
  assert (is_valid_const_3vl (a));
  assert (is_valid_const (b));

  len = (int) strlen (b);
  if (b[len - 1] == '1')
    result = btor_srl_n_bits_const (mm, a, 1);
  else
    result = btor_copy_const (mm, a);
  for (i = len - 2; i >= 0; i--)
  {
    temp = result;
    if (b[i] == '1')
      result = btor_srl_n_bits_const (mm, temp, btor_pow_2_util (len - i - 1));
    else
      result = btor_copy_const (mm, temp);
    btor_delete_const (mm, temp);
  }
  return result;
}

char *
btor_srl_const (BtorMemMgr *mm, const char *a, const char *b)
{
  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert ((int) strlen (a) > 1);
  assert (btor_is_power_of_2_util ((int) strlen (a)));
  assert (btor_log_2_util ((int) strlen (a)) == (int) strlen (b));
  assert (is_valid_const (a));
  assert (is_valid_const (b));

  return srl_const (mm, a, b);
}

char *
btor_ult_const (BtorMemMgr *mm, const char *a, const char *b)
{
  char *result;

  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (strlen (a) == strlen (b));
  assert ((int) strlen (a) > 0);
  assert (is_valid_const (a));
  assert (is_valid_const (b));

  BTOR_NEWN (mm, result, 2);
  if (strcmp (a, b) == -1)
    result[0] = '1';
  else
    result[0] = '0';
  result[1] = '\0';
  return result;
}

static char *
concat_const (BtorMemMgr *mm, const char *a, const char *b)
{
  char *result;

  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert ((int) strlen (a) > 0);
  assert ((int) strlen (b) > 0);
  assert (is_valid_const_3vl (a));
  assert (is_valid_const_3vl (b));

  BTOR_NEWN (mm, result, (int) strlen (a) + (int) strlen (b) + 1);
  strcpy (result, a);
  strcat (result, b);
  return result;
}

char *
btor_concat_const (BtorMemMgr *mm, const char *a, const char *b)
{
  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert ((int) strlen (a) > 0);
  assert ((int) strlen (b) > 0);
  assert (is_valid_const (a));
  assert (is_valid_const (b));
  return concat_const (mm, a, b);
}

char *
btor_udiv_unbounded_const (BtorMemMgr *mem,
                           const char *dividend,
                           const char *divisor,
                           char **rem_ptr)
{
  char *quotient, *rest, *extended_divisor, *tmp;
  int delta, plen, qlen;
  const char *p, *q;

  assert (mem != NULL);
  assert (dividend != NULL);
  assert (divisor != NULL);
  assert (is_valid_const (dividend));
  assert (is_valid_const (divisor));

  dividend = strip_zeroes (dividend);
  divisor  = strip_zeroes (divisor);

  for (p = dividend; *p && *p == '0'; p++)
    ;

  for (q = divisor; *q && *q == '0'; q++)
    ;

  assert (*q); /* in any case even if 'dividend == 0' */

  if (!*p || btor_cmp_const (p, q) < 0)
  {
    if (rem_ptr) *rem_ptr = btor_strdup (mem, p); /* copy divident */

    return btor_strdup (mem, "");
  }

  plen  = (int) strlen (p);
  qlen  = (int) strlen (q);
  delta = plen - qlen;
  assert (delta >= 0);

  BTOR_NEWN (mem, extended_divisor, plen + 1);
  memset (extended_divisor, '0', delta);
  strcpy (extended_divisor + delta, divisor);

  udiv_urem_const (mem, dividend, extended_divisor, &quotient, &rest);

  btor_delete_const (mem, extended_divisor);

  tmp = btor_strdup (mem, strip_zeroes (quotient));
  btor_delete_const (mem, quotient);
  quotient = tmp;

  tmp = btor_strdup (mem, strip_zeroes (rest));
  btor_delete_const (mem, rest);
  rest = tmp;

  assert (btor_cmp_const (rest, divisor) < 0);
#ifndef NDEBUG
  {
    char *tmp1 = btor_mult_unbounded_const (mem, quotient, divisor);
    char *tmp2 = btor_add_unbounded_const (mem, tmp1, rest);
    assert (!btor_cmp_const (dividend, tmp2));
    btor_freestr (mem, tmp1);
    btor_freestr (mem, tmp2);
  }
#endif
  if (rem_ptr)
    *rem_ptr = rest;
  else
    btor_delete_const (mem, rest);

  return quotient;
}

char *
btor_uext_const (BtorMemMgr *mm, const char *c, int len)
{
  char *res, *q;
  const char *p;
  int rlen;

  assert (mm != NULL);
  assert (c != NULL);
  assert (len > 0);
  assert (is_valid_const (c));

  rlen = (int) strlen (c) + len;

  BTOR_NEWN (mm, res, rlen + 1);

  for (q = res; len; len--, q++) *q = '0';

  for (p = c; *p; p++, q++) *q = *p;

  assert (res + rlen == q);
  *q = 0;

  return res;
}

char *
btor_inverse_const (BtorMemMgr *mm, const char *c)
{
  char *a, *b, *y, *ly, *r, *q, *yq, *res, *ty;
  int len = strlen (c);

  assert (mm != NULL);
  assert (c != NULL);
  assert (len > 0);
  assert (c[len - 1] == '1'); /* odd */
  assert (is_valid_const (c));

  BTOR_NEWN (mm, a, len + 2);
  a[0] = '1';
  memset (a + 1, '0', len);
  a[len + 1] = 0;

  BTOR_NEWN (mm, b, len + 2);
  b[0] = '0';
  memcpy (b + 1, c, len);
  b[len + 1] = 0;

  y  = btor_unsigned_to_const (mm, 1, len + 1);
  ly = btor_unsigned_to_const (mm, 0, len + 1);

  while (!btor_is_zero_const (b))
  {
    udiv_urem_const (mm, a, b, &q, &r);

    btor_delete_const (mm, a);

    a = b;
    b = r;

    ty = y;
    yq = btor_mul_const (mm, y, q);
    btor_delete_const (mm, q);

    y = btor_sub_const (mm, ly, yq);
    btor_delete_const (mm, yq);

    btor_delete_const (mm, ly);
    ly = ty;
  }

  res = btor_slice_const (mm, ly, len - 1, 0);

#ifndef NDEBUG
  assert (strlen (res) == strlen (c));
  ty = btor_mul_const (mm, c, res);
  assert (btor_is_one_const (ty));
  btor_delete_const (mm, ty);
#endif

  btor_delete_const (mm, ly);
  btor_delete_const (mm, y);
  btor_delete_const (mm, b);
  btor_delete_const (mm, a);

  return res;
}

char *
btor_const_to_hex (BtorMemMgr *mem, const char *c)
{
  int clen, rlen, i, j, tmp;
  char *res, ch;

  assert (mem != NULL);
  assert (c != NULL);
  assert (is_valid_const (c));

  clen = (int) strlen (c);
  rlen = (clen + 3) / 4;

  if (rlen)
  {
    BTOR_NEWN (mem, res, rlen + 1);

    i = clen - 1;
    j = rlen;

    res[j--] = 0;

    while (i >= 0)
    {
      tmp = (c[i--] == '1');
      if (i >= 0)
      {
        tmp |= (c[i--] == '1') << 1;
        if (i >= 0)
        {
          tmp |= (c[i--] == '1') << 2;
          if (i >= 0) tmp |= (c[i--] == '1') << 3;
        }
      }

      if (tmp < 10)
        ch = '0' + tmp;
      else
        ch = 'a' + (tmp - 10);

      res[j--] = ch;
    }
  }
  else
    res = btor_strdup (mem, "0");

  return res;
}

char *
btor_const_to_decimal (BtorMemMgr *mem, const char *c)
{
  char *res, *q, *tmp, *rem, ch;
  BtorCharStack stack;
  const char *p;
  int len;
  BTOR_INIT_STACK (stack);

  assert (mem != NULL);
  assert (c != NULL);
  assert (is_valid_const (c));

  res = btor_copy_const (mem, c);
  while (*res)
  {
    tmp = btor_udiv_unbounded_const (mem, res, "1010", &rem); /* / 10 */
    assert ((int) strlen (rem) <= 4);
    ch = 0;
    for (p = strip_zeroes (rem); *p; p++)
    {
      ch <<= 1;
      if (*p == '1') ch++;
    }
    assert (ch < 10);
    ch += '0';
    BTOR_PUSH_STACK (mem, stack, ch);
    btor_delete_const (mem, rem);
    btor_delete_const (mem, res);
    res = tmp;
  }
  btor_delete_const (mem, res);

  if (BTOR_EMPTY_STACK (stack)) BTOR_PUSH_STACK (mem, stack, '0');

  len = BTOR_COUNT_STACK (stack);
  BTOR_NEWN (mem, res, len + 1);
  q = res;
  p = stack.top;
  while (p > stack.start) *q++ = *--p;
  assert (res + len == q);
  *q = 0;
  assert (len == (int) strlen (res));
  BTOR_RELEASE_STACK (mem, stack);
  return res;
}

char *
btor_x_const_3vl (BtorMemMgr *mm, int len)
{
  char *res;
  int i;

  assert (mm != NULL);
  assert (len > 0);

  BTOR_NEWN (mm, res, len + 1);
  for (i = 0; i < len; i++) res[i] = 'x';
  res[i] = '\0';

  return res;
}

#if 0
void
btor_invert_const_3vl (BtorMemMgr * mm, char *a)
{
  assert (mm != NULL);
  assert (a != NULL);
  assert ((int) strlen (a) > 0);
  assert (is_valid_const_3vl (a));
  invert_const (mm, a);
}
#endif

char *
btor_not_const_3vl (BtorMemMgr *mm, const char *a)
{
  assert (mm != NULL);
  assert (a != NULL);
  assert ((int) strlen (a) > 0);
  assert (is_valid_const_3vl (a));
  return not_const (mm, a);
}

#if 0
char *
btor_and_const_3vl (BtorMemMgr * mm, const char *a, const char *b)
{
  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (strlen (a) == strlen (b));
  assert ((int) strlen (a) > 0);
  assert (is_valid_const_3vl (a));
  assert (is_valid_const_3vl (b));
  return and_const (mm, a, b);
}

char *
btor_eq_const_3vl (BtorMemMgr * mm, const char *a, const char *b)
{
  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (strlen (a) == strlen (b));
  assert ((int) strlen (a) > 0);
  assert (is_valid_const_3vl (a));
  assert (is_valid_const_3vl (b));
  return eq_const (mm, a, b);
}

static void
ult_const_0_x_or_x_1_case (const char *a, const char *b, int len, int i,
                           char *result)
{
  assert (a != NULL);
  assert (b != NULL);
  assert (len == (int) strlen (a));
  assert (i >= 0);
  assert (i < len);
  assert (result != NULL);
  assert ((int) strlen (result) == 1);
  assert ((a[i] == '0' && b[i] == 'x') || (a[i] == 'x' && b[i] == '1'));

  result[0] = 'x';

  for (i = i + 1; i < len; i++)
    {

      if (a[i] == '0' && b[i] == 'x')
        continue;

      if (a[i] == 'x' && b[i] == '1')
        continue;

      if (a[i] == '0' && b[i] == '1')
        {
          result[0] = '1';
          break;
        }

      if (a[i] == b[i] && a[i] != 'x' && b[i] != 'x')
        continue;
      else
        break;

    }
}

static void
ult_const_1_x_or_x_0_case (const char *a, const char *b, int len, int i,
                           char *result)
{
  assert (a != NULL);
  assert (b != NULL);
  assert (len == (int) strlen (a));
  assert (i >= 0);
  assert (i < len);
  assert (result != NULL);
  assert ((int) strlen (result) == 1);
  assert ((a[i] == '1' && b[i] == 'x') || (a[i] == 'x' && b[i] == '0'));

  result[0] = '0';

  for (i = i + 1; i < len; i++)
    {

      if (a[i] == '0')
        {
          if (b[i] == '0')
            continue;
          else
            {
              result[0] = 'x';
              break;
            }
        }

      if (a[i] == '1')
        {
          if (b[i] == '0')
            break;
          else
            continue;
        }

      if (a[i] == 'x')
        {
          if (b[i] == '0')
            continue;
          else
            {
              result[0] = 'x';
              break;
            }
        }

    }

}

char *
btor_ult_const_3vl (BtorMemMgr * mm, const char *a, const char *b)
{
  char *result;
  int i, len;

  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (strlen (a) == strlen (b));
  assert ((int) strlen (a) > 0);
  assert (is_valid_const_3vl (a));
  assert (is_valid_const_3vl (b));

  len = (int) strlen (a);
  BTOR_NEWN (mm, result, 2);
  result[0] = '0';
  result[1] = '\0';

  for (i = 0; i < len; i++)
    {

      if (a[i] == '0')
        {
          if (b[i] == 'x')
            {
              ult_const_0_x_or_x_1_case (a, b, len, i, result);
              break;
            }
          else if (b[i] == '1')
            {
              result[0] = '1';
              break;
            }
          else
            continue;
        }

      if (a[i] == '1')
        {
          if (b[i] == 'x')
            {
              ult_const_1_x_or_x_0_case (a, b, len, i, result);
              break;
            }
          else if (b[i] == '0')
            {
              result[0] = '0';
              break;
            }
          else
            continue;
        }

      if (a[i] == 'x')
        {
          if (b[i] == '1')
            {
              ult_const_0_x_or_x_1_case (a, b, len, i, result);
              break;
            }
          else if (b[i] == '0')
            {
              ult_const_1_x_or_x_0_case (a, b, len, i, result);
              break;
            }
          else
            {
              result[0] = 'x';
              break;
            }
        }

      assert (a[i] == b[i] && a[i] != 'x' && b[i] != 'x');
    }

  return result;

}

char *
btor_add_const_3vl (BtorMemMgr * mm, const char *a, const char *b)
{
  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (strlen (a) == strlen (b));
  assert ((int) strlen (a) > 0);
  assert (is_valid_const_3vl (a));
  assert (is_valid_const_3vl (b));
  return add_const (mm, a, b);
}

static void 
compute_min_max_num_shifts (BtorMemMgr * mm, const char *b, int *min_shifts,
                            int *max_shifts)
{
  int len, i, shifts;
  assert (mm != NULL);
  assert (b != NULL);
  assert (min_shifts != NULL);
  assert (max_shifts != NULL);
  (void) mm;

  len = (int) strlen (b);
  *min_shifts = 0;
  *max_shifts = 0;

  for (i = 0; i < len; i++)
    {
      if (b[i] == '1')
        {
	  shifts = btor_pow_2_util (len - i - 1);
          *min_shifts += shifts;
          *max_shifts += shifts;
        }
      else if (b[i] == 'x')
	*max_shifts += btor_pow_2_util (len - i - 1);
    }
}

char *
btor_sll_const_3vl (BtorMemMgr * mm, const char *a, const char *b)
{
  int min_shifts, max_shifts, len, found, i, j;
  char cur, *result, *temp;

  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert ((int) strlen (a) > 1);
  assert (btor_is_power_of_2_util ((int) strlen (a)));
  assert (btor_log_2_util ((int) strlen (a)) == (int) strlen (b));
  assert (is_valid_const_3vl (a));
  assert (is_valid_const_3vl (b));

  if (is_valid_const (b))
    result = sll_const (mm, a, b);
  else
    {
      len = (int) strlen (a);
      temp = btor_x_const_3vl (mm, len);
      compute_min_max_num_shifts (mm, b, &min_shifts, &max_shifts);
      result = btor_sll_n_bits_const (mm, temp, min_shifts);
      for (i = 0; i < len; i++)
        {
	  cur = a[i];
	  if (cur != 'x')
	    {
	      found = 0;
	      for (j = i + 1; j < len; j++)
		{
		  if (a[j] == cur)
		    found++;
		  else
		   break;
		}
	      if (found >= max_shifts)
		result[i] = cur;
	    }
	}
      btor_delete_const (mm, temp);
    }
  return result;
}

char *
btor_srl_const_3vl (BtorMemMgr * mm, const char *a, const char *b)
{

  int min_shifts, max_shifts, len, found, i, j;
  char *result, *temp, cur;

  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert ((int) strlen (a) > 1);
  assert (btor_is_power_of_2_util ((int) strlen (a)));
  assert (btor_log_2_util ((int) strlen (a)) == (int) strlen (b));
  assert (is_valid_const_3vl (a));
  assert (is_valid_const_3vl (b));

  if (is_valid_const (b))
    result = srl_const (mm, a, b);
  else
    {
      len = (int) strlen (a);
      temp = btor_x_const_3vl (mm, len);
      compute_min_max_num_shifts (mm, b, &min_shifts, &max_shifts);
      result = btor_srl_n_bits_const (mm, temp, min_shifts);
      for (i = len - 1; i >= 0; i--)
        {
	  cur = a[i];
	  if (cur != 'x')
	    {
	      found = 0;
	      for (j = i - 1; j >= 0; j--)
		{
		  if (a[j] == cur)
		    found++;
		  else
		   break;
		}
	      if (found >= max_shifts)
		result[i] = cur;
	    }
	}
      btor_delete_const (mm, temp);
    }
  return result;
}

char *
btor_mul_const_3vl (BtorMemMgr * mm, const char *a, const char *b)
{
  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (strlen (a) == strlen (b));
  assert ((int) strlen (a) > 0);
  assert (is_valid_const_3vl (a));
  assert (is_valid_const_3vl (b));
  return mul_const (mm, a, b);
}

static char 
is_unequal_zero_3vl (BtorMemMgr * mm, const char * b)
{
  int found_x;
  const char *p;
  assert (mm != NULL);
  assert (b != NULL);
  (void) mm;

  found_x = 0;
  for (p = b; *p; p++)
    {
      if (*p == '1')
	return '1';
      else if (*p == 'x')
	found_x = 1;
    }

  if (found_x)
    return 'x';
  return '0';
}

char *
btor_udiv_const_3vl (BtorMemMgr * mm, const char *a, const char *b)
{
  char *quotient, *remainder;

  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (strlen (a) == strlen (b));
  assert ((int) strlen (a) > 0);
  assert (is_valid_const_3vl (a));
  assert (is_valid_const_3vl (b));

  udiv_urem_const (mm, a, b, &quotient, &remainder);
  btor_delete_const (mm, remainder);

  return quotient;
}

char *
btor_urem_const_3vl (BtorMemMgr * mm, const char *a, const char *b)
{
  int i, len, skip_a, skip_b;
  char *quotient, *remainder;

  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (strlen (a) == strlen (b));
  assert ((int) strlen (a) > 0);
  assert (is_valid_const_3vl (a));
  assert (is_valid_const_3vl (b));

  udiv_urem_const (mm, a, b, &quotient, &remainder);
  btor_delete_const (mm, quotient);

  /* optimization: b != 0 => a urem b <= a  /\ a urem b <= b,
   * this optimization is necessary, as the urem circuit 
   * does not propagate the known bits well */ 
  skip_a = 0;
  skip_b = 0;
  if (is_unequal_zero_3vl(mm, b) == '1')
    {
      len = (int) strlen (a);
      for (i = 0; i < len; i++)
        {
	  if (!skip_a && a[i] == '0')
	    remainder[i] = '0';
	  else if (!skip_b)
	    skip_a = 1;
	  else
	    break;

	  if (!skip_b && b[i] == '0')
	    remainder[i] = '0';
	  else if (!skip_a)
	    skip_b = 1;
	  else
	    break;
	}
    }

  return remainder;
}

char *
btor_concat_const_3vl (BtorMemMgr * mm, const char *a, const char *b)
{
  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert ((int) strlen (a) > 0);
  assert ((int) strlen (b) > 0);
  assert (is_valid_const_3vl (a));
  assert (is_valid_const_3vl (b));
  return concat_const (mm, a, b);
}

char *
btor_slice_const_3vl (BtorMemMgr * mm, const char *a, int upper, int lower)
{
  assert (mm != NULL);
  assert (a != NULL);
  assert ((int) strlen (a) > 0);
  assert (upper < (int) strlen (a));
  assert (upper >= lower);
  assert (lower >= 0);
  assert (is_valid_const_3vl (a));

  return slice_const (mm, a, upper, lower);
}

char *
btor_cond_const_3vl (BtorMemMgr * mm, const char *a, const char *b,
                     const char *c)
{
  char *result;
  int i, len;

  assert (mm != NULL);
  assert (a != NULL);
  assert (b != NULL);
  assert (c != NULL);
  assert ((int) strlen (a) == 1);
  assert (strlen (b) == strlen (c));
  assert ((int) strlen (b) > 0);

  if (a[0] == '1')
    result = btor_copy_const (mm, b);
  else if (a[0] == '0')
    result = btor_copy_const (mm, c);
  else
    {
      len = (int) strlen (b);
      BTOR_NEWN (mm, result, len + 1);
      for (i = 0; i < len; i++)
        {
          if (b[i] != 'x' && c[i] != 'x' && b[i] == c[i])
            result[i] = b[i];
          else
            result[i] = 'x';
        }
      result[i] = '\0';
    }
  return result;
}
#endif

int
btor_is_const_2vl (BtorMemMgr *mm, const char *c)
{
  assert (mm != NULL);
  assert (c != NULL);
  (void) mm;
  return strchr (c, 'x') == NULL;
}
