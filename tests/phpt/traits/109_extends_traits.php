@kphp_should_fail
<?php
trait Hello {
    public function sayA() {
        echo 'A!';
    }
}

trait World extends Hello {
    public function sayB() {
        echo 'B!';
    }
}

class MyHelloWorld {
    use World;

    public function say() {
      $this->sayA();
    }
}

$o = new MyHelloWorld();
$o->say();
