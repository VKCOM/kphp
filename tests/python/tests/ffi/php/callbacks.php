<?php

require_once __DIR__ . '/include/common.php';

kphp_load_vector_lib();

/**
 * @param int $i
 * @param ffi_cdata<vector, struct Vector2*> $v
 * @kphp-required
 */
function callback_func($i, $v) {
  $v->x = (float)($i * 4);
  $v->y = (float)$i + 100;
}

function test_vector2_walk() {
  // check that void callbacks work
  $lib = FFI::scope('vector');
  $array = $lib->new('struct Vector2Array');
  $vec_size = 10;
  $lib->vector2_array_alloc(FFI::addr($array), $vec_size);

  // null as C function callback
  print_vec_array($array);
  $lib->vector2_array_walk(FFI::addr($array), null);
  print_vec_array($array);

  $lib->vector2_array_walk(FFI::addr($array), function ($i, $v) {
    $v->x = (float)($i * 2);
    $v->y = (float)$i;
  });
  for ($i = 0; $i < $vec_size; $i++) {
    $v = $lib->vector2_array_get(FFI::addr($array), $i);
    var_dump([$i => "$v->x $v->y"]);
  }
  $v = $lib->vector2_array_get(FFI::addr($array), 9);

  // test non-lambda callbacks
  $lib->vector2_array_walk(FFI::addr($array), 'callback_func');
  print_vec_array($array);
  $lib->vector2_array_walk(FFI::addr($array), 'FFIUtils::callback_method');
  print_vec_array($array);
  $lib->vector2_array_walk(FFI::addr($array), ['FFIUtils', 'callback_method']);
  print_vec_array($array);

  $lib->vector2_array_free(FFI::addr($array));
}

function test_vector2_map() {
  // check that struct-returning callbacks work
  $lib = FFI::scope('vector');
  $array = $lib->new('struct Vector2Array');
  $vec_size = 5;
  $lib->vector2_array_alloc(FFI::addr($array), $vec_size);

  $vec = $lib->new('struct Vector2');
  $vec->x = 11.0;
  $lib->vector2_array_set(FFI::addr($array), 1, $vec);
  $vec->x = 2.5;
  $lib->vector2_array_set(FFI::addr($array), 2, $vec);

  $new_array = $lib->vector2_array_map(FFI::addr($array), function ($v) {
    $new_vec = clone $v;
    $new_vec->x = $v->x + 1;
    $new_vec->y = 14.5;
    return $new_vec;
  });
  print_vec_array($new_array);

  // also check that it's possible to use FFI scope from the callbacks
  $new_array2 = $lib->vector2_array_map(FFI::addr($new_array), function ($v) {
    $new_vec = vector_lib()->new('struct Vector2');
    $new_vec->x = $v->x * 3;
    $new_vec->y = $v->x + $v->y;
    return $new_vec;
  });
  print_vec_array($new_array2);

  print_vec_array($array);

  $lib->vector2_array_free(FFI::addr($array));
  $lib->vector2_array_free(FFI::addr($new_array));
  $lib->vector2_array_free(FFI::addr($new_array2));
}

function test_from_class_methods() {
  // check that lambdas can be created from inside the class methods
  $lib = FFI::scope('vector');
  $array = $lib->new('struct Vector2Array');
  $vec_size = 3;
  $lib->vector2_array_alloc(FFI::addr($array), $vec_size);

  $vec = $lib->new('struct Vector2');
  $vec->x = 1.1;
  $lib->vector2_array_set(FFI::addr($array), 1, $vec);
  $vec->x = 2.2;
  $lib->vector2_array_set(FFI::addr($array), 2, $vec);

  Example::test_from_static_method($array);
  $example = new Example();
  $example->test_from_instance_method($array);

  $lib->vector2_array_free(FFI::addr($array));
}

function test_functype_without_typedef() {
  // check that inline function pointer declarations in signatures work too

  $lib = FFI::scope('vector');
  $array_size = 3;

  $array1 = $lib->vector2f_array_create($array_size, function ($v) {
    $v->x = 3.0;
    $v->y = -1.0;
  });
  for ($i = 0; $i < $array_size; $i++) {
    $v = $lib->vector2f_array_get(FFI::addr($array1), $i);
    var_dump("$v->x $v->y");
  }

  $array2 = $lib->vector2f_array_create_index($array_size, function (int $i, $v) {
    $v->x = (float)($i + 10);
    $v->y = (float)($i - 10);
  });
  for ($i = 0; $i < $array_size; $i++) {
    $v = $lib->vector2f_array_get(FFI::addr($array2), $i);
    var_dump("$v->x $v->y");
  }

  $lib->vector2f_array_free(FFI::addr($array1));
  $lib->vector2f_array_free(FFI::addr($array2));
}

class Example {
  /** @param ffi_cdata<vector, struct Vector2Array> $arr */
  public static function test_from_static_method($arr) {
    $result = vector_lib()->vector2_array_reduce(FFI::addr($arr), function ($v) {
      return $v->x;
    });
    var_dump($result);
  }

  /** @param ffi_cdata<vector, struct Vector2Array> $arr */
  public function test_from_instance_method($arr) {
    $result = vector_lib()->vector2_array_reduce(FFI::addr($arr), function ($v) {
      return $v->x;
    });
    var_dump($result);

    // For instance methods, it makes sense to use 'static' lambdas
    $result = vector_lib()->vector2_array_reduce(FFI::addr($arr), static function ($v) {
      return $v->x * 4.5;
    });
    var_dump($result);
  }
}

class FFIUtils {
  /** @var ffi_scope<util_cdef> */
  public static $cdef;

  public static function init() {
    FFIUtils::$cdef = FFI::cdef('
#define FFI_SCOPE "util_cdef"
struct UserData {
  double a[8];
};');
  }

  /**
   * @param int $i
   * @param ffi_cdata<vector, struct Vector2*> $v
   * @kphp-required
   */
  public static function callback_method($i, $v) {
    $v->x = (float)($i * 8);
    $v->y = (float)$i + 300;
  }
}

function test_user_data() {
  $lib = FFI::scope('vector');

  $user_data = FFIUtils::$cdef->new('struct UserData');
  for ($i = 0; $i < 8; $i++) {
    ffi_array_set($user_data->a, $i, (float)($i + 2.0));
  }

  $array = $lib->new('struct Vector2Array');
  $vec_size = 4;
  $lib->vector2_array_alloc(FFI::addr($array), $vec_size);

  $lib->vector2_array_walk_ud(FFI::cast('void*', FFI::addr($user_data)), FFI::addr($array), function ($ud, $i, $v) {
    $user_data = FFIUtils::$cdef->cast('struct UserData*', $ud);
    $v->x = ffi_array_get($user_data->a, ($i*2)+0);
    $v->y = ffi_array_get($user_data->a, ($i*2)+1);
  });
  print_vec_array($array);

  $lib->vector2_array_free(FFI::addr($array));
}

function test_cstring() {
  $lib = FFI::scope('vector');

  $res1 = $lib->string_c_arg(function ($s) {
    return strlen($s);
  });
  var_dump($res1);
}

FFIUtils::init();

test_vector2_walk();
test_vector2_map();
test_from_class_methods();
test_functype_without_typedef();
test_user_data();
test_cstring();
