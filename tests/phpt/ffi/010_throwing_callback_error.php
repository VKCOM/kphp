@kphp_should_fail k2_skip
/FFI callback should not throw/
/Throw chain: function\(\$id\) -> callable\(\):int -> function\(\) -> function\(\) -> throwing/
<?php

function throwing() {
  throw new \Exception('oops');
}

$cdef = FFI::cdef('
#define FFI_SCOPE "example"
void register_func(int, int (*g) (int));
');

class FuncContainer {
  /** @var (callable():int)[] */
  public static $funcs = [];
}

/**
 * @param ffi_scope<example> $cdef
 * @param callable():int $f
 */
function register_ffi_callback($cdef, $f) {
  $id = count(count(FuncContainer::$funcs));
  FuncContainer::$funcs[] = $f;
  $cdef->register_func($id, static function ($id) {
     $orig_f = FuncContainer::$funcs[$id];
     return $orig_f();
  });
}

register_ffi_callback($cdef, function() {
  throwing();
  return 0;
});
