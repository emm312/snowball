#include "../../../../ir/values/Cast.h"
#include "../../../Transformer.h"

using namespace snowball::utils;
using namespace snowball::Syntax::transform;

namespace snowball {
namespace Syntax {

SN_TRANSFORMER_VISIT(Statement::VariableDecl) {
  auto definedType = p_node->getDefinedType() == nullptr ? nullptr : transformType(p_node->getDefinedType())->copy();
  auto variableName = p_node->getName();
  auto variableValue = p_node->getValue();
  auto isMutable = p_node->isMutable();
  assert(p_node->isInitialized() || definedType != nullptr);
  if (definedType) {
    // definedType = utils::copy_shared(definedType);
    definedType->setMutable(isMutable);
  }

  if (ctx->getInScope(variableName, ctx->currentScope()).second) {
    E<VARIABLE_ERROR>(p_node,
                      FMT("Variable with name '%s' is already "
                          "defined in the current scope!",
                          variableName.c_str()));
  }

  auto var = builder.createVariable(p_node->getDBGInfo(), variableName, false, isMutable);
  auto item = std::make_shared<transform::Item>(transform::Item::Type::VALUE, var);
  // TODO: it should always be declared
  if (p_node->isInitialized()) {
    auto val = trans(variableValue);
    auto varDecl = builder.createVariableDeclaration(p_node->getDBGInfo(), variableName, val, isMutable);
    varDecl->setId(var->getId());
    builder.setType(varDecl, val->getType());
    if (auto f = ctx->getCurrentFunction().get()) {
      f->addSymbol(varDecl);
    } else {
      ctx->module->addVariable(varDecl);
    }

    if (definedType == nullptr || definedType->is(val->getType())) {
      this->value = varDecl;
    } else {
      if (auto v = tryCast(val, definedType); v != nullptr) {
        this->value = v;
      } else {
        E<VARIABLE_ERROR>(p_node,
                          FMT("Can't assign value with type '%s' to "
                              "the variable with type '%s'!",
                              val->getType()->getPrettyName().c_str(),
                              definedType->getPrettyName().c_str()));
      }
    }

    auto ty = val->getType()->copy();
    ty->setMutable(isMutable);
    builder.setType(var, ty);
  } else {
    auto varDecl = builder.createVariableDeclaration(p_node->getDBGInfo(), variableName, nullptr, isMutable);
    varDecl->setId(var->getId());
    builder.setType(varDecl, definedType);
    if (auto f = ctx->getCurrentFunction().get()) {
      f->addSymbol(varDecl);
    } else {
      ctx->module->addVariable(varDecl);
    }

    builder.setType(var, definedType);
    this->value = var;
  }

  ctx->addItem(variableName, item);
}

} // namespace Syntax
} // namespace snowball