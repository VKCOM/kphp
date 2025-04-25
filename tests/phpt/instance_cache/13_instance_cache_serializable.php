@ok non-idempotent
<?php

require_once 'kphp_tester_include.php';

/** @kphp-immutable-class 
 *  @kphp-serializable
*/
class X {
  /** @var int 
   *  @kphp-serialized-field 0
  */
  public $x_int = 1;

  /** @var string 
   *  @kphp-serialized-field 1
  */
  public $x_str = "hello";

  /** @var int[] 
   *  @kphp-serialized-field 2
  */
  public $x_array = [1, 2, 3, 4];

  /** @var mixed[] 
   *  @kphp-serialized-field 3
  */
  public $x_array_var = ["foo", 12, [123]];
}

/** @kphp-immutable-class 
 *  @kphp-serializable
*/
class Y {
  /** @var X 
   *  @kphp-serialized-field 0
  */
  public $x_instance;

  /** @var string 
   *  @kphp-serialized-field 1
   * 
  */
  public $y_string;

  /** @var int[] 
   *  @kphp-serialized-field 2
   * 
  */
  public $y_array;

  /** @var mixed[] 
   *  @kphp-serialized-field 3
   * 
  */
  public $y_array_var;

  /** @var string|false 
   *  @kphp-serialized-field 4
   * 
  */
  public $y_string_or_false;

  /** @var tuple<string | false, mixed[], int[], string, X> 
   *  @kphp-serialized-field 5
   * 
  */
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

/** @kphp-immutable-class 
 *  @kphp-serializable
*/
class TreeX {
  /** @var int
   *  @kphp-serialized-field 0
  */
  public $value = 0;
  /** @var tuple<int, TreeX[]> [] 
   *  @kphp-serialized-field 1
  */
  public $children = [];

  public function __construct(int $value, array $children = [], bool $make_loop = false) {
    $this->value = $value;
    $this->children = $children;
    if ($make_loop) {
      $this->children[] = tuple(1, [$this]);
    }
  }
}

/** @kphp-immutable-class
 *  @kphp-serializable
*/
class VectorY {
  /** @var Y[] 
   *  @kphp-serialized-field 0
  */
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
}

function test_tree() {
  $root = new TreeX (0, [tuple(1, [new TreeX(1)])]);
  var_dump(instance_cache_store("tree_root", $root));

  $cached_root1 = instance_cache_fetch(TreeX::class, "tree_root");
  var_dump(to_array_debug($cached_root1));
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


// test_empty_fetch();
// test_store_fetch() ;
// test_mismatch_classes();
// test_update_ttl();
// test_delete();
// test_tree();
// test_loop_in_tree();
test_same_instance_in_array();

