@ok php8
<?php

interface I {
  const X = 1;
}

class C implements I {}

class D extends C {
  const X = 2;
}
