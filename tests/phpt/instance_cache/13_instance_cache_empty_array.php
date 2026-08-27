@ok non-idempotent
<?php

require_once 'kphp_tester_include.php';

#ifndef KPHP
function get_reference_counter($a)
{
    return 1;
}
#endif

/**
 * @kphp-immutable-class
 */
class EmptyArrayHolder
{
    /** @var int[] */
    public array $empty_array = [];
}

function test_store_fetch_empty_array()
{
    var_dump(instance_cache_store("test_store_fetch_empty_array", new EmptyArrayHolder));
    $fetched = instance_cache_fetch(EmptyArrayHolder::class, "test_store_fetch_empty_array");
    if ($fetched !== null) {
        var_dump(empty($fetched->empty_array));
    }
}

/**
 * @kphp-immutable-class
 */
class EmptyArraysHolder
{
    /** @var int[] */
    public array $empty_int_array = [];
    /** @var string[] */
    public array $empty_string_array = [];
    /** @var EmptyArrayHolder[] */
    public array $empty_instance_array = [];
    /** @var int[][] */
    public array $array_with_empty_nested = [[], [1, 2, 3], []];
    /** @var mixed */
    public $mixed_empty_array = [];
}

function test_store_fetch_various_empty_arrays()
{
    var_dump(instance_cache_store("test_store_fetch_various_empty_arrays", new EmptyArraysHolder));
    $fetched = instance_cache_fetch(EmptyArraysHolder::class, "test_store_fetch_various_empty_arrays");
    if ($fetched === null) {
        var_dump(false);
        return;
    }
    var_dump(empty($fetched->empty_int_array));
    var_dump(empty($fetched->empty_string_array));
    var_dump(empty($fetched->empty_instance_array));
    var_dump(count($fetched->array_with_empty_nested));
    var_dump(empty($fetched->array_with_empty_nested[0]));
    var_dump($fetched->array_with_empty_nested[1]);
    var_dump(empty($fetched->array_with_empty_nested[2]));
    var_dump(is_array($fetched->mixed_empty_array));
    var_dump(empty($fetched->mixed_empty_array));
}

function test_store_fetch_empty_array_twice_independent_keys()
{
    var_dump(instance_cache_store("test_store_fetch_empty_array_twice_key1", new EmptyArrayHolder));
    var_dump(instance_cache_store("test_store_fetch_empty_array_twice_key2", new EmptyArraysHolder));

    $holder1 = instance_cache_fetch(EmptyArrayHolder::class, "test_store_fetch_empty_array_twice_key1");
    $holder2 = instance_cache_fetch(EmptyArraysHolder::class, "test_store_fetch_empty_array_twice_key2");
    if ($holder1 !== null) {
        var_dump(empty($holder1->empty_array));
    }
    if ($holder2 !== null) {
        var_dump(empty($holder2->empty_int_array));
    }
}

function test_empty_array_refcnt_preserved()
{
    $const_empty_array = [];
    $expected_refcnt = get_reference_counter($const_empty_array);

    instance_cache_store("test_empty_array_refcnt_preserved", new EmptyArraysHolder);
    $fetched = instance_cache_fetch(EmptyArraysHolder::class, "test_empty_array_refcnt_preserved");
    if ($fetched === null) {
        var_dump(false);
        return;
    }

    var_dump($expected_refcnt === get_reference_counter($fetched->empty_int_array));
    var_dump($expected_refcnt === get_reference_counter($fetched->empty_string_array));
    var_dump($expected_refcnt === get_reference_counter($fetched->empty_instance_array));
    var_dump($expected_refcnt === get_reference_counter($fetched->array_with_empty_nested[0]));
    var_dump($expected_refcnt === get_reference_counter($fetched->array_with_empty_nested[2]));
    var_dump($expected_refcnt === get_reference_counter($fetched->mixed_empty_array));
}

test_store_fetch_empty_array();
test_store_fetch_various_empty_arrays();
test_store_fetch_empty_array_twice_independent_keys();
test_empty_array_refcnt_preserved();
