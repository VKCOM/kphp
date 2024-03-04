<?php

interface Simple {
}

interface Complex extends Simple {
}

/** @kphp-immutable-class */
class SimpleType implements Simple {
    public string $name;
    function __construct() {
        $this->name = "simple name";
    }
}

/** @kphp-immutable-class */
class ComplexType implements Complex {
    public string $name;
    function __construct() {
        $this->name = "complex name";
    }
}

/** @kphp-immutable-class */
class CompletelyComplexType extends ComplexType {
    public string $position;
    function __construct() {
        parent::__construct();
        $this->position = "position";
    }
}

function save_as_base(string $key, Simple $obj) {
    $flag = instance_cache_store($key, $obj);
    if ($flag) {
        fprintf(STDERR, "successfully store $key\n");
    }
}

function test_polymorphic_store() {
    $simple = new SimpleType();
    echo "simple name - " .$simple->name ."\n";
    save_as_base("SimpleType", $simple);
    save_as_base("ComplexType", new ComplexType());
    save_as_base("CompletelyComplexType", new CompletelyComplexType());
}

function test_polymorphic_fetch_and_modify() {
    $simple = instance_cache_fetch(Simple::class, "SimpleType");
    $complex = instance_cache_fetch(Simple::class, "ComplexType");
    $completelyComplex = instance_cache_fetch(Simple::class, "CompletelyComplexType");
    if ($simple instanceof SimpleType) {
        fprintf(STDERR, "successfully fetch SimpleType\n");
        echo $simple->name;
        if (strcmp($simple->name, "simple name") !== 0) {
            critical_error("expected 'simple name' but got $simple->name\n");
        }
        $copyName = $simple->$name;
        $copyName .= "used name";
    }
    if ($complex instanceof ComplexType) {
        fprintf(STDERR, "successfully fetch ComplexType\n");
         if (strcmp($complex->name, "complex name") !== 0) {
             critical_error("expected 'complex name' but got $complex->name\n");
         }
         $copyName = $complex->$name;
         $copyName .= "used name";
    }
    if ($completelyComplex instanceof CompletelyComplexType) {
         fprintf(STDERR, "successfully fetch CompletelyComplexType\n");
         if (strcmp($completelyComplex->position, "position") !== 0) {
             critical_error("expected 'position' but got $completelyComplex->position\n");
         }
         $copyPosition = $completelyComplex->$position;
         $copyPosition .= "used position";
    }

}
