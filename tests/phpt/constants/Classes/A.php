<?php

 namespace Classes;

 class A {
     public int $x;
     public function __construct(int $x) {
         $this->x = $x;
     }
     public function __toString() {
         return strval($this->x);
     }
 }
