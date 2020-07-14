<?php

namespace Classes;

class DerivedClassWithMagic extends BaseClassWithMagic {
    /**
     * @kphp-infer
     * @return int
     */
    static public function do_magic2() {
      return self::demo("123");
    }
}

