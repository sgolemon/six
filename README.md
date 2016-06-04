## Fun with Unicode

This extension is a recreation of a [patch](http://blog.golemon.com/2007/07/fun-with-unicode.html)
I was playing with about nine years ago while we were working on PHP6, then subsequently lost.

The following table of symbols and their corresponding translations are currently supported:

Six syntax | PHP Syntax | Symbol Name
--- | --- | ---
¼ | (1/4) | VULGAR FRACTION ONE QUARTER
½ | (1/2) | VULGER FRACTION ONE HALF
¾ | (3/4) | VULGAR FRACTION THREE QUARTERS
⅓ | (1/3) | VULGAR FRACTION ONE THIRD
→ | -> | RIGHTWARDS ARROW
⇒ | => | RIGHTWARDS DOUBLE ARROW
⇔ | <=> | LEFT RIGHT DOUBLE ARROW
≈ | == | ALMOST EQUAL TO
≡ | === | IDENTICAL TO
≤ | <= | LESS-THAN OR EQUAL TO
≥ | >= | GREATER-THAN OR EQUAL TO
💩 | throw new | PILE OF POO EMOJI

### Implementation Details

Unlike that patch, this works as an independent extension, and doesn't require recompiling PHP.
Unfortuntely, it accomplishes this is a pretty heavy-handed way,
but doing the equivalent of the following, roughly translated, and largely untested in PHP code:

```
function rewrite_token(string $text) {
  static $replacements = [
     "\xC2\xBC" => "(1/4)", /* U+00BC VULGAR FRACTION ONE QUARTER */
     "\xC2\xBD" => "(1/2)", /* U+00BD VULGAR FRACTION ONE HALF */
     "\xC2\xBE" => "(3/4)", /* U+00BE VULGAR FRACTION THREE QUARTERS */

     "\xE2\x85\x93" => "(1/3)", /* U+2153 VULGAR FRACTION ONE THIRD */
     "\xE2\x86\x92" => "->", /* U+2192 RIGHTWARDS ARROW */
     "\xE2\x87\x92" => "=>", /* U+21D2 RIGHTWARDS DOUBLE ARROW */
     "\xE2\x87\x94" => "<=>", /* U+21D4 LEFT RIGHT DOUBLE ARROW */
     "\xE2\x89\x88" => "==", /* U+2248 ALMOST EQUAL TO */
     "\xE2\x89\xA1" => "===", /* U+2261 IDENTICAL TO */
     "\xE2\x89\xA4" => "<=", /* U+2264 LESS-THAN OR EQUAL TO */
     "\xE2\x89\xA5" => ">=", /* U+2265 GREATER-THAN OR EQUAL TO */

     "\xF0\x9F\x92\xA9" => "throw new", /* U+1F4A9 PILE OF POO EMOJI */
  ];

  $ret = '';
  foreach ($repalcements as $search => $replace) {
    $pos = strpos($text, $search);
    if ($pos === false) continue;
    if ($pos > 0) {
      $ret .= rewrite_token(substr($text, 0, $pos));
      $text = substr($text, $pos);
    }
    $ret .= $replace;
    $text = substr($text, strlen($search));
  }
  return $ret . $text;
}

function require(string $filename) {
  $script = '';
  foreach (token_get_all(file_get_contents($filename)) as $token) {
    if (is_string($token)) {
      $script .= $token;
      continue;
    }
    if (in_array($token[0], [T_STRING, T_VARIABLE, T_STRING_VARIABLE])) {
      $script .= rewrite_token($token[1]);
      continue;
    }
    $script .= $token[1];
  }
  return eval($filename);
}
```

Obviously this is very inefficient and wasteful.  A better implementation, while still working with the existing Zend Extension API, would be to run two parallel tokenizers with the translator feeding the compiler live.  Unfortunately, the existing tokenizer is REALLY not designed to have two copies running at the same time in the same thread.  It could probably be done by TSRM_BULK swapping scanner globals, but I can't quite be bothered to try that today.
