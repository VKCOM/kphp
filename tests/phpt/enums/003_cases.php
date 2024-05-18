@ok php8
<?php

enum Suit {
  case Hearts;
  case Diamonds;
  case Clubs;
  case Spades;
}

foreach (Suit::cases() as $cur_suit) {
    var_dump($cur_suit->name);
}

enum Foo {
    case Bar;
    case Baz;
}

enum Single {
    case Single;
}

enum EmptyEnum {
}

var_dump(count(Foo::cases()));
var_dump(count(Single::cases()));
var_dump(count(EmptyEnum::cases()));
