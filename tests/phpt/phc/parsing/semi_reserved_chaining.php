@ok
<?php

class Foo {
    public $use = 'treasure';

    public function throw() {
        var_dump(__METHOD__);
        return $this;
    }
}

var_dump((new Foo)->throw()->throw()->use);
