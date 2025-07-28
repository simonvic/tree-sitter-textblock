/**
 * @file Text block and multiline string parser
 * @author simonvic <simonvic.dev@gmail.com>
 * @license MIT
 */

/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

module.exports = grammar({
  name: "textblock",

  externals: $ => [$._dummy, $.incidental_ws, $.content],

  rules: {
    text: $ => seq(
      optional($._dummy),
      repeat1(seq(optional($.incidental_ws), $.content)),
      optional($.incidental_ws),
    )
  }
});
