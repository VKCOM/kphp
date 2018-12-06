<?php

namespace Classes;

class C_copy
{
  private $c2 = 2;

  public function test_access_private_field_with_same_name() {
      $c = new C;
      $c->setC2(200);
      var_dump($c->c2);
  }
}
