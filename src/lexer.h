#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>

enum TokenType;

typedef struct {
	char* input;
	size_t position;
	uint8_t ch;
} Lexer;

enum TokenType {
	LETTER,
	NUMBER,
	SYMBOL,
	SPACE,
	OTHER,
	EOF,
};

typedef struct {
	enum TokenType token_type;
	char content[16];
} Token;

Token next_token(Lexer lexer);
uint8_t peek(Lexer lexer);


#endif
