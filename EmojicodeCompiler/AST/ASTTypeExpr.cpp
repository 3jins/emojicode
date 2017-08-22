//
//  ASTTypeExpr.cpp
//  Emojicode
//
//  Created by Theo Weidmann on 04/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "ASTTypeExpr.hpp"
#include "../Analysis/SemanticAnalyser.hpp"
#include "../Generation/FnCodeGenerator.hpp"

namespace EmojicodeCompiler {

Type ASTInferType::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    if (expectation.type() == TypeContent::StorageExpectation || expectation.type() == TypeContent::Nothingness) {
        throw CompilerError(position(), "Cannot infer ⚫️.");
    }
    auto type = expectation;
    type.setOptional(false);
    type_ = type;
    availability_ = expectation.type() == TypeContent::Class ? TypeAvailability::StaticAndAvailabale
    : TypeAvailability::StaticAndAvailabale;
    return type;
}

Type ASTTypeFromExpr::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    auto type = expr_->analyse(analyser, expectation);
    if (!type.meta()) {
        throw CompilerError(position(), "Expected meta type.");
    }
    if (type.optional()) {
        throw CompilerError(position(), "🍬 can’t be used as meta type.");
    }
    type.setMeta(false);
    return type;
}

void ASTTypeFromExpr::generateExpr(FnCodeGenerator *fncg) const {
    expr_->generate(fncg);
}

Type ASTStaticType::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    return type_;
}

void ASTStaticType::generateExpr(FnCodeGenerator *fncg) const {
    if (type_.type() == TypeContent::Class) {
        fncg->wr().writeInstruction(INS_GET_CLASS_FROM_INDEX);
        fncg->wr().writeInstruction(type_.eclass()->index);
    }
    else {
        assert(availability() == TypeAvailability::StaticAndUnavailable);
    }
}

Type ASTThisType::analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) {
    return analyser->typeContext().calleeType();
}

void ASTThisType::generateExpr(FnCodeGenerator *fncg) const {
    fncg->wr().writeInstruction(INS_THIS);
}

}  // namespace EmojicodeCompiler
