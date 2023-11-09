
#include "../../ir/values/Call.h"
#include "../../ir/values/Dereference.h"
#include "../../ir/values/ReferenceTo.h"
#include "../../services/OperatorService.h"
#include "../../utils/utils.h"
#include "LLVMBuilder.h"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

namespace snowball {
namespace codegen {

void LLVMBuilder::visit(ir::Call* call) {
  bool isConstructor = false;
  if (buildOperator(call)) return; // TODO: maybe jump to cleanup?
  if (utils::is<ir::ZeroInitialized>(call)) {
    this->value = llvm::Constant::getNullValue(getLLVMType(call->getType()));
    return;
  }

  auto calleeValue = call->getCallee();
  auto fnType = utils::cast<types::FunctionType>(calleeValue->getType());
  auto calleeType = getLLVMFunctionType(fnType, utils::cast<ir::Func>(calleeValue.get()));

  llvm::Value* llvmCall = nullptr;
  llvm::Value* allocatedValue = nullptr;
  llvm::Type* allocatedValueType = nullptr;
  //setDebugInfoLoc(nullptr);
  if (auto c = utils::cast<types::FunctionType>(calleeValue->getType());
      (c != nullptr && utils::cast<types::BaseType>(c->getRetType())) || ctx->callStoreValue) {
    if (ctx->callStoreValue) {
      allocatedValue = ctx->callStoreValue;
      ctx->callStoreValue = nullptr;
    } else {
      auto retType = c->getRetType();
      allocatedValueType = getLLVMType(retType);
      if (ctx->retValueUsedFromArg) {
        allocatedValue = ctx->getCurrentFunction()->getArg(0);
        ctx->retValueUsedFromArg = false;
      } else {
        // It's a function returning a type that's not a pointer
        // We need to allocate the value
        allocatedValue = createAlloca(getLLVMType(retType), ".ret-temp");
      }
    }
  }

  auto callee = build(calleeValue.get());
  auto args = utils::vector_iterate<std::shared_ptr<ir::Value>, llvm::Value*>(
    call->getArguments(), [this](std::shared_ptr<ir::Value> arg) { return expr(arg.get()); }
  );

  if (allocatedValue) 
    args.insert(args.begin(), allocatedValue);

  setDebugInfoLoc(nullptr);
  if (auto c = utils::dyn_cast<ir::Func>(calleeValue); c != nullptr && c->isConstructor()) {
    auto instance = utils::cast<ir::ObjectInitialization>(call);
    auto instanceType = getLLVMType(instance->getType());
    isConstructor = true;
    assert(instance);
    assert(c->hasParent());

    llvm::Value* object = allocatedValue;
    if (instance->createdObject) {
      object = build(instance->createdObject.get());
    } else if (!allocatedValue) {
      auto classType = utils::cast<types::DefinedType>(instance->getType());
      assert(classType && "Class type is not a defined type!");
      object = allocateObject(classType);
      ctx->doNotLoadInMemory = true;
    }

    if (!allocatedValue)
      args.insert(args.begin(), load(object, instance->getType()->getReferenceTo()));
    setDebugInfoLoc(call); // TODO:
    llvmCall = createCall(calleeType, callee, args);
    this->value = object;
  } else if (auto c = utils::dyn_cast<ir::Func>(calleeValue); c != nullptr && c->inVirtualTable()) {
    assert(c->hasParent());

    auto index = c->getVirtualIndex() + 2; // avoid class info
    auto parent = c->getParent();

    auto f = llvm::cast<llvm::Function>(callee);
    // note: allocatedValue != nullptr is because there are 2 possible cases:
    // 1. The function returns a type that's not a pointer (meaning self is at index 1)
    // 2. The function does not return a type that's not a pointer (meaning self is at index 0)
    auto parentValue = args.at(/* self = */ allocatedValue != nullptr);
    auto parentIR = call->getArguments().at(/* self = */ 0);
    auto parentType = parentIR->getType();
    auto definedType = utils::cast<types::DefinedType>(parentType);
    if (auto x = utils::cast<types::ReferenceType>(parentType))
      definedType = utils::cast<types::DefinedType>(x->getPointedType());
    else
      assert(false && "Parent type is not a reference type!");
    assert(definedType && "Parent type is not a defined type!");
    getVtableType(definedType); // generate vtable type (if not generated yet)
    auto vtableType = ctx->getVtableTy(definedType->getId());
    // vtable structure:
    // class instance = { [0] = vtable, ... }
    // vtable = { [size x ptr] } { [0] = fn1, [1] = fn2, ... } }
    auto vtable = builder->CreateLoad(vtableType->getPointerTo(), parentValue);
    auto fn = builder->CreateLoad(
            calleeType->getPointerTo(), builder->CreateConstInBoundsGEP1_32(vtableType->getPointerTo(), vtable, index)
    );
    builder->CreateAssumption(builder->CreateIsNotNull(fn));
    setDebugInfoLoc(call);
    llvmCall = createCall(calleeType, (llvm::Function*) fn, args);
    // builder->CreateStore(llvmCall, allocatedValue);
    // auto alloca = createAlloca(allocatedValueType);
    // builder->CreateStore(llvmCall, alloca);
    // builder->CreateMemCpy(alloca, llvm::MaybeAlign(), allocatedValue, llvm::MaybeAlign(),
    //         module->getDataLayout().getTypeAllocSize(allocatedValueType), 0);
    // this->value = allocatedValue;
    this->value = allocatedValue ? allocatedValue : llvmCall;
  } else {
    if (!llvm::isa<llvm::Function>(callee)) {
      // we are calling a value instead of a direct function.
      // this means we need to dereference the value first
      // and then call it.
      callee = load(callee, fnType);
    }
    setDebugInfoLoc(call);
    llvmCall = createCall(calleeType, callee, args);
    this->value = allocatedValue ? allocatedValue : llvmCall;
  }

#define SET_CALL_ATTRIBUTES(type)                                                                                      \
  if (llvm::isa<type>(llvmCall)) {                                                                                     \
    auto call = llvm::cast<type>(llvmCall);                                                                            \
    auto calledFunction = call->getCalledFunction();                                                                   \
    bool retIsReference = false;                                                                                       \
    if (auto f = utils::dyn_cast<ir::Func>(calleeValue)) {                                                             \
      auto valBackup = this->value;                                                                                    \
      retIsReference = utils::cast<types::ReferenceType>(f->getRetTy()) != nullptr;                                    \
      calledFunction = llvm::cast<llvm::Function>(build(f.get()));                                                     \
      this->value = valBackup;                                                                                         \
    }                                                                                                                  \
    if (calledFunction) {                                                                                              \
      auto attrSet = calledFunction->getAttributes();                                                                  \
      if (retIsReference) { attrSet = attrSet.addRetAttribute(*context, llvm::Attribute::NonNull); }                   \
      call->setAttributes(attrSet);                                                                                    \
    }                                                                                                                  \
  }

  SET_CALL_ATTRIBUTES(llvm::CallInst)
  else SET_CALL_ATTRIBUTES(llvm::InvokeInst) else { assert(false); }

  if (!allocatedValue && !isConstructor) ctx->doNotLoadInMemory = true;
}

} // namespace codegen
} // namespace snowball
