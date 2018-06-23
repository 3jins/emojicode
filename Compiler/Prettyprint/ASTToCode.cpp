//
//  ASTToCode.cpp
//  EmojicodeCompiler
//
//  Created by Theo Weidmann on 22/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#include "Parsing/OperatorHelper.hpp"
#include "AST/ASTBinaryOperator.hpp"
#include "AST/ASTClosure.hpp"
#include "AST/ASTControlFlow.hpp"
#include "AST/ASTExpr.hpp"
#include "AST/ASTInitialization.hpp"
#include "AST/ASTLiterals.hpp"
#include "AST/ASTMethod.hpp"
#include "AST/ASTStatements.hpp"
#include "AST/ASTTypeExpr.hpp"
#include "AST/ASTUnary.hpp"
#include "AST/ASTUnsafeBlock.hpp"
#include "AST/ASTVariables.hpp"
#include "Prettyprinter.hpp"
#include "Types/Type.hpp"
#include <sstream>

namespace EmojicodeCompiler {

void ASTArguments::toCode(Prettyprinter &pretty) const {
    if (!arguments_.empty()) {
        pretty << " ";
        for (auto &arg : arguments_) {
            arg->toCode(pretty);
        }
    }
    pretty.refuseOffer() << (imperative_ ? "❗️" : "❓️");
}

void ASTBlock::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    if (stmts_.empty()) {
        pretty << "🍇🍉\n";
        return;
    }
    pretty << "🍇";
    pretty.offerNewLine();
    innerToCode(pretty);
    pretty.indent() << "🍉\n";
}

void ASTBlock::innerToCode(Prettyprinter &pretty) const {
    pretty.increaseIndent();
    for (auto &stmt : stmts_) {
        stmt->toCode(pretty);
        pretty.offerNewLine();
        if (stmt->paragraph()) {
            pretty << "\n";
        }
    }
    pretty.decreaseIndent();
}

void ASTRepeatWhile::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty.indent() << "🔁 ";
    condition_->toCode(pretty);
    block_.toCode(pretty);
}

void ASTForIn::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty.indent() << "🔂 " << utf8(varName_) << " ";
    iteratee_->toCode(pretty);
    block_.toCode(pretty);
}

void ASTUnsafeBlock::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty.indent() << "☣️ ";
    block_.toCode(pretty);
}

void ASTIf::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty.indent() << "↪️ ";
    conditions_.front()->toCode(pretty);
    pretty << " ";
    blocks_.front().toCode(pretty);
    for (size_t i = 1; i < conditions_.size(); i++) {
        pretty.indent() << "🙅↪️ ";
        conditions_[i]->toCode(pretty);
        pretty << " ";
        blocks_[i].toCode(pretty);
    }
    if (hasElse()) {
        pretty.indent() << "🙅 ";
        blocks_.back().toCode(pretty);
    }
}

void ASTClosure::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty.printClosure(closure_.get());
}

void ASTErrorHandler::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty.indent() << "🥑 " << utf8(valueVarName_) << " ";
    value_->toCode(pretty);
    pretty << " ";
    valueBlock_.toCode(pretty);
    pretty.indent() << "🙅‍♀️ " << utf8(errorVarName_) << " ";
    errorBlock_.toCode(pretty);
}

void ASTExprStatement::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty.indent();
    expr_->toCode(pretty);
}

void ASTVariableDeclaration::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty.indent() << "🖍🆕 " << utf8(varName_) << " " << type_;
}

void ASTVariableAssignment::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty.indent();
    expr_->toCode(pretty);
    pretty << "➡️ 🖍" << utf8(name());
}

void ASTVariableDeclareAndAssign::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty.indent();
    expr_->toCode(pretty);
    pretty << "➡️ 🖍🆕 " << utf8(name());
}

void ASTConstantVariable::toCode(Prettyprinter &pretty) const {
    pretty.indent();
    expr_->toCode(pretty);
    pretty.printComments(position());
    pretty << " ➡️ " << utf8(name());
}

void ASTConditionalAssignment::toCode(Prettyprinter &pretty) const {
    expr_->toCode(pretty);
    pretty << " ➡️ " << utf8(varName_);
}

void ASTOperatorAssignment::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty.indent();
    auto binaryOperator = dynamic_cast<ASTBinaryOperator *>(expr_.get());
    pretty << utf8(name()) << " ⬅️" << utf8(operatorName(binaryOperator->operatorType())) << " ";
    binaryOperator->right()->toCode(pretty);
}

void ASTGetVariable::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << utf8(name());
}

void ASTSuper::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << "⤴️" << utf8(name_);
    args_.toCode(pretty);
}

void ASTInitialization::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << "🆕";
    typeExpr_->toCode(pretty);
    pretty << utf8(name_);
    args_.toCode(pretty);
}

void ASTThisType::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << "🐕";
}

void ASTInferType::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << "⚫️";
}

void ASTStaticType::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << type_;
}

void ASTTypeFromExpr::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << "⬛️";
    expr_->toCode(pretty);
}

void ASTTypeAsValue::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << ASTTypeValueType::toString(tokenType_) << type_;
}

void ASTSizeOf::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << "⚖️" << type_;
}

void ASTCallableCall::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << "⁉️";
    callable_->toCode(pretty);
    args_.toCode(pretty);
}

void ASTBooleanTrue::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << "👍";
}

void ASTBooleanFalse::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << "👎";
}

void ASTThis::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << "🐕";
}

void ASTIsError::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << "🚥";
    value_->toCode(pretty);
}

void ASTUnwrap::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << " 🍺";
    value_->toCode(pretty);
}

void ASTNumberLiteral::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << utf8(string_);
    pretty.offerSpace();
}

void ASTSymbolLiteral::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << "🔟" << utf8(std::u32string(1, value_));
}

void ASTNoValue::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << "🤷‍♀️";
}

void ASTStringLiteral::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << "🔤" << utf8(value_) << "🔤";
}

void ASTRaise::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty.indent() << "🚨";
    value_->toCode(pretty);
}

void ASTReturn::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    if (value_ == nullptr) {
        pretty.indent() << "↩️↩️";
    }
    else {
        pretty.indent() << "↩️ ";
        value_->toCode(pretty);
    }
}

void ASTCast::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << "🔲";
    value_->toCode(pretty);
    typeExpr_->toCode(pretty);
}

void ASTMethod::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << utf8(name_);
    callee_->toCode(pretty);
    args_.toCode(pretty);
}

void ASTConcatenateLiteral::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << "🍪 ";
    for (auto &val : values_) {
        val->toCode(pretty);
        pretty << " ";
    }
    pretty << "🍪";
}

void ASTListLiteral::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << "🍨 ";
    for (auto &val : values_) {
        val->toCode(pretty);
        pretty << " ";
    }
    pretty << "🍆";
}

void ASTDictionaryLiteral::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    pretty << "🍯 ";
    for (auto &val : values_) {
        val->toCode(pretty);
        pretty << " ";
    }
    pretty << "🍆";
}

void ASTBinaryOperator::printBinaryOperand(int precedence, const std::shared_ptr<ASTExpr> &expr,
                                           Prettyprinter &pretty) const {
    pretty.printComments(position());
    if (auto oper = dynamic_cast<ASTBinaryOperator *>(expr.get())) {
        if (operatorPrecedence(oper->operator_) < precedence) {
            pretty << "🤜";
            expr->toCode(pretty);
            pretty << "🤛";
            return;
        }
    }
    expr->toCode(pretty);
}

void ASTBinaryOperator::toCode(Prettyprinter &pretty) const {
    pretty.printComments(position());
    auto precedence = operatorPrecedence(operator_);
    printBinaryOperand(precedence, left_, pretty);
    pretty << " " << utf8(operatorName(operator_)) << " ";
    printBinaryOperand(precedence, right_, pretty);
}

void ASTType::toCode(Prettyprinter &pretty) const {
    if (optional_) {
        pretty << "🍬";
    }
    toCodeType(pretty);
}

void ASTTypeId::toCodeType(Prettyprinter &pretty) const {
    if (!namespace_.empty()) {
        pretty << "🔶" << utf8(namespace_);
    }
    pretty << utf8(name_);
    if (!genericArgs_.empty()) {
        pretty << "🐚";
        for (auto &arg : genericArgs_) {
            pretty << arg;
        }
        pretty.refuseOffer() << "🍆";
    }
}

void ASTErrorType::toCodeType(Prettyprinter &pretty) const {
    pretty << "🚨" << enum_ << content_;
}

void ASTCallableType::toCodeType(Prettyprinter &pretty) const {
    pretty << "🍇";
    for (auto &type : params_) {
        pretty << type;
    }
    if (return_ != nullptr) {
        pretty << "➡️" << return_;
    }
    pretty << "🍉";
}

void ASTMultiProtocol::toCodeType(Prettyprinter &pretty) const {
    pretty << "🍱";
    for (auto &type : protocols_) {
        pretty << type;
    }
    pretty << "🍱";
}

void ASTGenericVariable::toCodeType(Prettyprinter &pretty) const {
    pretty << utf8(name_);
    pretty.offerSpace();
}

void ASTTypeValueType::toCodeType(Prettyprinter &pretty) const {
    pretty << toString(tokenType_) << type_;
}

void ASTLiteralType::toCode(Prettyprinter &pretty) const {
    pretty << type();
}

} // namespace EmojicodeCompiler
