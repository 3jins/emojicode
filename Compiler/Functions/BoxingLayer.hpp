//
//  BoxingLayer.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 19/02/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef BoxingLayer_hpp
#define BoxingLayer_hpp

#include <utility>

#include "EmojicodeCompiler.hpp"
#include "Function.hpp"
#include "FunctionType.hpp"
#include "Types/Type.hpp"

namespace EmojicodeCompiler {

class BoxingLayer : public Function {
public:
    /// Creates a boxing layer for a protocol function.
    /// @parameter destinationFunction That function that should be called by the boxing layer. The "actual" method.
    BoxingLayer(Function *destinationFunction, const std::u32string &protocolName,
                const std::vector<Parameter> &arguments, const Type &returnType, const SourcePosition &p)
    : Function(destinationFunction->protocolBoxingLayerName(protocolName), AccessLevel::Private, true,
               destinationFunction->owningType(), destinationFunction->package(), p, false, std::u32string(), false,
               false, true, FunctionType::BoxingLayer), destinationReturnType_(destinationFunction->returnType()),
        destinationFunction_(destinationFunction) {
        this->setParameters(arguments);
        this->setReturnType(returnType);

        destinationArgumentTypes_.reserve(destinationFunction->parameters().size());
        for (auto &arg : destinationFunction->parameters()) {
            destinationArgumentTypes_.emplace_back(arg.type);
        }
    }
    /// Creates a boxing layer for a callable. The argument/return conversions will be performed and
    /// INS_EXECUTE_CALLABLE callable will be applied to the this context.
    BoxingLayer(Type thisCallable, Package *pkg, const std::vector<Parameter> &arguments, const Type &returnType,
                const SourcePosition &p)
    : Function(std::u32string(), AccessLevel::Private, true, std::move(thisCallable), pkg, p, false,
               std::u32string(), false, false, true, FunctionType::BoxingLayer),
      destinationArgumentTypes_(owningType().genericArguments().begin() + 1, owningType().genericArguments().end()),
      destinationReturnType_(owningType().genericArguments()[0]) {
        this->setParameters(arguments);
        this->setReturnType(returnType);
    }

    const std::vector<Type>& destinationArgumentTypes() const { return destinationArgumentTypes_; }
    const Type& destinationReturnType() const { return destinationReturnType_; }
    /// Returns the function whill will be called. Returns @c nullptr if this is a callable boxing layer.
    Function* destinationFunction() const { return destinationFunction_; }
private:
    std::vector<Type> destinationArgumentTypes_;
    Type destinationReturnType_;
    Function *destinationFunction_ = nullptr;
};

}  // namespace EmojicodeCompiler

#endif /* BoxingLayer_hpp */
