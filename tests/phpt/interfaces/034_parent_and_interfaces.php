@ok
<?php

interface IToArray {
    /**
     * @return int[]
     */
    public function to_array(): array;
}

interface IFromArray {
    public function from_array(array $arr) : IFromArray;
}

interface IArrayConvertible extends IToArray, IFromArray {
}

interface IRunnable {
    public function run(): void;
}

interface IPrintable {
    public function print_me(): void;
}

class Printable implements IPrintable {
    public $name = "Printable";

    public function print_me(): void {
        var_dump("{$this->name}::print_me");
    }
}

class Foo extends Printable implements IArrayConvertible, IRunnable {
    public $x = 10;

    public function __construct() {
        $this->name = "Foo";
    }

    /**
     * @return int[]
     */
    public function to_array(): array {
        var_dump("{$this->name}::to_array");
        return [10];
    }

    public function from_array(array $arr) : IFromArray {
        var_dump("{$this->name}::from_array");

        return new Foo();
    }

    public function run(): void {
        var_dump("{$this->name}::run");
        $this->print_me();
    }
}

class Bar implements IToArray, IFromArray {
    public $x = 777;

    /**
     * @return int[]
     */
    public function to_array(): array {
        var_dump("Bar::to_array");
        return [10];
    }

    public function from_array(array $arr) : IFromArray {
        var_dump("Bar::from_array");

        return new Bar();
    }
}

function run_to_array(IToArray $i_to_array) {
    $i_to_array->to_array();
}

function run_from_array(IFromArray $i_from_array) {
    $i_from_array->from_array([]);
}

function run_print_me(IPrintable $i_printable) {
    $i_printable->print_me();
}

function run_run(IRunnable $i_runnable) {
    $i_runnable->run();
}

function run() {

    run_to_array(new Foo());
    run_to_array(new Bar());

    run_from_array(new Foo());
    run_from_array(new Bar());

    run_print_me(new Foo());
    run_print_me(new Printable());

    run_run(new Foo());

    /**@var $to_arr IToArray*/
    $to_arr = new Bar();
    $to_arr = new Foo();
    $to_arr->to_array();

    /**@var $from_arr IFromArray*/
    $from_arr = new Bar();
    $from_arr = new Foo();
    $from_arr->from_array([]);

    /**@var $pri IPrintable*/
    $pri = new Foo();
    $pri = new Printable();
    $pri->print_me();

    if ($to_arr instanceof Printable) {
        $to_arr->print_me();
    }

    if ($to_arr instanceof IArrayConvertible) {
        var_dump("convertible");
    }

    if ($to_arr instanceof IToArray) {
        var_dump("to_array");
    }

    if ($to_arr instanceof IFromArray) {
        var_dump("from_array");
    }

    if ($to_arr instanceof Foo) {
        var_dump("FOO:{$to_arr->x}");
    } else if ($to_arr instanceof Bar) {
        var_dump("BAR:{$to_arr->x}");
    }
}

run();
