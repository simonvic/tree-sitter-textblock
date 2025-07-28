#include "tree_sitter/alloc.h"
#include "tree_sitter/parser.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

enum TokenType { _DUMMY, INCIDENTAL_WS, TEXTBLOCK_FRAGMENT };

struct Scanner {
	/**
	 * Count of whitespaces to be considered incidental
	 */
	int incidentalWSWidth;

	/**
	 * Character that is considered indentation leader (used for code
	 * indentation).
	 * All characters equal to the indentation leader found at the beginning of
	 * a line will be considered as incidental whitespaces
	 */
	char indentationLeader;

	/**
	 * During first line parsing, incidental whitespaces calcuation is ignored
	 */
	bool isFirstLine;
};

static inline void consume(TSLexer* lexer) {
	lexer->advance(lexer, false);
}

static inline void skip(TSLexer* lexer) {
	lexer->advance(lexer, true);
}

static inline bool eof(TSLexer* lexer) {
	return lexer->eof(lexer);
}

void* tree_sitter_textblock_external_scanner_create(void) {
	struct Scanner* scanner = ts_malloc(sizeof(struct Scanner));
	scanner->incidentalWSWidth = -1;
	scanner->indentationLeader = '\0';
	scanner->isFirstLine = true;
	return scanner;
}

void tree_sitter_textblock_external_scanner_destroy(void* payload) {
	free(payload);
}

unsigned tree_sitter_textblock_external_scanner_serialize(void* payload,
                                                          char* buffer) {
	struct Scanner* scanner = (struct Scanner*)payload;
	size_t size = 0;
	buffer[size++] = (char)scanner->incidentalWSWidth;
	buffer[size++] = (char)scanner->indentationLeader;
	buffer[size++] = (char)scanner->isFirstLine;
	return size;
}

void tree_sitter_textblock_external_scanner_deserialize(void* payload,
                                                        const char* buffer,
                                                        unsigned length) {
	if (length <= 0) return;
	struct Scanner* scanner = (struct Scanner*)payload;
	size_t size = 0;
	scanner->incidentalWSWidth = (int)buffer[size++];
	scanner->indentationLeader = (char)buffer[size++];
	scanner->isFirstLine = (bool)buffer[size++];
	assert(size == length);
}

static inline bool is_indent_char(char c) {
	return c == ' ' || c == '\t';
}

bool tree_sitter_textblock_external_scanner_scan(void* payload, TSLexer* lexer,
                                                 const bool* valid_symbols) {
	struct Scanner* scanner = (struct Scanner*)payload;

	if (eof(lexer)) return false;

	// first scan to count minimum amount of incidental whitespaces
	if (scanner->incidentalWSWidth == -1) {
	    // TODO: replace dummy with first_line rule
		lexer->result_symbol = _DUMMY;
		lexer->mark_end(lexer);
		while (scanner->isFirstLine && !eof(lexer)) {
			if (lexer->lookahead == '\n') {
				skip(lexer);
				scanner->isFirstLine = false;
				break;
			}
			skip(lexer);
		}
		if (!is_indent_char(lexer->lookahead)) {
			// no incidental whitespaces
			scanner->incidentalWSWidth = 0;
			return true;
		}
		scanner->indentationLeader = lexer->lookahead;
		int minWidth = -1;
		while (!eof(lexer)) {
			int width = 0;
			// count whitespaces
			while (lexer->lookahead == scanner->indentationLeader &&
			       !eof(lexer)) {
				width++;
				skip(lexer);
			}
			if (width < minWidth || minWidth == -1) {
				minWidth = width;
			}
			// skip "content" unti new line
			while (!eof(lexer)) {
				if (lexer->lookahead == '\n') {
					skip(lexer);
					break;
				}
				skip(lexer);
			}
		}
		scanner->incidentalWSWidth = minWidth;
		// reset first line to true for the actual parse
		scanner->isFirstLine = true;
		return true;
	}

	// parse first whitespaces as incidental
	// incidental whitespaces are ignored in the first line
	if (valid_symbols[INCIDENTAL_WS] && !scanner->isFirstLine &&
	    scanner->incidentalWSWidth >= 1) {
		lexer->result_symbol = INCIDENTAL_WS;
		int parsedIncidentalWS = 0;
		while (parsedIncidentalWS < scanner->incidentalWSWidth) {
			consume(lexer);
			parsedIncidentalWS++;
		}
		return true;
	}

	// parse rest of the text up to newline as content
	if (valid_symbols[TEXTBLOCK_FRAGMENT]) {
		lexer->result_symbol = TEXTBLOCK_FRAGMENT;
		while (!eof(lexer)) {
			if (lexer->lookahead == '\n') {
				consume(lexer);
				scanner->isFirstLine = false;
				break;
			}
			consume(lexer);
		}
		return true;
	}

	return false;
}
