@kphp_should_fail
/Json decoding for IValue is unavailable, because it's an interface/
<?php

interface IValue {}

class MyJsonEncoder extends JsonEncoder {}

class A1 implements IValue { public int $a1 = 1; }
class A2 implements IValue { public int $a2 = 1; }

class Demo {
  public $tup;      // will be auto inferred as IValue
}


$d = new Demo;
$d->tup = new A1;
$d->tup = new A2;
MyJsonEncoder::decode('', Demo::class);
