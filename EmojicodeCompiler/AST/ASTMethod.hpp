//
//  ASTMethod.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 05/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTMethod_hpp
#define ASTMethod_hpp

#include <utility>

#include "ASTExpr.hpp"

namespace EmojicodeCompiler {

class SemanticAnalyser;

class ASTMethodable : public ASTExpr {
protected:
    explicit ASTMethodable(const SourcePosition &p) : ASTExpr(p), args_(p) {}
    ASTMethodable(const SourcePosition &p, ASTArguments args) : ASTExpr(p), args_(std::move(args)) {}
    Type analyseMethodCall(SemanticAnalyser *analyser, const EmojicodeString &name,
                           std::shared_ptr<ASTExpr> &callee);
    EmojicodeInstruction instruction_;
    ASTArguments args_;
    bool invert_ = false;
};

class ASTMethod final : public ASTMethodable {
public:
    ASTMethod(EmojicodeString name, std::shared_ptr<ASTExpr> callee,
              const ASTArguments &args, const SourcePosition &p)
    : ASTMethodable(p, args), name_(std::move(name)), callee_(std::move(callee)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
private:
    void generateExpr(FnCodeGenerator *fncg) const override;
    EmojicodeString name_;
    std::shared_ptr<ASTExpr> callee_;
};

}  // namespace EmojicodeCompiler

#endif /* ASTMethod_hpp */
