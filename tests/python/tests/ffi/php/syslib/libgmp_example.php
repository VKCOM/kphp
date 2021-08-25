<?php

function test() {
  $libgmp = FFI::cdef('
      extern const char *const __gmp_version;

      struct mpz_t {
        int _mp_alloc;
        int _mp_size;
        void *_data;
      };

      void __gmpz_init(struct mpz_t *z);
      void __gmpz_clear(struct mpz_t *z);

      void __gmpz_set_si(struct mpz_t *z, long int value);
      long int __gmpz_get_si(struct mpz_t *z);

      void __gmpz_add(struct mpz_t *dst, struct mpz_t *x, struct mpz_t *y);
  ', 'libgmp.so');

  var_dump($libgmp->__gmp_version);

  $x = $libgmp->new('struct mpz_t');
  $libgmp->__gmpz_init(FFI::addr($x));
  $y = $libgmp->new('struct mpz_t');
  $libgmp->__gmpz_init(FFI::addr($y));

  var_dump($libgmp->__gmpz_get_si(FFI::addr($x)));
  $libgmp->__gmpz_set_si(FFI::addr($x), 50);
  var_dump($libgmp->__gmpz_get_si(FFI::addr($x)));

  $libgmp->__gmpz_set_si(FFI::addr($y), 2400);

  $libgmp->__gmpz_add(FFI::addr($x), FFI::addr($x), FFI::addr($y));
  var_dump($libgmp->__gmpz_get_si(FFI::addr($x)));

  $libgmp->__gmpz_clear(FFI::addr($x));
  $libgmp->__gmpz_clear(FFI::addr($y));
}

test();
