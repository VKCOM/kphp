@ok
<?php

interface IBB {}
interface IB extends IBB {}
interface IA {}
interface IC {}

class A implements IA, IB, IC {}
class B implements IA, IBB {}

/**@var IBB*/
$x = new A();
$x = new B();

