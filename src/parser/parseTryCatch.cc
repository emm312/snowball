
#include "../common.h"
#include "../lexer/tokens/token.h"
#include "../services/OperatorService.h"
#include "./Parser.h"

#include <assert.h>

namespace snowball::parser {

Syntax::Statement::TryCatch* Parser::parseTryCatch() {
    createError<TODO>("TryCatch is still being implemented!");
    assert(is<TokenType::KWORD_TRY>());
    auto dbg = DBGSourceInfo::fromToken(m_source_info, m_current);
    auto tryBlock = parseBlock();
    std::vector<Syntax::Statement::TryCatch::CatchBlock*> catchBlocks;
    assert_tok<TokenType::KWORD_CATCH>("a catch block");
    while (is<TokenType::KWORD_CATCH>()) {
        next();
        consume<TokenType::BRACKET_LPARENT>("a left parenthesis '(' to start the catch block");
        auto name = assert_tok<TokenType::IDENTIFIER>("an identifier for the exception variable").to_string();
        consume<TokenType::SYM_COLLON>("a collon ':' to separate the exception variable from the type");
        auto type = parseType();
        consume<TokenType::BRACKET_RPARENT>("a right parenthesis ')' to end the catch block header");
        auto block = parseBlock();
        catchBlocks.push_back(new Syntax::Statement::TryCatch::CatchBlock(type, name, block));
    }
    auto tryCatch = new Syntax::Statement::TryCatch(tryBlock, catchBlocks);
    tryCatch->setDBGInfo(dbg);
    return tryCatch;
}   

} // namespace snowball::parser
