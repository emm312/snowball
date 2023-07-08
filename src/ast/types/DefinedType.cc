
#include "DefinedType.h"

#include "../../common.h"
#include "../../constants.h"
#include "../../ir/values/Func.h"
#include "../../utils/utils.h"
#include "../syntax/common.h"
#include "Type.h"

#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace snowball {
namespace types {

DefinedType::DefinedType(const std::string& name,
                         const std::string uuid,
                         std::shared_ptr<ir::Module>
                                 module,
                         Syntax::Statement::DefinedTypeDef* ast,
                         std::vector<ClassField*>
                                 fields,
                         std::shared_ptr<DefinedType>
                                 parent,
                         std::vector<std::shared_ptr<Type>>
                                 generics,
                         bool isStruct)
    : AcceptorExtend(Kind::CLASS, name)
    , uuid(uuid)
    , parent(parent)
    , module(module)
    , ast(ast)
    , fields(fields)
    , _struct(isStruct)
    , generics(generics) {
    setPrivacy(PUBLIC);
}
DefinedType::ClassField::ClassField(const std::string& name,
                                    std::shared_ptr<Type>
                                            type,
                                    Privacy privacy,
                                    Syntax::Expression::Base* initializedValue,
                                    bool isMutable)
    : name(name)
    , type(type)
    , Syntax::Statement::Privacy(privacy)
    , initializedValue(initializedValue)
    , isMutable(isMutable) { }
std::string DefinedType::getUUID() const { return uuid; }
Syntax::Statement::DefinedTypeDef* DefinedType::getAST() const { return ast; }
void DefinedType::addField(ClassField* f) { fields.emplace_back(f); }
std::shared_ptr<ir::Module> DefinedType::getModule() const { return module; }
int DefinedType::getVtableSize() { return classVtable.size(); }
int DefinedType::addVtableItem(std::shared_ptr<ir::Func> f) {
    classVtable.push_back(f);
    return getVtableSize() - 1;
}
std::vector<std::shared_ptr<ir::Func>> DefinedType::getVTable() const { return classVtable; }

bool DefinedType::is(DefinedType* other) const {
    auto otherArgs = other->getGenerics();
    bool genericSizeEqual = otherArgs.size() == generics.size();
    bool argumentsEqual = genericSizeEqual
            ? std::all_of(otherArgs.begin(),
                          otherArgs.end(),
                          [&, idx = 0](std::shared_ptr<Type> i) mutable {
                              return generics.at(idx)->is(i);
                              idx++;
                          })
            : false;
    return (other->getUUID() == uuid) && argumentsEqual;
}

std::string DefinedType::getPrettyName() const {
    auto base = module->isMain() ? "" : module->getName() + "::";
    auto n = base + getName();

    std::string genericString; // Start args tag
    if (generics.size() > 0) {
        genericString = "<";

        for (auto g : generics) { genericString += g->getPrettyName(); }

        genericString += ">";
    }

    return n + genericString;
}

std::string DefinedType::getMangledName() const {
    auto base = module->getUniqueName();
    auto _tyID = static_cast<ir::id_t>(getId());
    std::stringstream sstm;
    sstm << (utils::startsWith(base, _SN_MANGLE_PREFIX) ? base : _SN_MANGLE_PREFIX) << "&"
         << name.size() << name << "Cv" << _tyID;
    auto prefix = sstm.str(); // disambiguator

    std::string mangledArgs; // Start args tag
    if (generics.size() > 0) {
        mangledArgs = "ClsGSt";

        int argCounter = 1;
        for (auto g : generics) {
            mangledArgs += "A" + std::to_string(argCounter) + g->getMangledName();
            argCounter++;
        }
    }

    std::string mangled = prefix + mangledArgs + "ClsE"; // ClsE = end of class
    return mangled;
}

Syntax::Expression::TypeRef* DefinedType::toRef() {
    auto tRef = Syntax::TR(getUUID(), nullptr, shared_from_this());
    std::vector<Syntax::Expression::TypeRef*> genericRef;
    for (auto g : generics) { genericRef.push_back(g->toRef()); }

    tRef->setGenerics(genericRef);
    return tRef;
}

bool DefinedType::canCast(Type* ty) const {
    SNOWBALL_MUTABLE_CAST_CHECK
    if (auto x = utils::cast<DefinedType>(ty)) return canCast(x);
    return false;
}

bool DefinedType::canCast(DefinedType* ty) const {
    if (getParent() && (ty->is(getParent().get()))) {
        auto otherArgs = ty->getGenerics();
        bool argumentsEqual = std::all_of(
                otherArgs.begin(), otherArgs.end(), [&, idx = 0](std::shared_ptr<Type> i) mutable {
                    return generics.at(idx)->is(i);
                    idx++;
                });
        return argumentsEqual;
    }

    return false;
}

}; // namespace types
}; // namespace snowball
