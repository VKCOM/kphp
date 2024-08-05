@kphp_should_fail
\"const" member is not supported in trait\
<?php
 trait T {
     public const T_ = 2;
     public int $t_ = 5;
     public function f() {
         echo self::T_;
         $this->t_ = 1;
         echo $this->t_;
     }
 }

 class B {
     use T;
 }

 $obj = new B();
 $obj->f();
