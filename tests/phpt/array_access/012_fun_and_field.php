@ok
<?php
require_once 'kphp_tester_include.php';

class A {
    /** @var mixed */
    public $mix;

    /** @var Classes\LoggingLikeArray */
    public $obj;

    
    /** @return mixed */
    public function get_mix() {
        return $this->mix;
    }

    /** @return Classes\LoggingLikeArray */
    public function get_obj() {
        return $this->obj;
    }
}

class B {
    /** @var A*/
    public $a;

    /** @return A */
    public function get_a() {
        var_dump("get_a");
        return $this->a;
    }
}

class C {
    /** @var B*/
    public $b;

    /** @return B */
    public function get_b() {
        var_dump("get_b");
        return $this->b;
    }
}

/** @param $x mixed[]
 *  @return mixed
 */
function as_mix_obj($x) {
    var_dump("as_mixed_obj");
    return to_mixed(new Classes\LoggingLikeArray($x));
}


/** @param $b B
 *  @return C
 */
function create_c($b) {
    var_dump("create_c");
    $c = new C;
    $c->b = $b;

    return $c;
}


function test() {
    $a = new A;
    $a->obj = new Classes\LoggingLikeArray([null, 1 => [1 => 42]]);
    $a->mix = as_mix_obj([1 => [1 => 100500]]);
    $b = new B;
    $b->a = $a;
    $c = new C;
    $c->b = $b;


    var_dump(isset($c->b->a->obj[1][1]));
    var_dump(isset($c->b->get_a()->get_mix()[1][1]));
    var_dump(isset($c->b->get_a()->get_obj()[1][1]));
    var_dump(isset($c->get_b()->a->obj[1][1]));
    var_dump(isset($c->b->get_a()->get_mix()[1][1]));
    var_dump(isset($c->get_b()->get_a()->get_mix()[1][1]));
    var_dump(isset(create_c($c->b)->b->a->obj[1][1]));
    var_dump(isset(create_c($c->b)->b->get_a()->get_mix()[1][1]));
    var_dump(isset(create_c($c->b)->get_b()->a->obj[1][1]));
    var_dump(isset(create_c($c->b)->get_b()->get_a()->get_mix()[1][1]));
    var_dump(isset($c->get_b()->a->get_mix()[1][1]));
    var_dump(isset(create_c($c->b)->b->a->get_mix()[1][1]));
    var_dump(isset(create_c($c->b)->get_b()->a->get_mix()[1][1]));

    var_dump(empty($c->b->a->obj[1][1]));
    var_dump(empty($c->b->get_a()->get_mix()[1][1]));
    var_dump(empty($c->b->get_a()->get_obj()[1][1]));
    var_dump(empty($c->get_b()->a->obj[1][1]));
    var_dump(empty($c->get_b()->get_a()->get_mix()[1][1]));
    var_dump(empty(create_c($c->b)->b->a->obj[1][1]));
    var_dump(empty(create_c($c->b)->b->get_a()->get_mix()[1][1]));
    var_dump(empty(create_c($c->b)->get_b()->a->obj[1][1]));
    var_dump(empty(create_c($c->b)->get_b()->get_a()->get_mix()[1][1]));
    var_dump(empty($c->b->a->get_mix()[1][1]));
    var_dump(empty($c->get_b()->a->get_mix()[1][1]));
    var_dump(empty(create_c($c->b)->b->a->get_mix()[1][1]));
    var_dump(empty(create_c($c->b)->get_b()->a->get_mix()[1][1]));

    unset($c->b->a->obj[1][1]);
    unset($c->b->a->mix[1][1]);
    unset($c->b->a->obj[1][1]);
    unset($c->b->a->mix[1][1]);
    unset($c->b->a->mix[1][1]);
    unset($c->b->a->mix[1][1]);

    var_dump($c->b->a->obj[1][1]);
    var_dump($c->b->get_a()->get_mix()[1][1]);
    var_dump($c->get_b()->a->obj[1][1]);
    var_dump($c->get_b()->get_a()->get_mix()[1][1]);
    var_dump($c->b->get_a()->get_mix()[1][1]);
    var_dump($c->get_b()->a->get_mix()[1][1]);
}

test();
