@ok php8
<?php

class Boo {}
interface Goo {}
interface Doo {}

#[SimpleAttribute]
class Foo1 {}

#[AttributeWithArgs(100, SomeClass::class)]
class Foo2 extends Boo {}

#[FirstAttribute(100, SomeClass::class), SecondAttribute()]
class Foo3 implements Goo {}

#[FirstAttribute(100, SomeClass::class), SecondAttribute()]
abstract class Foo4 implements Goo, Doo {}

#[
	FirstLineAttribute(100, SomeClass::class),
	SecondLineAttribute()
]
class Foo5 {}

#[FirstAttribute(100, SomeClass::class), SecondAttribute()]
#[SecondGroupFirstAttribute(SomeClass::class), SecondGroupSecondAttribute]
class Foo6 extends Boo implements Goo {}

#[\WithSlash]
class Foo7 {}

#[\F\Q\N]
class Foo8 {}

#[\WithSlash]
final class Foo9 extends Boo implements Goo, Doo {}
