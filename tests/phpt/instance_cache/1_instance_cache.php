@ok non-idempotent k2_skip
<?php

require_once 'kphp_tester_include.php';

/** @kphp-immutable-class */
class X {
  /** @var int */
  public $x_int = 1;
  /** @var string */
  public $x_str = "hello";
  /** @var int[] */
  public $x_array = [1, 2, 3, 4];
  /** @var array */
  public $x_array_var = ["foo", 12, [123]];
}

/** @kphp-immutable-class */
class Y {
  /** @var X */
  public $x_instance;
  /** @var string */
  public $y_string;
  /** @var int[] */
  public $y_array;
  /** @var array */
  public $y_array_var;
  /** @var string|false */
  public $y_string_or_false;
  /** @var tuple<string | false, array, int[], string, X> */
  public $y_tuple;

  /**
   * @param int $i
   * @param string $s
   * @param string|false $or_false_str
   */
  public function __construct($i, $s, $or_false_str = false) {
    $this->x_instance = new X;
    $this->y_string = $this->x_instance->x_str . " world" . $s;
    $this->y_array = $this->x_instance->x_array;
    $this->y_array[] = $i;
    $this->y_array_var = $this->x_instance->x_array_var;
    $this->y_array_var[] = $s;
    $this->y_string_or_false = 1 ? "or_false" : false;
    $this->y_tuple = tuple($or_false_str, $this->y_array_var, $this->y_array, $this->y_string, new X);
  }
}

/** @kphp-immutable-class */
class TreeX {
  /** @var int */
  public $value = 0;
  /** @var tuple<int, TreeX[]> [] */
  public $children = [];

  public function __construct(int $value, array $children = [], bool $make_loop = false) {
    $this->value = $value;
    $this->children = $children;
    if ($make_loop) {
      $this->children[] = tuple(1, [$this]);
    }
  }
}

/** @kphp-immutable-class */
class VectorY {
  /** @var Y[] */
  public $elements = [];

  public function __construct(int $elements_count, array $elements_array = []) {
    if ($elements_count) {
      for ($i = 1; $i < $elements_count; ++$i) {
        $this->elements[] = new Y($i, " <-");
      }
    } else {
      $this->elements = $elements_array;
    }
  }
}

/** @kphp-immutable-class */
class HasShape {
  /** @var tuple(int, string) */
  var $t;
  /** @var shape(x:int, y:string, z?:int[]) */
  var $sh;

  /**
   * @param int $sh_x
   * @param bool $with_z
   */
  function __construct($sh_x, $with_z = false) {
    $this->t = tuple(1, 's');
    if ($with_z) {
      $this->sh = shape(['y' => 'y', 'x' => 2, 'z' => [1,2,3]]);
    } else {
      $this->sh = shape(['y' => 'y', 'x' => $sh_x]);
    }
  }
}

function test_empty_fetch() {
  $x = instance_cache_fetch(X::class, "key_x0");
  var_dump(!$x);
  $y = instance_cache_fetch(Y::class, "key_x0");
  var_dump(!$y);
}

function test_store_fetch() {
  var_dump(instance_cache_store("key_x1", new X));
  var_dump(instance_cache_store("key_y1", new Y(1, "test_store_fetch")));

  $x = instance_cache_fetch(X::class, "key_x1");
  var_dump(to_array_debug($x));

  $y = instance_cache_fetch(Y::class, "key_y1");
  var_dump(to_array_debug($y));
}

function test_mismatch_classes() {
  var_dump(instance_cache_store("key_x2", new X));
  var_dump(instance_cache_store("key_y2", new Y(2, "test_mismatch_classes", "optional")));

  $x = instance_cache_fetch(Y::class, "key_x2");
  var_dump(!$x);

  $y = instance_cache_fetch(X::class, "key_y2");
  var_dump(!$y);
}

function test_update_ttl() {
  var_dump(instance_cache_update_ttl("key_x_test_update_ttl", 12));

  var_dump(instance_cache_store("key_x_test_update_ttl", new X, 1));
  var_dump(instance_cache_update_ttl("key_x_test_update_ttl", 3));
  var_dump(instance_cache_update_ttl("key_x_test_update_ttl", 2));

  var_dump(instance_cache_delete("key_x_test_update_ttl"));
#ifndef KPHP
  var_dump(true);
  if (0)
#endif
  var_dump(instance_cache_update_ttl("key_x_test_update_ttl", 3));

  var_dump(instance_cache_store("key_x_test_update_ttl", new X, 2));
  var_dump(instance_cache_update_ttl("key_x_test_update_ttl"));
}


function test_delete() {
  var_dump(instance_cache_store("key_x3", new X));
  var_dump(instance_cache_store("key_y3", new Y(3, "test_delete", "super optional")));

  var_dump(instance_cache_delete("key_x3_unknown"));
  var_dump(instance_cache_delete("key_y3_unknown"));

  $x = instance_cache_fetch(X::class, "key_x3");
  var_dump(to_array_debug($x));

  $y = instance_cache_fetch(Y::class, "key_y3");
  var_dump(to_array_debug($y));

  var_dump(instance_cache_delete("key_x3"));
  var_dump(instance_cache_delete("key_y3"));

  $x = instance_cache_fetch(X::class, "key_x3");
  var_dump(!$x);

  $y = instance_cache_fetch(Y::class, "key_y3");
  var_dump(!$y);

// delete на самом деле не удаляет элемент,
// а лишь меняет ему ttl так, что бы следующий fetch вернул false
#ifndef KPHP
  var_dump(true);
  if (0)
#endif
  var_dump(instance_cache_delete("key_x3"));

#ifndef KPHP
  var_dump(true);
  if (0)
#endif
  var_dump(instance_cache_delete("key_y3"));
}

function test_tree() {
  $root = new TreeX (0, [tuple(1, [new TreeX(1)])]);
  var_dump(instance_cache_store("tree_root", $root));

  $cached_root1 = instance_cache_fetch(TreeX::class, "tree_root");
  var_dump(to_array_debug($cached_root1));
}

function test_loop_in_tree() {
  $root = new TreeX(0, [], true);

  $result = instance_cache_store("tree_root_loop", $root);
#ifndef KPHP
  var_dump(true);
  if (false)
#endif
  var_dump($result);

  $cached_root1 = instance_cache_fetch(TreeX::class, "tree_root_loop");
#ifndef KPHP
  var_dump(false);
  if (false)
#endif
  var_dump(!$cached_root1);
}

function test_same_instance_in_array() {
  $y = new Y(10, " <-first");
#ifndef KPHP
  $vector = new VectorY(0, [$y, clone $y, new Y(11, " <-second")]);
  if (false)
#endif
  $vector = new VectorY(0, [$y, $y, new Y(11, " <-second")]);

  var_dump(instance_cache_store("vector", $vector));

  $cached_vector = instance_cache_fetch(VectorY::class, "vector");
  var_dump(to_array_debug($cached_vector));
}

function test_memory_limit_exceed() {
  $vector = new VectorY(1000000);

#ifndef KPHP
  var_dump(false);
  if (false)
#endif
  var_dump(instance_cache_store("large_vector", $vector));
  var_dump(instance_cache_fetch(VectorY::class, "large_vector") ? false : true);
}

function test_with_shape() {
  $a = new HasShape(19);
  instance_cache_store('has_shape', $a);

  $a2 = instance_cache_fetch(HasShape::class, 'has_shape');
  $dump = to_array_debug($a2);
#ifndef KPHP // in KPHP order of keys will differs from php
  $dump['sh'] = ['x' => 19, 'y' => 'y', 'z' => null];
#endif
  var_dump($dump);
  var_dump($a2->sh['x']);
  var_dump($a2->sh['y']);
  var_dump($a2->sh['z']);

  $a = new HasShape(19, true);
  instance_cache_store('has_shape_more', $a);
  $a3 = instance_cache_fetch(HasShape::class, 'has_shape_more');
  $dump = to_array_debug($a3);
#ifndef KPHP // in KPHP order of keys will differs from php
  $dump['sh'] = ['x' => 2, 'y' => 'y', 'z' => [1,2,3]];
#endif
  var_dump($dump);
  var_dump($a3->sh['x']);
  var_dump($a3->sh['y']);
  var_dump($a3->sh['z']);
}

test_empty_fetch();
test_store_fetch() ;
test_mismatch_classes();
test_update_ttl();
test_delete();
test_tree();
test_loop_in_tree();
test_same_instance_in_array();
test_with_shape();
// this test should be the last!
test_memory_limit_exceed();
