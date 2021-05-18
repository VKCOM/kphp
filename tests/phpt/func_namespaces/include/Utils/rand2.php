<?php

namespace Utils;

require_once __DIR__ . '/rand.php';

function rand2() {
  return rand(); // should be Utils\rand
}
