//
//  ASTExpr.hpp
//  Emojicode
//
//  Created by Theo Weidmann on 03/08/2017.
//  Copyright © 2017 Theo Weidmann. All rights reserved.
//

#ifndef ASTExpr_hpp
#define ASTExpr_hpp

#include <utility>

#include "../Parsing/OperatorHelper.hpp"
#include "ASTNode.hpp"

namespace EmojicodeCompiler {

class ASTTypeExpr;
class SemanticAnalyser;
class TypeExpectation;
class FnCodeGenerator;
class CGScoper;

class ASTExpr : public ASTNode {
public:
    explicit ASTExpr(const SourcePosition &p) : ASTNode(p) {}
    /// Set after semantic analysis and transformation.
    /// Iff this node represents an expression type this type is the exact type produced by this node.
    const Type& expressionType() const { return expressionType_; }
    void setExpressionType(const Type &type) { expressionType_ = type; }
    void setTemporarilyScoped() { temporarilyScoped_ = true; }

    void generate(FnCodeGenerator *fncg) const;
    virtual Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) = 0;
    virtual void toCode(std::stringstream &stream) const = 0;
protected:
    virtual void generateExpr(FnCodeGenerator *fncg) const = 0;
private:
    Type expressionType_ = Type::nothingness();
    bool temporarilyScoped_ = false;
};

class ASTGetVariable final : public ASTExpr, public ASTVariable {
public:
    ASTGetVariable(std::u32string name, const SourcePosition &p) : ASTExpr(p), name_(std::move(name)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
    void toCode(std::stringstream &stream) const override;

    void setReference() { reference_ = true; }
    bool reference() { return reference_; }
    const std::u32string& name() { return name_; }
private:
    bool reference_ = false;
    std::u32string name_;
};

class ASTMetaTypeInstantiation final : public ASTExpr {
public:
    ASTMetaTypeInstantiation(Type type, const SourcePosition &p) : ASTExpr(p), type_(std::move(type)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
    void toCode(std::stringstream &stream) const override;
private:
    Type type_;
};

class ASTArguments final : public ASTNode {
public:
    explicit ASTArguments(const SourcePosition &p) : ASTNode(p) {}
    void addGenericArgument(const Type &type) { genericArguments_.emplace_back(type); }
    std::vector<Type>& genericArguments() { return genericArguments_; }
    void addArguments(const std::shared_ptr<ASTExpr> &arg) { arguments_.emplace_back(arg); }
    std::vector<std::shared_ptr<ASTExpr>>& arguments() { return arguments_; }
    const std::vector<std::shared_ptr<ASTExpr>>& arguments() const { return arguments_; }
    void toCode(std::stringstream &stream) const;
private:
    std::vector<Type> genericArguments_;
    std::vector<std::shared_ptr<ASTExpr>> arguments_;
};

class ASTCast final : public ASTExpr {
public:
    ASTCast(std::shared_ptr<ASTExpr> value, std::shared_ptr<ASTTypeExpr> type,
            const SourcePosition &p) : ASTExpr(p), value_(std::move(value)), typeExpr_(std::move(type)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
    void toCode(std::stringstream &stream) const override;
private:
    enum class CastType {
        ClassDowncast, ToClass, ToProtocol, ToValueType,
    };
    CastType castType_;
    std::shared_ptr<ASTExpr> value_;
    std::shared_ptr<ASTTypeExpr> typeExpr_;
};

class ASTCallableCall final : public ASTExpr {
public:
    ASTCallableCall(std::shared_ptr<ASTExpr> value, ASTArguments args,
                    const SourcePosition &p) : ASTExpr(p), callable_(std::move(value)), args_(std::move(args)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
    void toCode(std::stringstream &stream) const override;
private:
    std::shared_ptr<ASTExpr> callable_;
    ASTArguments args_;
};

class ASTSuperMethod final : public ASTExpr {
public:
    ASTSuperMethod(std::u32string name, ASTArguments args, const SourcePosition &p)
    : ASTExpr(p), name_(std::move(name)), args_(std::move(args)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
    void toCode(std::stringstream &stream) const override;
private:
    std::u32string name_;
    Type calleeType_ = Type::nothingness();
    ASTArguments args_;
};

class ASTCaptureMethod final : public ASTExpr {
public:
    ASTCaptureMethod(std::u32string name, std::shared_ptr<ASTExpr> callee, const SourcePosition &p)
    : ASTExpr(p), name_(std::move(name)), callee_(std::move(callee)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
    void toCode(std::stringstream &stream) const override;
private:
    std::u32string name_;
    std::shared_ptr<ASTExpr> callee_;
};

class ASTCaptureTypeMethod final : public ASTExpr {
public:
    ASTCaptureTypeMethod(std::u32string name, std::shared_ptr<ASTTypeExpr> callee,
                         const SourcePosition &p) : ASTExpr(p), name_(std::move(name)), callee_(std::move(callee)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
    void toCode(std::stringstream &stream) const override;
private:
    std::u32string name_;
    std::shared_ptr<ASTTypeExpr> callee_;
    bool contextedFunction_ = false;
};

class ASTTypeMethod final : public ASTExpr {
public:
    ASTTypeMethod(std::u32string name, std::shared_ptr<ASTTypeExpr> callee,
                  ASTArguments args, const SourcePosition &p)
    : ASTExpr(p), name_(std::move(name)), callee_(std::move(callee)), args_(std::move(args)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
    void toCode(std::stringstream &stream) const override;
private:
    bool valueType_ = false;
    std::u32string name_;
    const std::shared_ptr<ASTTypeExpr> callee_;
    ASTArguments args_;
};

class ASTConditionalAssignment final : public ASTExpr {
public:
    ASTConditionalAssignment(std::u32string varName, std::shared_ptr<ASTExpr> expr,
                             const SourcePosition &p) : ASTExpr(p), varName_(std::move(varName)), expr_(std::move(expr)) {}
    Type analyse(SemanticAnalyser *analyser, const TypeExpectation &expectation) override;
    void generateExpr(FnCodeGenerator *fncg) const override;
    void toCode(std::stringstream &stream) const override;
private:
    std::u32string varName_;
    std::shared_ptr<ASTExpr> expr_;
    VariableID varId_;
};
    
} // namespace EmojicodeCompiler

#endif /* ASTExpr_hpp */
