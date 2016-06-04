--TEST--
Six basic ops tests
--INI--
precision=14
--FILE--
<?php

var_dump(¼);
var_dump(½);
var_dump(¾);
var_dump(⅓);

$o = (object)array(
  'a' ⇒ "Apple",
);

var_dump($o → a);
var_dump("banana" ⇔ "carrot");
var_dump("banana" ⇔ "banana");
var_dump("carrot" ⇔ "banana");

var_dump(0 ≈ false);
var_dump(1 ≡ 1);
var_dump(4 ≤ 5);
var_dump(4 ≥ 6);

try {
  💩 Exception('eeewwwww');
} catch (Exception $e) {
  echo $e->getMessage();
}
--EXPECT--
float(0.25)
float(0.5)
float(0.75)
float(0.33333333333333)
string(5) "Apple"
int(-1)
int(0)
int(1)
bool(true)
bool(true)
bool(true)
bool(false)
eeewwwww
