@kphp_should_fail
<?php
trait Hello {
    public function sayAB() {
      echo "AB";
    }
}

interface HelloWorld {
    use Hello;
}

class MyHelloWorld implements HelloWorld {
    public function sayAB() {
      echo "MyAB";
    }
}

$o = new MyHelloWorld();
$o->sayAB();

