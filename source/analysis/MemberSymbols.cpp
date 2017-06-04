//------------------------------------------------------------------------------
// MemberSymbols.cpp
// Contains member-related symbol definitions.
//
// File is under the MIT license; see LICENSE for details.
//------------------------------------------------------------------------------
#include "Symbol.h"

#include "Binder.h"

namespace slang {

const BoundStatement& StatementBlockSymbol::bindStatement(const StatementSyntax& syntax) {
    return static_cast<const StatementBlockSymbol*>(this)->bindStatement(syntax);
}

const BoundStatementList& StatementBlockSymbol::bindStatementList(const SyntaxList<SyntaxNode>& items) {
    return static_cast<const StatementBlockSymbol*>(this)->bindStatementList(items);
}

const BoundStatement& StatementBlockSymbol::bindStatement(const StatementSyntax& syntax) const {
    switch (syntax.kind) {
        case SyntaxKind::ReturnStatement: return bindReturnStatement((const ReturnStatementSyntax&)syntax);
        case SyntaxKind::ConditionalStatement: return bindConditionalStatement((const ConditionalStatementSyntax&)syntax);
        case SyntaxKind::ForLoopStatement: return bindForLoopStatement((const ForLoopStatementSyntax&)syntax);
        case SyntaxKind::ExpressionStatement: return bindExpressionStatement((const ExpressionStatementSyntax&)syntax);

            DEFAULT_UNREACHABLE;
    }
    return badStmt(nullptr);
}

const BoundStatementList& StatementBlockSymbol::bindStatementList(const SyntaxList<SyntaxNode>& items) const {
    SmallVectorSized<const BoundStatement*, 8> buffer;
    for (const auto& item : items) {
        if (item->kind == SyntaxKind::DataDeclaration)
            bindVariableDecl(item->as<DataDeclarationSyntax>(), buffer);
        else if (isStatement(item->kind))
            buffer.append(&bindStatement(item->as<StatementSyntax>()));
    }

    const DesignRootSymbol& root = getRoot();
    return root.allocate<BoundStatementList>(buffer.copy(root.allocator()));
}

BoundStatement& StatementBlockSymbol::bindReturnStatement(const ReturnStatementSyntax& syntax) const {
    auto location = syntax.returnKeyword.location();
    const Symbol* subroutine = findAncestor(SymbolKind::Subroutine);
    if (!subroutine) {
        addError(DiagCode::ReturnNotInSubroutine, location);
        return badStmt(nullptr);
    }

    const auto& expr = Binder(*this).bindAssignmentLikeContext(*syntax.returnValue, location,
                                                               subroutine->as<SubroutineSymbol>().returnType());
    return allocate<BoundReturnStatement>(syntax, &expr);
}

BoundStatement& StatementBlockSymbol::bindConditionalStatement(const ConditionalStatementSyntax& syntax) const {
    ASSERT(syntax.predicate.conditions.count() == 1,
           "The &&& operator in if condition is not yet supported");
    ASSERT(!syntax.predicate.conditions[0]->matchesClause,
           "Pattern-matching is not yet supported");

    const auto& cond = Binder(*this).bindSelfDeterminedExpression(syntax.predicate.conditions[0]->expr);
    const auto& ifTrue = bindStatement(syntax.statement);
    const BoundStatement* ifFalse = nullptr;
    if (syntax.elseClause)
        ifFalse = &bindStatement(syntax.elseClause->clause.as<StatementSyntax>());

    return allocate<BoundConditionalStatement>(syntax, cond, ifTrue, ifFalse);
}

BoundStatement& StatementBlockSymbol::bindForLoopStatement(const ForLoopStatementSyntax& syntax) const {
    /*SmallVectorSized<const Symbol*, 2> initializers;
    SmallVectorSized<const BoundVariableDecl*, 2> boundVars;
    Scope *forScope = root.allocate<Scope>(scope);

    for (auto initializer : syntax.initializers) {
    auto forVarDecl = (const ForVariableDeclarationSyntax *)initializer;
    const TypeSymbol *type = sem.getType(forVarDecl->type, forScope);
    sem.handleVariableDeclarator(forVarDecl->declarator, initializers, forScope, {}, type);
    }
    ArrayRef<const Symbol*> initializersRef = initializers.copy(alloc);
    for (auto initializerSym : initializersRef) {
    boundVars.append(root.allocate<BoundVariableDecl>((const VariableSymbol*)initializerSym));
    }
    Binder binder(sem, forScope);
    auto stopExpr = binder.bindExpression(syntax.stopExpr);
    SmallVectorSized<const BoundExpression*, 2> steps;
    for (auto step : syntax.steps) {
    steps.append(binder.bindExpression(*step));
    }
    auto statement = binder.bindStatement(syntax.statement);

    return root.allocate<BoundForLoopStatement>(syntax, boundVars.copy(alloc), stopExpr, steps.copy(alloc), statement);*/

    return badStmt(nullptr);
}

void StatementBlockSymbol::bindVariableDecl(const DataDeclarationSyntax& syntax,
                                            SmallVector<const BoundStatement*>& results) const {
    // TODO: figure out const-ness of the scope here; shouldn't const cast obviously
    /*SmallVectorSized<const Symbol*, 8> buffer;
    sem.makeVariables(syntax, buffer, const_cast<Scope*>(scope));

    for (auto symbol : buffer)
    results.append(root.allocate<BoundVariableDecl>((const VariableSymbol*)symbol));*/
}

BoundStatement& StatementBlockSymbol::bindExpressionStatement(const ExpressionStatementSyntax& syntax) const {
    const auto& expr = Binder(*this).bindSelfDeterminedExpression(syntax.expr);
    return allocate<BoundExpressionStatement>(syntax, expr);
}

BoundStatement& StatementBlockSymbol::badStmt(const BoundStatement* stmt) const {
    return allocate<BadBoundStatement>(stmt);
}

ParameterSymbol::ParameterSymbol(StringRef name, SourceLocation location, const Symbol& parent) :
    Symbol(SymbolKind::Parameter, parent, name, location) {}

VariableSymbol::VariableSymbol(Token name, const DataTypeSyntax& type, const Symbol& parent, VariableLifetime lifetime,
                               bool isConst, const ExpressionSyntax* initializer) :
    Symbol(SymbolKind::Variable, name, parent),
    lifetime(lifetime), isConst(isConst), typeSyntax(&type), initializerSyntax(initializer)
{
}

VariableSymbol::VariableSymbol(StringRef name, SourceLocation location, const TypeSymbol& type, const Symbol& parent,
                               VariableLifetime lifetime, bool isConst, const BoundExpression* initializer) :
    Symbol(SymbolKind::Variable, parent, name, location),
    lifetime(lifetime), isConst(isConst), typeSymbol(&type), initializerBound(initializer)
{
}

VariableSymbol::VariableSymbol(SymbolKind kind, StringRef name, SourceLocation location, const TypeSymbol& type,
                               const Symbol& parent, VariableLifetime lifetime, bool isConst, const BoundExpression* initializer) :
    Symbol(kind, parent, name, location),
    lifetime(lifetime), isConst(isConst), typeSymbol(&type), initializerBound(initializer)
{
}

const TypeSymbol& VariableSymbol::type() const {
    if (typeSymbol)
        return *typeSymbol;

    ASSERT(typeSyntax);
    typeSymbol = &containingScope().getType(*typeSyntax);
    return *typeSymbol;
}

const BoundExpression* VariableSymbol::initializer() const {
    if (initializerBound)
        return initializerBound;

    if (initializerSyntax)
        initializerBound = &Binder(containingScope()).bindAssignmentLikeContext(*initializerSyntax, location, type());

    return initializerBound;
}

FormalArgumentSymbol::FormalArgumentSymbol(const TypeSymbol& type, const Symbol& parent) :
    VariableSymbol(SymbolKind::FormalArgument, nullptr, SourceLocation(), type, parent)
{
}

FormalArgumentSymbol::FormalArgumentSymbol(StringRef name, SourceLocation location, const TypeSymbol& type,
                                           const Symbol& parent, const BoundExpression* initializer,
                                           FormalArgumentDirection direction) :
    VariableSymbol(SymbolKind::FormalArgument, name, location, type, parent, VariableLifetime::Automatic,
                   direction == FormalArgumentDirection::ConstRef, initializer),
    direction(direction)
{
}

// TODO: handle functions that don't have simple name tokens
SubroutineSymbol::SubroutineSymbol(const FunctionDeclarationSyntax& syntax, const Symbol& parent) :
    StatementBlockSymbol(SymbolKind::Subroutine, syntax.prototype.name.getFirstToken(), parent),
    syntax(&syntax)
{
    defaultLifetime = getLifetimeFromToken(syntax.prototype.lifetime, VariableLifetime::Automatic);
    isTask = syntax.kind == SyntaxKind::TaskDeclaration;
}

SubroutineSymbol::SubroutineSymbol(StringRef name, const TypeSymbol& returnType, ArrayRef<const FormalArgumentSymbol*> arguments,
                                   SystemFunction systemFunction, const Symbol& parent) :
    StatementBlockSymbol(SymbolKind::Subroutine, parent, name),
    systemFunctionKind(systemFunction), returnType_(&returnType),
    arguments_(arguments), initialized(true)
{
}

void SubroutineSymbol::init() const {
    if (initialized)
        return;
    initialized = true;

    const ScopeSymbol& parentScope = containingScope();
    const DesignRootSymbol& root = getRoot();
    const auto& proto = syntax->prototype;
    auto returnType = parentScope.getType(*proto.returnType);

    SmallVectorSized<const FormalArgumentSymbol*, 8> arguments;

    if (proto.portList) {
        const TypeSymbol* lastType = &root.getKnownType(SyntaxKind::LogicType);
        auto lastDirection = FormalArgumentDirection::In;

        for (const FunctionPortSyntax* portSyntax : proto.portList->ports) {
            FormalArgumentDirection direction;
            bool directionSpecified = true;
            switch (portSyntax->direction.kind) {
                case TokenKind::InputKeyword: direction = FormalArgumentDirection::In; break;
                case TokenKind::OutputKeyword: direction = FormalArgumentDirection::Out; break;
                case TokenKind::InOutKeyword: direction = FormalArgumentDirection::InOut; break;
                case TokenKind::RefKeyword:
                    if (portSyntax->constKeyword)
                        direction = FormalArgumentDirection::ConstRef;
                    else
                        direction = FormalArgumentDirection::Ref;
                    break;
                default:
                    // Otherwise, we "inherit" the previous argument
                    direction = lastDirection;
                    directionSpecified = false;
                    break;
            }

            // If we're given a type, use that. Otherwise, if we were given a
            // direction, default to logic. Otherwise, use the last type.
            const TypeSymbol* type;
            if (portSyntax->dataType)
                type = &parentScope.getType(*portSyntax->dataType);
            else if (directionSpecified)
                type = &root.getKnownType(SyntaxKind::LogicType);
            else
                type = lastType;

            const auto& declarator = portSyntax->declarator;
            const BoundExpression* initializer = nullptr;
            if (declarator.initializer) {
                initializer = &Binder(parentScope).bindAssignmentLikeContext(declarator.initializer->expr,
                                                                             declarator.name.location(), *type);
            }

            arguments.append(&root.allocate<FormalArgumentSymbol>(
                declarator.name.valueText(),
                declarator.name.location(),
                *type,
                *this,
                initializer,
                direction
                ));

            addMember(*arguments.back());

            lastDirection = direction;
            lastType = type;
        }
    }

    returnType_ = &returnType;
    body_ = &bindStatementList(syntax->items);
    arguments_ = arguments.copy(root.allocator());
}

}