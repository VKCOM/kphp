@ok php8
<?php

interface Boo {}
interface Goo {}
interface Doo {}

#[SimpleAttribute]
interface Foo1 {}

#[AttributeWithArgs(100, SomeClass::class)]
interface Foo2 extends Boo {}

#[FirstAttribute(100, SomeClass::class), SecondAttribute()]
interface Foo3 {}

#[
	FirstLineAttribute(100, SomeClass::class),
	SecondLineAttribute()
]
interface Foo4 {}

#[FirstAttribute(100, SomeClass::class), SecondAttribute()]
#[SecondGroupFirstAttribute(SomeClass::class), SecondGroupSecondAttribute]
interface Foo5 extends Boo, Goo {}

#[\WithSlash]
interface Foo6 {}

#[\F\Q\N]
interface Foo7 {}
