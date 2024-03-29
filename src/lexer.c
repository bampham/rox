#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

#include "lexer.h"

Token next_token(Lexer* lexer);
static char* _capture_token(Lexer* lexer); 
static void _forward(Lexer* lexer);

Token* lex(char* input, intmax_t file_size, size_t* token_count) {

    // initiate lexer
    Lexer lexer;
    lexer.input = (char*)malloc(file_size);
    strcpy(lexer.input, input);
    lexer.position = 0;
    lexer.length = strlen(lexer.input);
    lexer.ch = lexer.input[lexer.position];

    /* creating pointer to the stored tokens and keeping
     * track of the size 
     */

    Token* tokens = (Token*)malloc(sizeof(Token)); 
    if (tokens == NULL) {
        perror("could not allocate memory for tokens");
        free(lexer.input);
        return NULL;
    }

    Token token;

    while ((token = next_token(&lexer)).type != EOF_TYPE) {
        tokens[*token_count] = token;
        ++(*token_count);

        tokens = (Token*)realloc(tokens, sizeof(Token) * (*token_count + 1)); 
        if (tokens == NULL) {
            perror("could not allocate memory for tokens");
            free(lexer.input);
            return NULL;
        }
    }
    
    token.type = EOF_TYPE;
    tokens[*token_count] = token;
    ++(*token_count);

    // for EOF_TYPE
    tokens = (Token*)realloc(tokens, sizeof(Token) * (*token_count + 1));
    if (tokens == NULL) {
        perror("could not allocate memory for tokens");
        free(lexer.input);
        return NULL;
    }

    free(lexer.input); 
    return tokens;
}

Token next_token(Lexer* lexer) {
    Token token;
    char current_ch = lexer->ch;

    if (lexer->position >= strlen(lexer->input)) {
        token.type = EOF_TYPE;
        token.content = NULL;
        return token;
    }
    
    switch (lexer->ch) {
        case ' ':
            _forward(lexer);
            return next_token(lexer);
        case '<':
            token.type = L_ANGLE;
            break;
        case '>':
            token.type = R_ANGLE;
            break; 
        case '{':
            token.type = L_BRACKET;
            break;
        case '}':
            token.type = R_BRACKET;
            break;
        case '[':
            token.type = L_SQ_BRACKET;
            break;  
        case ']':
            token.type = R_SQ_BRACKET;
            break;  
        case '(':
            token.type = R_PARENT;
            break;
        case ')':
            token.type = L_PARENT;
            break;
        case '!':
            token.type = BANG;
            break;
        case '\'':
            token.type = S_QUOTE;
            break;
        case '\"':
            token.type = D_QUOTE;
            break;
        case ':':
            token.type = COLON;
            break;
        case ';':
            token.type = SEMI_COLON;
            break;
        case ',':
            token.type = COMMA;
            break;
        case '=':
            token.type = EQUALS;
            break;
        case '%':
            token.type = MOD;
            break;
        case '+': token.type = ADD;
            break;
        case '*':
            token.type = ASTERISK;
            break;
        case '-':
            token.type = SUBTRACT;
            break;
        case '/':
            token.type = F_SLASH;
            break;
        case '#':
            token.type = HASH;
            break;
        case '.':
            token.type = DOT;
            break;
        case '@':
            token.type = AT;
            break;
        case '&':
            token.type = AND;
            break;
        case '$':
            token.type = DOLLAR;
            break;
        default:
            if (isalnum(current_ch)) {
                token.type = LITERAL;
            }
            else {
                _forward(lexer);
                return next_token(lexer);
            }
    }
    
    if (token.type == LITERAL) {
        token.content = _capture_token(lexer);
    }
    else {
        size_t content_length = 1; 
        token.content = (char*)malloc(content_length + 1); 
        if (token.content == NULL) {
            perror("could not allocate memory");
            token.type = EOF_TYPE;
            return token;
        }
        token.content[0] = current_ch; 
        token.content[1] = '\0'; 
        _forward(lexer);
    }

    lexer->last_type = token.type;
    return token;
}

static char* _capture_token(Lexer* lexer) {
    size_t len = 0;
    size_t start_position = lexer->position;

    while (lexer->position < lexer->length && isalnum(lexer->input[lexer->position])) {
        ++len;
        _forward(lexer);
    }

    char* str = (char*)malloc(len + 1);

    if (str == NULL) {
        perror("could not allocate memory");
        exit(EXIT_FAILURE);
    }

    memcpy(str, lexer->input + start_position, len);
    str[len] = '\0';

    lexer->position = start_position + len;
    return str;
}

static void _forward(Lexer* lexer) {
    if (lexer->ch != '\0' && lexer->position < lexer->length) {
        lexer->ch = lexer->input[++lexer->position];
    }
}

void free_tokens(Token* tokens, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        free(tokens[i].content);
    }
}


