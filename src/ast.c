#include <ctype.h>
#include <stdio.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ast.h"
#include "lexer.h"
#include "utils.h"

const char* element_keywords[115] = {
    "a", "abbr", "address", "area", "article", "aside", "audio",
    "b", "base", "bdi", "bdo", "blockquote", "body", "br", "button",
    "canvas", "caption", "cite", "code", "col", "colgroup",
    "data", "datalist", "dd", "del", "details", "dfn", "dialog", "div", "dl", "dt",
    "em", "embed",
    "fieldset", "figcaption", "figure", "footer", "form",
    "h1", "h2", "h3", "h4", "h5", "h6", "head", "header", "hr", "html",
    "i", "iframe", "img", "input", "ins",
    "kbd", "keygen",
    "label", "legend", "li", "link",
    "main", "map", "mark", "menu", "menuitem", "meta", "meter",
    "nav", "noscript",
    "object", "ol", "optgroup", "option", "output",
    "p", "param", "pre", "progress",
    "q",
    "rp", "rt", "ruby",
    "s", "samp", "script", "section", "select", "small", "source", "span", "strong", "style", "sub", "summary", "sup",
    "table", "tbody", "td", "textarea", "tfoot", "th", "thead", "time", "title", "tr", "track",
    "u", "ul",
    "var", "video",
    "wbr"
};

const char* attribute_keywords[160] = {
    "accept", "accept-charset", "accesskey", "action", "align", "alt", "async",
    "autocomplete", "autofocus", "autoplay",
    "bgcolor", "border",
    "challenge", "charset", "checked", "cite", "class", "code", "codebase", "color", "cols", "colspan", "content", "contenteditable", "contextmenu", "controls", "coords",
    "data", "data-*", "datetime", "default", "defer", "dir", "dirname", "disabled", "download", "draggable",
    "enctype",
    "for", "form", "formaction", "headers", "height", "hidden", "high", "href", "hreflang", "http-equiv",
    "icon", "id", "ismap",
    "keytype", "kind",
    "label", "lang", "list", "loop", "low",
    "manifest", "max", "maxlength", "media", "method", "min", "multiple", "muted",
    "name", "novalidate",
    "open", "optimum",
    "pattern", "ping", "placeholder", "poster", "preload",
    "radiogroup", "readonly", "rel", "required", "reversed", "rows", "rowspan",
    "sandbox", "scope", "scoped", "seamless", "selected", "shape", "size", "sizes", "span", "spellcheck", "src", "srcdoc", "srclang", "srcset", "start", "step", "style", "subject", "summary",
    "tabindex", "target", "title", "type",
    "usemap",
    "value"
};

static void _ignore_comments(Parser* parser);
static void _forward(Parser* parser);
static void _free_element(Element element);
static Element _init_element(TagType type, size_t attributes_count, char* content);
static SyntaxTreeNode* _init_node(SyntaxTreeNode* parent, TagType type, size_t attributes_count, char* content);
static void _push_node(SyntaxTreeNode* parent, SyntaxTreeNode* node);
void free_tree(SyntaxTreeNode* root);

SyntaxTree parse(char* path) {
    SyntaxTree st;

    char* input = xfread_full(path);
    size_t token_count = 0;

    /* Note to Myself:
     * DONT FREE tokens!
     */
    Token* tokens = lex(input, get_file_size(path), &token_count);
    free(input);

    /* print token content test
    for (int i = 0; i < token_count; ++i) {
        printf("%s\n", tokens[i].content);
    }

    init parser, which will get tokens
     provided by the lexer.
    */

    Parser* parser = (Parser*)malloc(sizeof(Parser));
    if (parser == NULL) {
        fprintf(stderr, "could not allocate memory\n");
        exit (1);
    }

    parser->tokens = tokens;
    parser->token = parser->tokens[0];
    parser->length = token_count;
    parser->position = 0;
    parser->in_tag = 0;
    
    /* allocate root node */
    st.root = (SyntaxTreeNode*)malloc(sizeof(SyntaxTreeNode));
    if (st.root == NULL) {
        fprintf(stderr, "Error allocating root node\n");
        free_tokens(parser->tokens, token_count);
        free(parser->tokens);
        return st;
    }


    /*              - child
     *      - child - child
     * root - child - child
     *      - child - child
     *              - child
     */
    
    /* init root node */
    st.root->parent = NULL;
    st.root->element = _init_element(UNKNOWN_TAG, 0, NULL);
    st.root->children = NULL;
    st.root->children_count = 0;

    /* parse */
    /* note: use stack data structure for parsing */
    
    Token element_stack[token_count];
    int stack_ptr = -1;
    
    while (parser->token.type != EOF_TYPE) {
        _ignore_comments(parser);
        _forward(parser);
    }
      
    free_tokens(parser->tokens, token_count);
    free(parser->tokens);
    return st;
}

static void _ignore_comments(Parser* parser) {
    // ignore HTML comments
    if (parser->token.type == L_ANGLE && parser->tokens[parser->position + 1].type == BANG) {
        while (!(parser->token.type == R_ANGLE && parser->tokens[parser->position - 1].type == SUBTRACT &&
                parser->tokens[parser->position - 2].type == SUBTRACT))
            _forward(parser);
        // move past the closing -->
        _forward(parser);
        _forward(parser);
    }
    
    // ignore JavaScript single-line comments
    if (parser->token.type == F_SLASH && parser->tokens[parser->position + 1].type == F_SLASH) {
        while (parser->token.type != NEW_LINE && parser->token.type != EOF_TYPE)
            _forward(parser);
    }

    // ignore JavaScript and CSS multi-line comments
    if (parser->token.type == F_SLASH && parser->tokens[parser->position + 1].type == ASTERISK) {
        while (!(parser->token.type == ASTERISK && parser->tokens[parser->position + 1].type == F_SLASH))
            _forward(parser);
        // move past the closing */
        _forward(parser);
        _forward(parser);
    }
}

static void _forward(Parser* parser) {
    if (parser->position > 0) {
        parser->token = parser->tokens[++parser->position];
    }
}

static void _free_element(Element element) {
    if (element.attributes != NULL) {
        for (size_t i = 0; i < element.attributes_count; ++i) {
            free(element.attributes[i]->content);
        }
        free(element.attributes);
    }

    free(element.content);
}

static Element _init_element(TagType type, size_t attributes_count, char* content) {
    Element element;
    element.type = type;

    element.attributes = (Attribute**)malloc(attributes_count * sizeof(Attribute*));
    if (element.attributes == NULL) {
        fprintf(stderr, "could not allocate memory\n");
        exit(EXIT_FAILURE);
    }

    element.attributes_count = attributes_count;

    // content should be allocated already
    element.content = content;  

    return element;
}

static SyntaxTreeNode* _init_node(SyntaxTreeNode* parent, TagType type, size_t attributes_count, char* content) {
    SyntaxTreeNode* node = (SyntaxTreeNode*)malloc(sizeof(SyntaxTreeNode));
    if (node == NULL) {
        fprintf(stderr, "could not allocate memory\n");
        exit(EXIT_FAILURE);
    }

    node->parent = parent;
    node->element = _init_element(type, attributes_count, content);

    // initialize children fields (if necessary)
    node->children = NULL;
    node->children_count = 0;

    return node;
}

static void _push_node(SyntaxTreeNode* parent, SyntaxTreeNode* node) {
    if (parent == NULL) {
        fprintf(stderr, "parent is NULL\n");
        return;
    }

    // allocate for childless parents
    if (parent->children == NULL) {
        parent->children = (SyntaxTreeNode**)malloc(sizeof(SyntaxTreeNode*));
        if (parent->children == NULL) {
            fprintf(stderr, "could not allocate memory\n");
            return;
        }
        parent->children[0] = node;
        parent->children_count = 1;
    } else {
        // allocate for new child
        parent->children = (SyntaxTreeNode**)realloc(parent->children, (parent->children_count + 1) * sizeof(SyntaxTreeNode*));
        if (parent->children == NULL) {
            fprintf(stderr, "could not allocate memory\n");
            return;
        }
        parent->children[parent->children_count] = node;
        ++parent->children_count;
    }

    // set parent
    node->parent = parent;
}

void free_tree(SyntaxTreeNode* root) {
    if (root == NULL) {
        return;
    }

    for (size_t i = 0; i < root->children_count; ++i) {
        free_tree(root->children[i]);
    }

    _free_element(root->element);
    free(root->children);
    free(root);
}