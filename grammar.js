/**
 * @file Text block and multiline string parser
 * @author simonvic <simonvic.dev@gmail.com>
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

module.exports = grammar({
  name: "textblock",

  externals: $ => [$._dummy, $.incidental_ws, $.textblock_fragment],

  rules: {
    textblock: $ => seq(
      optional($._dummy),
      repeat1(seq(optional($.incidental_ws), $.textblock_fragment)),
      optional($.incidental_ws),
    )
  }
});
