
#include "../../ValueVisitor/Visitor.h"
#include "../../ast/syntax/nodes.h"
#include "../../ast/types/Type.h"
#include "../../common.h"
#include "Value.h"
#include "VariableDeclaration.h"

#include <assert.h>
#include <map>
#include <vector>

#ifndef __SNOWBALL_ARGUMENT_VALUE_H_
#define __SNOWBALL_ARGUMENT_VALUE_H_

namespace snowball {
namespace ir {

/// @brief This is just an utility class that we use in order to access
///  an argument.
class Argument : public IdMixin, public AcceptorExtend<Argument, Value> {
  /// @brief Argument index respective to the parent function arg list
  int index = 0;
  /// @brief Argument name used to identify where it's pointing to
  std::string name = "";
  /// @brief default value used for the function
  Syntax::Expression::Base* defaultValue = nullptr;
  /// @brief If the variable is mutable or not
  bool mutability = false;

public:
  auto operator=(Argument*&) = delete;
  explicit Argument(const std::string& name, int index = 0, Syntax::Expression::Base* defaultValue = nullptr)
      : name(name), index(index), defaultValue(defaultValue){};

  /// @return Argument index on the list
  auto getIndex() { return index; }
  /// @return Argument index on the list
  auto getName() { return name; }
  /// @brief check if the function contains a default value
  bool hasDefaultValue() { return defaultValue != nullptr; }
  /// @return default value if it exists
  auto getDefaultValue() {
    assert(hasDefaultValue());
    return defaultValue;
  }
  /// @brief set mutability to the argument
  void setMutability(bool m = true) { mutability = m; }
  /// @return the argument's mutability
  bool isMutable() { return mutability; }

  // Set a visit handler for the generators
  SN_GENERATOR_VISITS
};

} // namespace ir
} // namespace snowball

#endif // __SNOWBALL_ARGUMENT_VALUE_H_
