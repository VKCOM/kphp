@ok
<?php

class Data {
    public $x = 20;
}

trait HasData {
    /** @var Data */
    public $data;

    /**
     * @kphp-infer
     * @param int $y
     */
    public function print_x_plus_y($y) {
        var_dump($this->data->x + $y);
    }

    public function __construct() {
        $this->data = new Data();
    }
}

class One {
    use HasData;

    public function run() {
        $this->print_x_plus_y(209);
    }
}

class Two {
    use HasData;
}

function test() {
    $one = new One();
    $one->print_x_plus_y(20);
    $one->run();

    $two = new Two();
    $two->print_x_plus_y(200);
}

test();

