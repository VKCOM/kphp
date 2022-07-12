@kphp_should_fail
/Json decoding for IValue is unavailable, because it's an interface/
<?php

interface IValue {}

class A1 implements IValue { public int $a1 = 1; }
class A2 implements IValue { public int $a2 = 1; }

JsonEncoder::decode('', IValue::class);
