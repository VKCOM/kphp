@kphp_should_fail php8
/C::X cannot override final constant I::X/
<?php

interface I {
  final public const X = 1;
}

class C implements I {
  const X = 2;
}
