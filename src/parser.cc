
#include "snowball/nodes.h"
#include "snowball/colors.h"
#include "snowball/errors.h"
#include "snowball/parser.h"
#include "snowball/logger.h"

#include "snowball/snowball.h"

#include <string>
#include <memory>
#include <utility>

#include <assert.h>

#define ASSERT(x) assert(x);

#define PARSER_ERROR(err, msg) _parser_error(err, msg);
#define UNEXPECTED_TOK(expectation) PARSER_ERROR(Error::SYNTAX_ERROR, Logger::format("Expected %s, got %s%s%s", expectation, RED, _current_token.to_string().c_str(), RESET));
#define ASSERT_TOKEN(tk, ty, idnt) if (tk.type != ty) UNEXPECTED_TOK(idnt);
#define ASSERT_TOKEN_EOF(tk, ty, idnt, method) \
    if (tk.type == TokenType::_EOF) PARSER_ERROR(Error::UNEXPECTED_EOF, Logger::format("Found an unexpected EOF while parsing %s", method)); \
    ASSERT_TOKEN(tk, ty, idnt)

namespace snowball {

    void Parser::parse() {
        while (true) {
            switch (_current_token.type)
            {
                case TokenType::_EOF:
                    return;

                case TokenType::KWORD_FUNC: {
                    FunctionNode* function = _parse_function();
                    _nodes.push_back(function);
                    break;
                }

                case TokenType::KWORD_VAR: {
                    int _width = _current_token.col;
                    std::pair<int, int> _pos = std::make_pair(_current_token.line, _current_token.col);

                    VarNode* var = _parse_variable();
                    _width = _width - _current_token.col;

                    var->pos = _pos;
                    var->width = (uint32_t)_width;

                    _nodes.push_back(var);
                    break;
                }

                case TokenType::SYM_SEMI_COLLON:
                    break;

                case TokenType::VALUE_BOOL:
                case TokenType::VALUE_NULL:
                case TokenType::VALUE_FLOAT:
                case TokenType::VALUE_NUMBER:
                case TokenType::VALUE_STRING:
                case TokenType::VALUE_UNDEFINED: {

                    int _width = _current_token.col;
                    std::pair<int, int> _pos = std::make_pair(_current_token.line, _current_token.col);

                    ConstantValue* value = static_cast<ConstantValue *>(_parse_expression()); // Type: ConstantValue
                    _width = _width - _current_token.col;

                    value->pos = _pos;
                    value->width = (uint32_t)_width;

                    _nodes.push_back(value);
                    break;
                }

                default:
                    PARSER_ERROR(Error::SYNTAX_ERROR, Logger::format("Unexpected token found: %s%s%s", BLU, _current_token.to_string().c_str(), RESET))
                    break;
            }

            next_token();
        }
    }

    // Parse methods

    VarNode* Parser::_parse_variable() {
        ASSERT(_current_token.type == TokenType::KWORD_VAR)
        next_token();

        VarNode* var = new VarNode();
        ASSERT_TOKEN_EOF(_current_token, TokenType::IDENTIFIER, "an identifier", "variable")

        var->name = _current_token.to_string();
        next_token();

        ASSERT_TOKEN_EOF(_current_token, TokenType::OP_EQ, "=", "variable")
        next_token();

        if (_current_token.type == TokenType::_EOF) {
            PARSER_ERROR(Error::UNEXPECTED_EOF, "Found an unexpected EOF while parsing variable's statement");
        }

        Node* value = _parse_expression();
        var->value = value;

        if (_current_token.type == TokenType::SYM_SEMI_COLLON) {
            next_token();
        }

        return var;
    }

    FunctionNode* Parser::_parse_function() {
        // PARSER_ERROR(Error::TODO, "Functions are not yet supported.")

        // Assert if the token is the "func" keyword
        // and consume it.
        ASSERT(_current_token.type == TokenType::KWORD_FUNC)

        FunctionNode* func = new FunctionNode();
        next_token();

        // Find function's name
        ASSERT_TOKEN(_current_token, TokenType::IDENTIFIER, "an identifier")
        func->name = _current_token.to_string();

        next_token();

        std::vector<ArgumentNode*> arguments;

        // TODO: check for args
        if (_current_token.type == TokenType::BRACKET_LPARENT) {
            next_token();
            while (true) {
                if (_current_token.type == TokenType::IDENTIFIER) {
                    // Argument structure: (name: type, name2: type2 = default, ...argv)
                    // TODO: argv, default

                    std::string name = _current_token.to_string();
                    std::string type_name;

                    next_token(); // consume name
                    ASSERT_TOKEN_EOF(_current_token, TokenType::SYM_COLLON, ":", "argument statement")
                    next_token(); // consume :

                    ASSERT_TOKEN_EOF(_current_token, TokenType::IDENTIFIER, "<type>", "argument type declaration")
                    type_name = _current_token.to_string();
                    next_token(); // consume :

                    ArgumentNode* argument = new ArgumentNode(name, type_name);
                    arguments.push_back(argument);

                    // cleanup
                    if (_current_token.type == TokenType::SYM_COMMA) {
                        next_token();
                        if (_current_token.type == TokenType::BRACKET_RPARENT) {
                            next_token();
                            break;
                        } else if (_current_token.type == TokenType::IDENTIFIER) {
                            continue;
                        }

                        UNEXPECTED_TOK("An indentifier or a ')'")

                    } else if (_current_token.type == TokenType::BRACKET_RPARENT) {
                        next_token();
                        break;
                    }
                } else if (_current_token.type == TokenType::BRACKET_RPARENT) {
                    next_token();
                    break;
                } else {
                    UNEXPECTED_TOK("An identifier or a ')'")
                }
            }
        }

        // TODO: add arguments to the function
        BlockNode* body = _parse_block();
        func->body = body;
        return func;
    }

    BlockNode* Parser::_parse_block(std::vector<TokenType> p_termination) {
        std::vector<Node *> stmts;
        BlockNode* b_node = new BlockNode();

        while (true) {

            next_token();
            Token tk = _current_token;

            switch (tk.type)
            {
                case TokenType::_EOF: {
                    PARSER_ERROR(Error::UNEXPECTED_EOF, "Found an unexpected EOF while parsing a block");
                    break;
                }

                case TokenType::KWORD_VAR: {
                    int _width = _current_token.col;
                    std::pair<int, int> _pos = std::make_pair(_current_token.line, _current_token.col);

                    VarNode* var = _parse_variable();
                    _width = _width - _current_token.col;

                    var->pos = _pos;
                    var->width = (uint32_t)_width;


                    stmts.push_back(var);
                    break;
                }

                default: {
                    for (TokenType termination : p_termination) {
                        if (tk.type == termination) {
                            b_node->exprs = stmts;
                            return b_node;
                        }
                    }

                    if (tk.type >= TokenType::KWORD__START__POINT && tk.type <= TokenType::KWORD__ENDING__POINT)
                        PARSER_ERROR(Error::SYNTAX_ERROR, Logger::format("%s%s%s not allowed inside blocks", BLU, _current_token.to_string().c_str(), RESET))

                    UNEXPECTED_TOK("a valid expression")
                }
            }
        }

        b_node->exprs = stmts;
        return b_node;
    }

    // Private
    void Parser::next_token(int p_offset) {
        try {
            __token_possition++;
            _current_token = _tokens.at(__token_possition);
        } catch (std::out_of_range& _) {
            PARSER_ERROR(Error::BUG, "Index error")
        }
    }

    Token Parser::peek(int p_offset, bool p_safe) {
        Token tmp = { TokenType::_EOF };
        if ((__token_possition + 1) + p_offset < 0 || (__token_possition + 1) + p_offset >= (int)_tokens.size()) {
            if (p_safe) return tmp;
            else PARSER_ERROR(Error::BUG, "Parser::peek() index out of bounds");
        }
        return _tokens.at((__token_possition + 1) + p_offset);
    }

    void Parser::_parser_error(Error type, std::string msg) {
        DBGSourceInfo* dbg_info = new DBGSourceInfo((SourceInfo*)_source_info, std::pair<int, int>(_current_token.line, _current_token.col), _current_token.to_string().size());
        throw ParserError(type, msg, dbg_info);
    }

    Node* Parser::_parse_expression() {
        std::vector<Node *> expressions;

        while (true) {
            switch (_current_token.type)
            {
                case TokenType::VALUE_BOOL:
                case TokenType::VALUE_NULL:
                case TokenType::VALUE_FLOAT:
                case TokenType::VALUE_NUMBER:
                case TokenType::VALUE_STRING:
                case TokenType::VALUE_UNDEFINED: {
                    expressions.push_back(new ConstantValue(_current_token.type, _current_token.to_string()));
                    break;
                }

                case TokenType::OP_NOT:
                case TokenType::OP_PLUS:
                case TokenType::OP_MINUS:
                case TokenType::OP_BIT_NOT:

                    if (_current_token.type == TokenType::OP_NOT)     expressions.push_back(new BinaryOp(OpType::OP_NOT))      ;
                    if (_current_token.type == TokenType::OP_PLUS)    expressions.push_back(new BinaryOp(OpType::OP_POSITIVE)) ;
                    if (_current_token.type == TokenType::OP_MINUS)   expressions.push_back(new BinaryOp(OpType::OP_NEGATIVE)) ;
                    if (_current_token.type == TokenType::OP_BIT_NOT) expressions.push_back(new BinaryOp(OpType::OP_BIT_NOT))  ;

                    break;

                default:
                    UNEXPECTED_TOK("a valid expression")
            }

            Token tk = peek(0, true);

            OpType op;
            bool valid = true;

            switch (tk.type) {
            #define OP_CASE(m_op) case TokenType::m_op: op = OpType::m_op; break;
                OP_CASE(OP_EQ);
                OP_CASE(OP_EQEQ);
                OP_CASE(OP_PLUS);
                OP_CASE(OP_PLUSEQ);
                OP_CASE(OP_MINUS);
                OP_CASE(OP_MINUSEQ);
                OP_CASE(OP_MUL);
                OP_CASE(OP_MULEQ);
                OP_CASE(OP_DIV);
                OP_CASE(OP_DIVEQ);
                OP_CASE(OP_MOD);
                OP_CASE(OP_MOD_EQ);
                OP_CASE(OP_LT);
                OP_CASE(OP_LTEQ);
                OP_CASE(OP_GT);
                OP_CASE(OP_GTEQ);
                OP_CASE(OP_AND);
                OP_CASE(OP_OR);
                OP_CASE(OP_NOT);
                OP_CASE(OP_NOTEQ);
                OP_CASE(OP_BIT_NOT);
                OP_CASE(OP_BIT_LSHIFT);
                OP_CASE(OP_BIT_LSHIFT_EQ);
                OP_CASE(OP_BIT_RSHIFT);
                OP_CASE(OP_BIT_RSHIFT_EQ);
                OP_CASE(OP_BIT_OR);
                OP_CASE(OP_BIT_OR_EQ);
                OP_CASE(OP_BIT_AND);
                OP_CASE(OP_BIT_AND_EQ);
                OP_CASE(OP_BIT_XOR);
                OP_CASE(OP_BIT_XOR_EQ);
            #undef OP_CASE

                default: valid = false;
            }

            if (valid) {
                next_token(); // Eat peeked token.
                expressions.push_back(new BinaryOp(op));

                next_token();
            } else {
                break;
            }

        }

        Node* op_tree = _build_op_tree(expressions);
        return op_tree;
    }

    Node* Parser::_build_op_tree(std::vector<Node*> &expressions) {
        ASSERT(expressions.size() > 0)

        while (expressions.size() > 1) {

            int next_op = -1;
            int min_precedence = 0xFFFFF;
            bool unary = false;

            for (int i = 0; i < (int)expressions.size(); i++) {
                BinaryOp* expression = static_cast<BinaryOp*>(expressions[i]);
                if (!(expression->type == Node::Type::OPERATOR)) {
                    continue;
                }

                int precedence = -1;
                switch (expression->op_type) {
                    case OpType::OP_NOT:
                    case OpType::OP_BIT_NOT:
                    case OpType::OP_POSITIVE:
                    case OpType::OP_NEGATIVE:
                        min_precedence = 0;
                        break;

                    case OpType::OP_MUL:
                    case OpType::OP_DIV:
                    case OpType::OP_MOD:
                        min_precedence = 1;
                        break;

                    case OpType::OP_PLUS:
                    case OpType::OP_MINUS:
                        min_precedence = 2;
                        break;

                    case OpType::OP_BIT_LSHIFT:
                    case OpType::OP_BIT_RSHIFT:
                        min_precedence = 3;
                        break;

                    case OpType::OP_LT:
                    case OpType::OP_LTEQ:
                    case OpType::OP_GT:
                    case OpType::OP_GTEQ:
                        min_precedence = 4;
                        break;

                    case OpType::OP_EQEQ:
                    case OpType::OP_NOTEQ:
                        min_precedence = 5;
                        break;

                    case OpType::OP_BIT_AND:
                        min_precedence = 6;
                        break;

                    case OpType::OP_BIT_XOR:
                        min_precedence = 7;
                        break;

                    case OpType::OP_BIT_OR:
                        min_precedence = 8;
                        break;

                    case OpType::OP_AND:
                        min_precedence = 9;
                        break;

                    case OpType::OP_OR:
                        min_precedence = 10;
                        break;

                    case OpType::OP_EQ:
                    case OpType::OP_PLUSEQ:
                    case OpType::OP_MINUSEQ:
                    case OpType::OP_MULEQ:
                    case OpType::OP_DIVEQ:
                    case OpType::OP_MOD_EQ:
                    case OpType::OP_BIT_LSHIFT_EQ:
                    case OpType::OP_BIT_RSHIFT_EQ:
                    case OpType::OP_BIT_AND_EQ:
                    case OpType::OP_BIT_XOR_EQ:
                    case OpType::OP_BIT_OR_EQ:
                        min_precedence = 11;
                        break;

                    case OpType::NONE:
                        min_precedence = -1;
                        break;
                }

                if (precedence < min_precedence) {
                    min_precedence = precedence;
                    next_op = i;
                    OpType op = expression->op_type;
                    unary = (
                        op == OpType::OP_NOT      ||
                        op == OpType::OP_BIT_NOT  ||
                        op == OpType::OP_POSITIVE ||
                        op == OpType::OP_NEGATIVE );
                    // break;
                }
            }

            ASSERT(next_op >= 0);

            if (unary) {

                int next_expr = next_op;
                while (expressions[next_expr]->type == Node::Type::OPERATOR) {
                    if (++next_expr == expressions.size()) {
                        PARSER_ERROR(Error::SYNTAX_ERROR, "expected an expression.");
                    }
                }

                for (int i = next_expr - 1; i >= next_op; i--) {
                    BinaryOp* op_node = new BinaryOp(expressions[(size_t)i]);

                    op_node->left = expressions[(size_t)i + 1];
                    expressions.erase(expressions.begin() + i + 1);

                    expressions.erase(expressions.begin() + i);
                    expressions.insert(expressions.begin(), op_node);

                }
            } else {

                // ASSERT(next_op >= 1 && next_op < (int)expressions.size() - 1)
                // ASSERT((!(expressions[(size_t)next_op - 1]->type == Node::Type::OPERATOR)) && (!(expressions[(size_t)next_op + 1].type == Node::Type::OPERATOR)));

                BinaryOp* op_node = new BinaryOp(expressions[(size_t)next_op + 1]);

                if (expressions[(size_t)next_op - 1]->type == Node::Type::OPERATOR) {
                    if (BinaryOp::is_assignment(expressions[(size_t)next_op - 1])) {
                        PARSER_ERROR(Error::SYNTAX_ERROR, "unexpected assignment.")
                    }
                }

                if (expressions[(size_t)next_op + 1]->type == Node::Type::OPERATOR) {
                    if (BinaryOp::is_assignment(expressions[(size_t)next_op - 1])) {
                        PARSER_ERROR(Error::SYNTAX_ERROR, "unexpected assignment.")
                    }
                }

                op_node->left = expressions[(size_t)next_op - 1];
                op_node->right = expressions[(size_t)next_op + 1];

                expressions.erase(expressions.begin() + (next_op));
                expressions.insert((expressions.begin() + (next_op - 1)), op_node);

                expressions.erase(expressions.begin() + next_op);
                expressions.erase(expressions.begin() + next_op);
            }
        }

        return expressions[0];
    }
}
