@ok
<?php

interface I {}
interface I2 {}

class GrandPa implements I, I2 {}

class Pa extends GrandPa {}
class Derived extends Pa {}

class Pa2 extends GrandPa {}
class Derived2 extends Pa2 {}

$d = new Derived();
$d2 = new Derived2();
