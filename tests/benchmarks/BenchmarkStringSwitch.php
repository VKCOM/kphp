<?php

class BenchmarkStringSwitch {
  private static function switchOnlyDefault(string $s) {
    switch ($s) {
    default:
      return 100;
    }
  }

  private static function switch1(string $s) {
    switch ($s) {
    case 'EVENT_TYPE_KIND\UPDATE':
      return 10;
    default:
      return 0;
    }
  }

  private static function switch10longString(string $s) {
    switch ($s) {
    case 'event_document_saved':
      return 10;
    case 'event_user_created':
      return 20;
    case 'event_user_login':
      return 30;
    case 'event_user_friend_request':
      return 40;
    case 'event_user_logout':
      return 50;
    case 'event_group_created':
      return 60;
    case 'event_group_deleted':
      return 65;
    case 'event_group_archived':
      return 70;
    case 'event_unknown':
      return 100;
    case 'event_extra_data':
      return 200;
    }
    return -1;
  }

  private static function switch8oneChar(string $s) {
    switch ($s) {
    case 'x': return 10;
    case 'y': return 20;
    case 'Q': return 30;
    case 'W': return 40;
    case 'E': return 50;
    case 'R': return 60;
    case 'T': return 70;
    case 'Y': return 100;
    default: return 0;
    }
  }

  private static function switchLexerComplex(string $tok) {
    switch ($tok) {
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
      return 2;

    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
      return 3;

    default:
      return -1;
    }
  }

  private static function switch8oneCharNoDefault(string $s) {
    switch ($s) {
    case 'x': return 10;
    case 'y': return 20;
    case 'Q': return 30;
    case 'W': return 40;
    case 'E': return 50;
    case 'R': return 60;
    case 'T': return 70;
    case 'Y': return 100;
    }
    return 0;
  }

  private static function switch6perfectHash(string $s) {
    switch ($s) {
    case 'a': return 1;
    case 'ab': return 12;
    case 'abc': return 123;
    case 'abcd': return 1234;
    case 'abcde1': return 12345;
    case 'abcde2': return 123456;
    default: return 0;
    }
  }

  private static function switch6perfectHashNoDefault(string $s) {
    switch ($s) {
    case 'a': return 1;
    case 'ab': return 12;
    case 'abc': return 123;
    case 'abcd': return 1234;
    case 'abcde1': return 12345;
    case 'abcde2': return 123456;
    }
    return 0;
  }

  private static function switch12perfectHash(string $s) {
    switch ($s) {
    case 'a': return 1;
    case 'ab': return 12;
    case 'abc': return 123;
    case 'abcd': return 1234;
    case 'abcde': return 12345;
    case 'abcdef': return 123456;
    case '?': return -1;
    case '??': return -2;
    case '???': return -3;
    case '????': return -4;
    case '?????': return -5;
    case '??????': return -6;
    default: return 0;
    }
  }

  private static function switch12perfectHashNoDefault(string $s) {
    switch ($s) {
    case 'a': return 1;
    case 'ab': return 12;
    case 'abc': return 123;
    case 'abcd': return 1234;
    case 'abcde': return 12345;
    case 'abcdef': return 123456;
    case '?': return -1;
    case '??': return -2;
    case '???': return -3;
    case '????': return -4;
    case '?????': return -5;
    case '??????': return -6;
    }
    return 0;
  }

  private static function switch6perfectHashWithPrefix(string $s) {
    switch ($s) {
    case 'document_a': return 1;
    case 'document_ab': return 12;
    case 'document_abc': return 123;
    case 'document_abcd': return 1234;
    case 'document_abcde1': return 12345;
    case 'document_abcde2': return 123456;
    default: return 111;
    }
  }

  private static function switch6perfectHashWithPrefixNoDefault(string $s) {
    switch ($s) {
    case 'document_a': return 1;
    case 'document_ab': return 12;
    case 'document_abc': return 123;
    case 'document_abcd': return 1234;
    case 'document_abcde1': return 12345;
    case 'document_abcde2': return 123456;
    }
    return 111;
  }

  private static function switch12perfectHashWithPrefix(string $s) {
    switch ($s) {
    case 'youtube_app_a': return 1;
    case 'youtube_app_ab': return 12;
    case 'youtube_app_abc': return 123;
    case 'youtube_app_abcd': return 1234;
    case 'youtube_app_abcde': return 12345;
    case 'youtube_app_abcdef': return 123456;
    case 'youtube_app_?': return -1;
    case 'youtube_app_??': return -2;
    case 'youtube_app_???': return -3;
    case 'youtube_app_????': return -4;
    case 'youtube_app_?????': return -5;
    case 'youtube_app_??????': return -6;
    case 'youtube_app_???????': return -7;
    case 'youtube_app_????????': return -8;
    default: return 111;
    }
  }

  private static function switch12perfectHashWithPrefixNoDefault(string $s) {
    switch ($s) {
    case 'youtube_app_a': return 1;
    case 'youtube_app_ab': return 12;
    case 'youtube_app_abc': return 123;
    case 'youtube_app_abcd': return 1234;
    case 'youtube_app_abcde': return 12345;
    case 'youtube_app_abcdef': return 123456;
    case 'youtube_app_?': return -1;
    case 'youtube_app_??': return -2;
    case 'youtube_app_???': return -3;
    case 'youtube_app_????': return -4;
    case 'youtube_app_?????': return -5;
    case 'youtube_app_??????': return -6;
    case 'youtube_app_???????': return -7;
    case 'youtube_app_????????': return -8;
    }
    return 111;
  }

  public function benchmarkSwitchOnlyDefault() {
    return self::switchOnlyDefault('string1') + self::switchOnlyDefault('') + self::switchOnlyDefault('some longer string');
  }

  public function benchmarkSwitch1() {
    return self::switch1('EVENT_TYPE_KIND\UPDATE') + self::switch1('EVENT_TYPE_KIND\UPDATE');
  }

  public function benchmarkSwitch1Miss() {
    return self::switch1('') + self::switch1('EVENT_TYPE_KIND\UPDATE2') + + self::switch1('EVENT_TYPE_KIND\UPDAT');
  }

  public function benchmarkSwitch10LongString() {
    return self::switch10longString('event_user_friend_request') +
        self::switch10longString('event_group_archived') +
        self::switch10longString('event_extra_data') +
        self::switch10longString('event_document_saved');
  }

  public function benchmarkSwitch10LongStringMiss() {
    return self::switch10longString('event_user_friend_requesT') +
        self::switch10longString('event_group+archived') +
        self::switch10longString('') +
        self::switch10longString('event_document_unknown');
  }

  public function benchmarkLexerComplex() {
    return self::switchLexerComplex('a') + self::switchLexerComplex('_') + self::switchLexerComplex('A') + self::switchLexerComplex('h');
  }

  public function benchmarkLexerComplexMiss() {
    return self::switchLexerComplex('9') + self::switchLexerComplex('#') + self::switchLexerComplex('?');
  }

  public function benchmarkSwitch8oneChar() {
    return self::switch8oneChar('x') + self::switch8oneChar('W') + self::switch8oneChar('Y') + self::switch8oneChar('y');
  }

  public function benchmarkSwitch8oneCharMiss() {
    return self::switch8oneChar('t') +
        self::switch8oneChar('z') +
        self::switch8oneChar("\0") +
        self::switch8oneChar('longer string');
  }

  public function benchmarkSwitch8oneCharNoDefault() {
    return self::switch8oneCharNoDefault('x') + self::switch8oneCharNoDefault('W') + self::switch8oneCharNoDefault('Y') + self::switch8oneCharNoDefault('y');
  }

  public function benchmarkSwitch8oneCharNoDefaultMiss() {
    return self::switch8oneCharNoDefault('t') +
        self::switch8oneCharNoDefault('z') +
        self::switch8oneCharNoDefault("\0") +
        self::switch8oneCharNoDefault('longer string');
  }

  public function benchmarkSwitch6perfectHash() {
    return self::switch6perfectHash('a') + self::switch6perfectHash('ab') +
        self::switch6perfectHash('abc') + self::switch6perfectHash('abcd') +
        self::switch6perfectHash('abcde1') + self::switch6perfectHash('abcde2');
  }

  public function benchmarkSwitch6perfectHashMiss() {
    return self::switch6perfectHash("a\0") +
        self::switch6perfectHash("\0a") +
        self::switch6perfectHash("abcde3") +
        self::switch6perfectHash("longer string case value miss");
  }

  public function benchmarkSwitch6perfectHashNoDefault() {
    return self::switch6perfectHashNoDefault('a') + self::switch6perfectHashNoDefault('ab') +
        self::switch6perfectHashNoDefault('abc') + self::switch6perfectHashNoDefault('abcd') +
        self::switch6perfectHashNoDefault('abcde1') + self::switch6perfectHashNoDefault('abcde2');
  }

  public function benchmarkSwitch6perfectHashNoDefaultMiss() {
    return self::switch6perfectHashNoDefault("a\0") +
        self::switch6perfectHashNoDefault("\0a") +
        self::switch6perfectHashNoDefault("abcde3") +
        self::switch6perfectHashNoDefault("longer string case value miss");
  }

  public function benchmarkSwitch12perfectHash() {
    return self::switch12perfectHash('a') + self::switch12perfectHash('ab') +
        self::switch12perfectHash('abc') + self::switch12perfectHash('abcd') +
        self::switch12perfectHash('???') + self::switch12perfectHash('??????');
  }

  public function benchmarkSwitch12perfectHashMiss() {
    return self::switch12perfectHash("bad") +
        self::switch12perfectHash("") +
        self::switch12perfectHash("???????") +
        self::switch12perfectHash("longer string case value miss");
  }

  public function benchmarkSwitch12perfectHashNoDefault() {
    return self::switch12perfectHashNoDefault('a') + self::switch12perfectHashNoDefault('ab') +
        self::switch12perfectHashNoDefault('abc') + self::switch12perfectHashNoDefault('abcd') +
        self::switch12perfectHashNoDefault('???') + self::switch12perfectHashNoDefault('??????');
  }

  public function benchmarkSwitch12perfectHashNoDefaultMiss() {
    return self::switch12perfectHashNoDefault("bad") +
        self::switch12perfectHashNoDefault("") +
        self::switch12perfectHashNoDefault("???????") +
        self::switch12perfectHashNoDefault("longer string case value miss");
  }

  public function benchmarkSwitch6perfectHashWithPrefix() {
    return self::switch6perfectHashWithPrefix('document_a') + self::switch6perfectHashWithPrefix('document_ab') +
        self::switch6perfectHashWithPrefix('document_abc') + self::switch6perfectHashWithPrefix('document_abcd') +
        self::switch6perfectHashWithPrefix('document_abcde1') + self::switch6perfectHashWithPrefix('document_abcde2');
  }

  public function benchmarkSwitch6perfectHashWithPrefixMiss() {
    return self::switch6perfectHashWithPrefix("document_a\0") +
        self::switch6perfectHashWithPrefix("\0document_a") +
        self::switch6perfectHashWithPrefix("document_abbbb0") +
        self::switch6perfectHashWithPrefix("abcde3") +
        self::switch6perfectHashWithPrefix("longer string case value miss");
  }

  public function benchmarkSwitch6perfectHashWithPrefixNoDefault() {
    return self::switch6perfectHashWithPrefixNoDefault('document_a') + self::switch6perfectHashWithPrefixNoDefault('document_ab') +
        self::switch6perfectHashWithPrefixNoDefault('document_abc') + self::switch6perfectHashWithPrefixNoDefault('document_abcd') +
        self::switch6perfectHashWithPrefixNoDefault('document_abcde1') + self::switch6perfectHashWithPrefixNoDefault('document_abcde2');
  }

  public function benchmarkSwitch6perfectHashWithPrefixNoDefaultMiss() {
    return self::switch6perfectHashWithPrefixNoDefault("document_a\0") +
        self::switch6perfectHashWithPrefixNoDefault("\0document_a") +
        self::switch6perfectHashWithPrefixNoDefault("document_abbbb0") +
        self::switch6perfectHashWithPrefixNoDefault("abcde3") +
        self::switch6perfectHashWithPrefixNoDefault("longer string case value miss");
  }

  public function benchmarkSwitch12perfectHashWithPrefix() {
    return self::switch12perfectHashWithPrefix('youtube_app_a') + self::switch12perfectHashWithPrefix('youtube_app_ab') +
        self::switch12perfectHashWithPrefix('youtube_app_abc') + self::switch12perfectHashWithPrefix('youtube_app_abcd') +
        self::switch12perfectHashWithPrefix('youtube_app_???') + self::switch12perfectHashWithPrefix('youtube_app_????????');
  }

  public function benchmarkSwitch12perfectHashWithPrefixMiss() {
    return self::switch12perfectHashWithPrefix("youtube_app_bad") +
        self::switch12perfectHashWithPrefix("") +
        self::switch12perfectHashWithPrefix("???????youtube_app") +
        self::switch12perfectHashWithPrefix('youtube_app_???????0') +
        self::switch12perfectHashWithPrefix("longer string case value miss");
  }

  public function benchmarkSwitch12perfectHashWithPrefixNoDefault() {
    return self::switch12perfectHashWithPrefixNoDefault('youtube_app_a') + self::switch12perfectHashWithPrefixNoDefault('youtube_app_ab') +
        self::switch12perfectHashWithPrefixNoDefault('youtube_app_abc') + self::switch12perfectHashWithPrefixNoDefault('youtube_app_abcd') +
        self::switch12perfectHashWithPrefixNoDefault('youtube_app_???') + self::switch12perfectHashWithPrefixNoDefault('youtube_app_????????');
  }

  public function benchmarkSwitch12perfectHashWithPrefixNoDefaultMiss1() {
    return self::switch12perfectHashWithPrefixNoDefault("youtube_app_bad") +
        self::switch12perfectHashWithPrefixNoDefault("") +
        self::switch12perfectHashWithPrefixNoDefault("???????youtube_app") +
        self::switch12perfectHashWithPrefixNoDefault("longer string case value miss");
  }

  public function benchmarkSwitch12perfectHashWithPrefixNoDefaultMiss2() {
    return self::switch12perfectHashWithPrefixNoDefault("youtube_app_a0") +
        self::switch12perfectHashWithPrefixNoDefault("youtube_app_a1");
  }

  public function benchmarkSwitchCombined() {
    static $hits = ['a', 'abc', 'abcdef', '?', '????', '??????'];
    static $misses = ['_', '349x', 'he11o', 'id10', '', 'a?b?c?d?'];
    $total = 0;
    for ($i = 0; $i < 100; $i++) {
      for ($j = 0; $j < count($hits); $j++) {
        $total += self::switch12perfectHash($hits[$j]);
        $total += self::switch12perfectHash($misses[$j]);
      }
    }
    return $total;
  }
}
