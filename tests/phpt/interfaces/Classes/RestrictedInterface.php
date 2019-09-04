<?php
namespace Classes;

/**
 * @kphp-infer
 */
interface RestrictedInterface {
    /**
     * @param mixed $x
     * @return int
     */
    public function run($x);
}
