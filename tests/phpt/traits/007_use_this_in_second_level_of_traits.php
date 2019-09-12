@ok
<?php

trait Hello {
    public function sayA() {
        echo 'A!';
        $this->sayB();
    }
}

trait World {
    use Hello;

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
