// Compiler for PHP (aka KPHP)
// Copyright (c) 2020 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef KDB_COMMON_OPENSSL_H
#define KDB_COMMON_OPENSSL_H

#include <stddef.h>
#include <string.h>

#include "openssl/crypto.h"
#include "openssl/engine.h"
#include "openssl/evp.h"
#include "openssl/opensslv.h"

#if OPENSSL_VERSION_NUMBER < 0x10100000L

static inline RSA *EVP_PKEY_get0_RSA(EVP_PKEY *pkey) {
  if (pkey->type != EVP_PKEY_RSA) {
    return NULL;
  }

  return pkey->pkey.rsa;
}

static inline int EVP_CIPHER_CTX_reset(EVP_CIPHER_CTX *ctx) {
  return EVP_CIPHER_CTX_cleanup(ctx);
}

static inline void *OPENSSL_zalloc(size_t num) {
  void *ret = OPENSSL_malloc(num);

  if (ret != NULL) {
    memset(ret, 0, num);
  }

  return ret;
}

static inline EVP_MD_CTX *EVP_MD_CTX_new() {
  return (EVP_MD_CTX *)OPENSSL_zalloc(sizeof(EVP_MD_CTX));
}

static inline void EVP_MD_CTX_free(EVP_MD_CTX *ctx) {
  EVP_MD_CTX_cleanup(ctx);
  OPENSSL_free(ctx);
}

static inline int BN_bn2lebinpad(const BIGNUM *a, unsigned char *to, int tolen) {
  int i;
  BN_ULONG l;
  bn_check_top(a);
  i = BN_num_bytes(a);
  if (tolen < i)
    return -1;
  /* Add trailing zeroes if necessary */
  if (tolen > i) {
    memset(to + i, 0, tolen - i);
  }
  to += i;
  while (i--) {
    l = a->d[i / BN_BYTES];
    to--;
    *to = (unsigned char)(l >> (8 * (i % BN_BYTES))) & 0xff;
  }
  return tolen;
}

static inline int DH_set0_pqg(DH *dh, BIGNUM *p, BIGNUM *q, BIGNUM *g) {
  /* If the fields p and g in d are NULL, the corresponding input
   * parameters MUST be non-NULL.  q may remain NULL.
   */
  if ((dh->p == NULL && p == NULL)
      || (dh->g == NULL && g == NULL)) {
    return 0;
  }

  if (p != NULL) {
    BN_free(dh->p);
    dh->p = p;
  }

  if (q != NULL) {
    BN_free(dh->q);
    dh->q = q;
  }

  if (g != NULL) {
    BN_free(dh->g);
    dh->g = g;
  }

  if (q != NULL) {
    dh->length = BN_num_bits(q);
  }

  return 1;
}

static inline void DH_get0_pqg(const DH *dh, const BIGNUM **p, const BIGNUM **q,
                               const BIGNUM **g) {
  if (p != NULL) {
    *p = dh->p;
  }

  if (q != NULL) {
    *q = dh->q;
  }

  if (g != NULL) {
    *g = dh->g;
  }
}

static inline void DH_get0_key(const DH *dh, const BIGNUM **pub_key,
                               const BIGNUM **priv_key) {
  if (pub_key != NULL) {
    *pub_key = dh->pub_key;
  }

  if (priv_key != NULL) {
    *priv_key = dh->priv_key;
  }
}


ASN1_TIME *X509_getm_notBefore(const X509 *x)
{
    return x->cert_info->validity->notBefore;
}

ASN1_TIME *X509_getm_notAfter(const X509 *x)
{
    return x->cert_info->validity->notAfter;
}

const unsigned char *ASN1_STRING_get0_data(const ASN1_STRING *x)
{
    return x->data;
}

#endif


// this is copypasted from openssl 1.1
#if OPENSSL_VERSION_NUMBER < 0x10002000L

#define SECS_PER_DAY (24 * 60 * 60)

static long date_to_julian(int y, int m, int d);
static void julian_to_date(long jd, int *y, int *m, int *d);
static int julian_adj(const struct tm *tm, int off_day, long offset_sec,
                      long *pday, int *psec);

int OPENSSL_gmtime_adj(struct tm *tm, int off_day, long offset_sec)
{
    int time_sec, time_year, time_month, time_day;
    long time_jd;

    /* Convert time and offset into Julian day and seconds */
    if (!julian_adj(tm, off_day, offset_sec, &time_jd, &time_sec))
        return 0;

    /* Convert Julian day back to date */

    julian_to_date(time_jd, &time_year, &time_month, &time_day);

    if (time_year < 1900 || time_year > 9999)
        return 0;

    /* Update tm structure */

    tm->tm_year = time_year - 1900;
    tm->tm_mon = time_month - 1;
    tm->tm_mday = time_day;

    tm->tm_hour = time_sec / 3600;
    tm->tm_min = (time_sec / 60) % 60;
    tm->tm_sec = time_sec % 60;

    return 1;

}

int OPENSSL_gmtime_diff(int *pday, int *psec,
                        const struct tm *from, const struct tm *to)
{
    int from_sec, to_sec, diff_sec;
    long from_jd, to_jd, diff_day;
    if (!julian_adj(from, 0, 0, &from_jd, &from_sec))
        return 0;
    if (!julian_adj(to, 0, 0, &to_jd, &to_sec))
        return 0;
    diff_day = to_jd - from_jd;
    diff_sec = to_sec - from_sec;
    /* Adjust differences so both positive or both negative */
    if (diff_day > 0 && diff_sec < 0) {
        diff_day--;
        diff_sec += SECS_PER_DAY;
    }
    if (diff_day < 0 && diff_sec > 0) {
        diff_day++;
        diff_sec -= SECS_PER_DAY;
    }

    if (pday)
        *pday = (int)diff_day;
    if (psec)
        *psec = diff_sec;

    return 1;

}

/* Convert tm structure and offset into julian day and seconds */
static int julian_adj(const struct tm *tm, int off_day, long offset_sec,
                      long *pday, int *psec)
{
    int offset_hms, offset_day;
    long time_jd;
    int time_year, time_month, time_day;
    /* split offset into days and day seconds */
    offset_day = (int)(offset_sec / SECS_PER_DAY);
    /* Avoid sign issues with % operator */
    offset_hms = (int)(offset_sec - (offset_day * SECS_PER_DAY));
    offset_day += off_day;
    /* Add current time seconds to offset */
    offset_hms += tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;
    /* Adjust day seconds if overflow */
    if (offset_hms >= SECS_PER_DAY) {
        offset_day++;
        offset_hms -= SECS_PER_DAY;
    } else if (offset_hms < 0) {
        offset_day--;
        offset_hms += SECS_PER_DAY;
    }

    /*
     * Convert date of time structure into a Julian day number.
     */

    time_year = tm->tm_year + 1900;
    time_month = tm->tm_mon + 1;
    time_day = tm->tm_mday;

    time_jd = date_to_julian(time_year, time_month, time_day);

    /* Work out Julian day of new date */
    time_jd += offset_day;

    if (time_jd < 0)
        return 0;

    *pday = time_jd;
    *psec = offset_hms;
    return 1;
}

/*
 * Convert date to and from julian day Uses Fliegel & Van Flandern algorithm
 */
static long date_to_julian(int y, int m, int d)
{
    return (1461 * (y + 4800 + (m - 14) / 12)) / 4 +
        (367 * (m - 2 - 12 * ((m - 14) / 12))) / 12 -
        (3 * ((y + 4900 + (m - 14) / 12) / 100)) / 4 + d - 32075;
}

static void julian_to_date(long jd, int *y, int *m, int *d)
{
    long L = jd + 68569;
    long n = (4 * L) / 146097;
    long i, j;

    L = L - (146097 * n + 3) / 4;
    i = (4000 * (L + 1)) / 1461001;
    L = L - (1461 * i) / 4 + 31;
    j = (80 * L) / 2447;
    *d = (int)(L - (2447 * j) / 80);
    L = j / 11;
    *m = (int)(j + 2 - (12 * L));
    *y = (int)(100 * (n - 49) + i + L);
}


static int asn1_utctime_to_tm(struct tm *tm, const ASN1_UTCTIME *d)
{
    static const int min[8] = { 0, 1, 1, 0, 0, 0, 0, 0 };
    static const int max[8] = { 99, 12, 31, 23, 59, 59, 12, 59 };
    char *a;
    int n, i, l, o;

    if (d->type != V_ASN1_UTCTIME)
        return (0);
    l = d->length;
    a = (char *)d->data;
    o = 0;

    if (l < 11)
        goto err;
    for (i = 0; i < 6; i++) {
        if ((i == 5) && ((a[o] == 'Z') || (a[o] == '+') || (a[o] == '-'))) {
            i++;
            if (tm)
                tm->tm_sec = 0;
            break;
        }
        if ((a[o] < '0') || (a[o] > '9'))
            goto err;
        n = a[o] - '0';
        if (++o > l)
            goto err;

        if ((a[o] < '0') || (a[o] > '9'))
            goto err;
        n = (n * 10) + a[o] - '0';
        if (++o > l)
            goto err;

        if ((n < min[i]) || (n > max[i]))
            goto err;
        if (tm) {
            switch (i) {
            case 0:
                tm->tm_year = n < 50 ? n + 100 : n;
                break;
            case 1:
                tm->tm_mon = n - 1;
                break;
            case 2:
                tm->tm_mday = n;
                break;
            case 3:
                tm->tm_hour = n;
                break;
            case 4:
                tm->tm_min = n;
                break;
            case 5:
                tm->tm_sec = n;
                break;
            }
        }
    }
    if (a[o] == 'Z')
        o++;
    else if ((a[o] == '+') || (a[o] == '-')) {
        int offsign = a[o] == '-' ? 1 : -1, offset = 0;
        o++;
        if (o + 4 > l)
            goto err;
        for (i = 6; i < 8; i++) {
            if ((a[o] < '0') || (a[o] > '9'))
                goto err;
            n = a[o] - '0';
            o++;
            if ((a[o] < '0') || (a[o] > '9'))
                goto err;
            n = (n * 10) + a[o] - '0';
            if ((n < min[i]) || (n > max[i]))
                goto err;
            if (tm) {
                if (i == 6)
                    offset = n * 3600;
                else if (i == 7)
                    offset += n * 60;
            }
            o++;
        }
        if (offset && !OPENSSL_gmtime_adj(tm, 0, offset * offsign))
            return 0;
    }
    return o == l;
 err:
    return 0;
}

static int asn1_generalizedtime_to_tm(struct tm *tm, const ASN1_GENERALIZEDTIME *d)
{
    static const int min[9] = { 0, 0, 1, 1, 0, 0, 0, 0, 0 };
    static const int max[9] = { 99, 99, 12, 31, 23, 59, 59, 12, 59 };
    char *a;
    int n, i, l, o;

    if (d->type != V_ASN1_GENERALIZEDTIME)
        return (0);
    l = d->length;
    a = (char *)d->data;
    o = 0;
    /*
     * GENERALIZEDTIME is similar to UTCTIME except the year is represented
     * as YYYY. This stuff treats everything as a two digit field so make
     * first two fields 00 to 99
     */
    if (l < 13)
        goto err;
    for (i = 0; i < 7; i++) {
        if ((i == 6) && ((a[o] == 'Z') || (a[o] == '+') || (a[o] == '-'))) {
            i++;
            if (tm)
                tm->tm_sec = 0;
            break;
        }
        if ((a[o] < '0') || (a[o] > '9'))
            goto err;
        n = a[o] - '0';
        if (++o > l)
            goto err;

        if ((a[o] < '0') || (a[o] > '9'))
            goto err;
        n = (n * 10) + a[o] - '0';
        if (++o > l)
            goto err;

        if ((n < min[i]) || (n > max[i]))
            goto err;
        if (tm) {
            switch (i) {
            case 0:
                tm->tm_year = n * 100 - 1900;
                break;
            case 1:
                tm->tm_year += n;
                break;
            case 2:
                tm->tm_mon = n - 1;
                break;
            case 3:
                tm->tm_mday = n;
                break;
            case 4:
                tm->tm_hour = n;
                break;
            case 5:
                tm->tm_min = n;
                break;
            case 6:
                tm->tm_sec = n;
                break;
            }
        }
    }
    /*
     * Optional fractional seconds: decimal point followed by one or more
     * digits.
     */
    if (a[o] == '.') {
        if (++o > l)
            goto err;
        i = o;
        while ((a[o] >= '0') && (a[o] <= '9') && (o <= l))
            o++;
        /* Must have at least one digit after decimal point */
        if (i == o)
            goto err;
    }

    if (a[o] == 'Z')
        o++;
    else if ((a[o] == '+') || (a[o] == '-')) {
        int offsign = a[o] == '-' ? 1 : -1, offset = 0;
        o++;
        if (o + 4 > l)
            goto err;
        for (i = 7; i < 9; i++) {
            if ((a[o] < '0') || (a[o] > '9'))
                goto err;
            n = a[o] - '0';
            o++;
            if ((a[o] < '0') || (a[o] > '9'))
                goto err;
            n = (n * 10) + a[o] - '0';
            if ((n < min[i]) || (n > max[i]))
                goto err;
            if (tm) {
                if (i == 7)
                    offset = n * 3600;
                else if (i == 8)
                    offset += n * 60;
            }
            o++;
        }
        if (offset && !OPENSSL_gmtime_adj(tm, 0, offset * offsign))
            return 0;
    } else if (a[o]) {
        /* Missing time zone information. */
        goto err;
    }
    return (o == l);
 err:
    return (0);
}

static int asn1_time_to_tm(struct tm *tm, const ASN1_TIME *t)
{
/*    if (t == NULL) {
        time_t now_t;
        time(&now_t);
        if (OPENSSL_gmtime(&now_t, tm))
            return 1;
        return 0;
    }*/

    if (t->type == V_ASN1_UTCTIME)
        return asn1_utctime_to_tm(tm, t);
    else if (t->type == V_ASN1_GENERALIZEDTIME)
        return asn1_generalizedtime_to_tm(tm, t);

    return 0;
}

int ASN1_TIME_diff(int *pday, int *psec,
                   const ASN1_TIME *from, const ASN1_TIME *to)
{
    struct tm tm_from, tm_to;
    if (!asn1_time_to_tm(&tm_from, from))
        return 0;
    if (!asn1_time_to_tm(&tm_to, to))
        return 0;
    return OPENSSL_gmtime_diff(pday, psec, &tm_from, &tm_to);
}

#undef SECS_PER_DAY

#endif


#endif // KDB_COMMON_CRYPTO_H
