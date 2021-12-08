@ok php8
<?php

interface I1 {
  const C = 1;
}

interface I2 {
  const C = 2;
}

interface I3 extends I1, I2 {
  const C = 3;
}

class F implements I3 {}

echo F::C;
