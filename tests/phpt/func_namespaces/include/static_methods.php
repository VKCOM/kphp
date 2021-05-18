<?php

namespace Example;

class BaseClassWithMagic {
    static protected function demo($key) {
        return 1;
    }
}

class DerivedClassWithMagic extends BaseClassWithMagic {
    static public function do_magic2() {
      return self::demo("123");
    }
}

var_dump(DerivedClassWithMagic::do_magic2());
