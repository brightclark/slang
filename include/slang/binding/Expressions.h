//------------------------------------------------------------------------------
// Expressions.h
// Expression creation and analysis.
//
// File is under the MIT license; see LICENSE for details.
//------------------------------------------------------------------------------
#pragma once

#include "slang/binding/BindContext.h"
#include "slang/binding/EvalContext.h"

namespace slang {

class Symbol;
class FieldSymbol;
class SubroutineSymbol;
class ValueSymbol;
class SystemSubroutine;
class Type;
struct AssignmentPatternExpressionSyntax;
struct ElementSelectExpressionSyntax;
struct ExpressionSyntax;
struct InvocationExpressionSyntax;

// clang-format off
#define EXPRESSION(x) \
    x(Invalid) \
    x(IntegerLiteral) \
    x(RealLiteral) \
    x(UnbasedUnsizedIntegerLiteral) \
    x(NullLiteral) \
    x(StringLiteral) \
    x(NamedValue) \
    x(UnaryOp) \
    x(BinaryOp) \
    x(ConditionalOp) \
    x(Inside) \
    x(Assignment) \
    x(Concatenation) \
    x(Replication) \
    x(ElementSelect) \
    x(RangeSelect) \
    x(MemberAccess) \
    x(Call) \
    x(Conversion) \
    x(DataType) \
    x(SimpleAssignmentPattern) \
    x(StructuredAssignmentPattern) \
    x(ReplicatedAssignmentPattern) \
    x(EmptyArgument) \
    x(OpenRange)
ENUM(ExpressionKind, EXPRESSION);
#undef EXPRESION

#define OP(x) \
    x(Plus) \
    x(Minus) \
    x(BitwiseNot) \
    x(BitwiseAnd) \
    x(BitwiseOr) \
    x(BitwiseXor) \
    x(BitwiseNand) \
    x(BitwiseNor) \
    x(BitwiseXnor) \
    x(LogicalNot) \
    x(Preincrement) \
    x(Predecrement) \
    x(Postincrement) \
    x(Postdecrement)
ENUM(UnaryOperator, OP);
#undef OP

#define OP(x) \
    x(Add) \
    x(Subtract) \
    x(Multiply) \
    x(Divide) \
    x(Mod) \
    x(BinaryAnd) \
    x(BinaryOr) \
    x(BinaryXor) \
    x(BinaryXnor) \
    x(Equality) \
    x(Inequality) \
    x(CaseEquality) \
    x(CaseInequality) \
    x(GreaterThanEqual) \
    x(GreaterThan) \
    x(LessThanEqual) \
    x(LessThan) \
    x(WildcardEquality) \
    x(WildcardInequality) \
    x(LogicalAnd) \
    x(LogicalOr) \
    x(LogicalImplication) \
    x(LogicalEquivalence) \
    x(LogicalShiftLeft) \
    x(LogicalShiftRight) \
    x(ArithmeticShiftLeft) \
    x(ArithmeticShiftRight) \
    x(Power)
ENUM(BinaryOperator, OP);
#undef OP

#define RANGE(x) x(Simple) x(IndexedUp) x(IndexedDown)
ENUM(RangeSelectionKind, RANGE);
#undef RANGE
// clang-format on

/// The base class for all expressions in SystemVerilog.
class Expression {
public:
    /// The kind of expression; indicates the type of derived class.
    ExpressionKind kind;

    /// The type of the expression.
    not_null<const Type*> type;

    /// The value of the expression, if it's constant. Otherwise nullptr.
    const ConstantValue* constant = nullptr;

    /// The syntax used to create the expression, if any. An expression tree can
    /// be created manually in which case it may not have a syntax representation.
    const ExpressionSyntax* syntax = nullptr;

    /// The source range of this expression, if it originated from source code.
    SourceRange sourceRange;

    /// Binds an expression tree from the given syntax nodes.
    static const Expression& bind(const ExpressionSyntax& syntax, const BindContext& context,
                                  bitmask<BindFlags> extraFlags = BindFlags::None);

    /// Binds an assignment-like expression from the given syntax nodes.
    static const Expression& bind(const Type& lhs, const ExpressionSyntax& rhs,
                                  SourceLocation location, const BindContext& context);

    /// Converts the given expression to the specified type, as if the right hand side had been
    /// assigned (without a cast) to a left hand side of the specified type.
    static Expression& convertAssignment(const BindContext& context, const Type& type,
                                         Expression& expr, SourceLocation location,
                                         optional<SourceRange> lhsRange = std::nullopt);

    /// Specialized method for binding all of the expressions in a set membership check.
    /// This is used for case statements and the 'inside' operator.
    ///
    /// The @param valueExpr is the value being checked for membership, and @param expressions
    /// denotes the set to check within. All of the expressions influence each other for
    /// purposes of finding a common comparison type.
    ///
    /// The @param keyword parameter is used to customize diagnostics produced.
    ///
    /// If @param wildcard is set to true, expression types will be restricted to
    /// be only integral types.
    ///
    /// If @param unwrapUnpacked is set to true, unpacked arrays will be unwrapped to
    /// their element types to find the type to check against. Otherwise, all aggregates
    /// are illegal.
    ///
    /// @returns true if all expressions are legal, otherwise false and appropriate
    /// diagnostics are issued.
    static bool bindMembershipExpressions(const BindContext& context, TokenKind keyword,
                                          bool wildcard, bool unwrapUnpacked,
                                          const ExpressionSyntax& valueExpr,
                                          span<const ExpressionSyntax* const> expressions,
                                          SmallVector<const Expression*>& results);

    /// Indicates whether the expression is invalid.
    bool bad() const;

    /// Indicates whether the expression evaluates to an lvalue.
    bool isLValue() const;

    /// Indicates whether the expression is of type string, or if it
    /// is implicitly convertible to a string.
    bool isImplicitString() const;

    /// Evaluates the expression under the given evaluation context. Any errors that occur
    /// will be stored in the evaluation context instead of issued to the compilation.
    ConstantValue eval(EvalContext& context) const;

    /// Evaluates an expression as an lvalue. Note that this will throw an exception
    /// if the expression does not represent an lvalue.
    LValue evalLValue(EvalContext& context) const;

    /// Verifies that this expression is valid as a constant expression.
    /// If it's not, appropriate diagnostics will be issued.
    bool verifyConstant(EvalContext& context) const;

    template<typename T>
    T& as() {
        ASSERT(T::isKind(kind));
        return *static_cast<T*>(this);
    }

    template<typename T>
    const T& as() const {
        ASSERT(T::isKind(kind));
        return *static_cast<const T*>(this);
    }

    template<typename TVisitor, typename... Args>
    decltype(auto) visit(TVisitor& visitor, Args&&... args);

    template<typename TVisitor, typename... Args>
    decltype(auto) visit(TVisitor& visitor, Args&&... args) const;

    /// Serialization of arbitrary expressions to JSON.
    friend void to_json(json& j, const Expression& expr);

protected:
    Expression(ExpressionKind kind, const Type& type, SourceRange sourceRange) :
        kind(kind), type(&type), sourceRange(sourceRange) {}

    static UnaryOperator getUnaryOperator(SyntaxKind kind);
    static BinaryOperator getBinaryOperator(SyntaxKind kind);

    static const Type* binaryOperatorType(Compilation& compilation, const Type* lt, const Type* rt,
                                          bool forceFourState);

    static ConstantValue evalBinaryOperator(BinaryOperator op, const ConstantValue& cvl,
                                            const ConstantValue& cvr);

    static void checkBindFlags(const Expression& expr, const BindContext& context);

    static Expression& create(Compilation& compilation, const ExpressionSyntax& syntax,
                              const BindContext& context,
                              bitmask<BindFlags> extraFlags = BindFlags::None,
                              const Type* assignmentTarget = nullptr);
    static Expression& implicitConversion(const BindContext& context, const Type& type,
                                          Expression& expr);

    static Expression& bindName(Compilation& compilation, const NameSyntax& syntax,
                                const InvocationExpressionSyntax* invocation,
                                const BindContext& context);
    static Expression& bindSelectExpression(Compilation& compilation,
                                            const ElementSelectExpressionSyntax& syntax,
                                            const BindContext& context);
    static Expression& bindSelector(Compilation& compilation, Expression& value,
                                    const ElementSelectSyntax& syntax, const BindContext& context);

    static Expression& bindAssignmentPattern(Compilation& compilation,
                                             const AssignmentPatternExpressionSyntax& syntax,
                                             const BindContext& context,
                                             const Type* assignmentTarget);

    static Expression& badExpr(Compilation& compilation, const Expression* expr);

    // Perform type propagation and constant folding of a context-determined subexpression.
    static void contextDetermined(const BindContext& context, Expression*& expr,
                                  const Type& newType);

    // Perform type propagation and constant folding of a self-determined subexpression.
    static void selfDetermined(const BindContext& context, Expression*& expr);
    [[nodiscard]] static Expression& selfDetermined(
        Compilation& compilation, const ExpressionSyntax& syntax, const BindContext& context,
        bitmask<BindFlags> extraFlags = BindFlags::None);
    struct PropagationVisitor;
};

/// Represents an invalid expression, which is usually generated and inserted
/// into an expression tree due to violation of language semantics or type checking.
class InvalidExpression : public Expression {
public:
    /// A wrapped sub-expression that is considered invalid.
    const Expression* child;

    InvalidExpression(const Expression* child, const Type& type) :
        Expression(ExpressionKind::Invalid, type, SourceRange()), child(child) {}

    void toJson(json& j) const;

    static bool isKind(ExpressionKind kind) { return kind == ExpressionKind::Invalid; }

    static const InvalidExpression Instance;
};

/// Represents an expression that references a named value.
class NamedValueExpression : public Expression {
public:
    const ValueSymbol& symbol;
    bool isHierarchical;

    NamedValueExpression(const ValueSymbol& symbol, bool isHierarchical, SourceRange sourceRange) :
        Expression(ExpressionKind::NamedValue, symbol.getType(), sourceRange), symbol(symbol),
        isHierarchical(isHierarchical) {}

    ConstantValue evalImpl(EvalContext& context) const;
    LValue evalLValueImpl(EvalContext& context) const;
    bool verifyConstantImpl(EvalContext& context) const;

    void toJson(json& j) const;

    static Expression& fromSymbol(const Scope& scope, const Symbol& symbol, bool isHierarchical,
                                  SourceRange sourceRange);

    static bool isKind(ExpressionKind kind) { return kind == ExpressionKind::NamedValue; }
};

/// Represents a single element selection expression.
class ElementSelectExpression : public Expression {
public:
    ElementSelectExpression(const Type& type, Expression& value, Expression& selector,
                            SourceRange sourceRange) :
        Expression(ExpressionKind::ElementSelect, type, sourceRange),
        value_(&value), selector_(&selector) {}

    const Expression& value() const { return *value_; }
    Expression& value() { return *value_; }

    const Expression& selector() const { return *selector_; }
    Expression& selector() { return *selector_; }

    ConstantValue evalImpl(EvalContext& context) const;
    LValue evalLValueImpl(EvalContext& context) const;
    bool verifyConstantImpl(EvalContext& context) const;

    void toJson(json& j) const;

    static Expression& fromSyntax(Compilation& compilation, Expression& value,
                                  const ExpressionSyntax& syntax, SourceRange fullRange,
                                  const BindContext& context);

    static bool isKind(ExpressionKind kind) { return kind == ExpressionKind::ElementSelect; }

private:
    Expression* value_;
    Expression* selector_;
};

struct RangeSelectSyntax;

/// Represents a range selection expression.
class RangeSelectExpression : public Expression {
public:
    RangeSelectionKind selectionKind;

    RangeSelectExpression(RangeSelectionKind selectionKind, const Type& type, Expression& value,
                          const Expression& left, const Expression& right,
                          SourceRange sourceRange) :
        Expression(ExpressionKind::RangeSelect, type, sourceRange),
        selectionKind(selectionKind), value_(&value), left_(&left), right_(&right) {}

    const Expression& value() const { return *value_; }
    Expression& value() { return *value_; }

    const Expression& left() const { return *left_; }
    const Expression& right() const { return *right_; }

    ConstantValue evalImpl(EvalContext& context) const;
    LValue evalLValueImpl(EvalContext& context) const;
    bool verifyConstantImpl(EvalContext& context) const;

    void toJson(json& j) const;

    static Expression& fromSyntax(Compilation& compilation, Expression& value,
                                  const RangeSelectSyntax& syntax, SourceRange fullRange,
                                  const BindContext& context);

    static bool isKind(ExpressionKind kind) { return kind == ExpressionKind::RangeSelect; }

private:
    static ConstantRange getIndexedRange(RangeSelectionKind kind, int32_t l, int32_t r,
                                         bool littleEndian);
    optional<ConstantRange> getRange(EvalContext& context, const ConstantValue& cl,
                                     const ConstantValue& cr) const;

    Expression* value_;
    const Expression* left_;
    const Expression* right_;
};

struct MemberAccessExpressionSyntax;

/// Represents an access of a structure variable's members.
class MemberAccessExpression : public Expression {
public:
    const FieldSymbol& field;

    MemberAccessExpression(const Type& type, Expression& value, const FieldSymbol& field,
                           SourceRange sourceRange) :
        Expression(ExpressionKind::MemberAccess, type, sourceRange),
        field(field), value_(&value) {}

    const Expression& value() const { return *value_; }
    Expression& value() { return *value_; }

    ConstantValue evalImpl(EvalContext& context) const;
    LValue evalLValueImpl(EvalContext& context) const;
    bool verifyConstantImpl(EvalContext& context) const;

    void toJson(json& j) const;

    static Expression& fromSelector(Compilation& compilation, Expression& expr,
                                    const LookupResult::MemberSelector& selector,
                                    const InvocationExpressionSyntax* invocation,
                                    const BindContext& context);

    static Expression& fromSyntax(Compilation& compilation,
                                  const MemberAccessExpressionSyntax& syntax,
                                  const InvocationExpressionSyntax* invocation,
                                  const BindContext& context);

    static bool isKind(ExpressionKind kind) { return kind == ExpressionKind::MemberAccess; }

private:
    Expression* value_;
};

/// Represents a subroutine call.
class CallExpression : public Expression {
public:
    using Subroutine = std::variant<const SubroutineSymbol*, const SystemSubroutine*>;
    Subroutine subroutine;

    CallExpression(const Subroutine& subroutine, const Type& returnType,
                   span<const Expression*> arguments, LookupLocation lookupLocation,
                   SourceRange sourceRange) :
        Expression(ExpressionKind::Call, returnType, sourceRange),
        subroutine(subroutine), arguments_(arguments), lookupLocation(lookupLocation) {}

    span<const Expression* const> arguments() const { return arguments_; }
    span<const Expression*> arguments() { return arguments_; }

    bool isSystemCall() const { return subroutine.index() == 1; }

    string_view getSubroutineName() const;

    ConstantValue evalImpl(EvalContext& context) const;
    bool verifyConstantImpl(EvalContext& context) const;

    void toJson(json& j) const;

    static Expression& fromSyntax(Compilation& compilation,
                                  const InvocationExpressionSyntax& syntax,
                                  const BindContext& context);

    static Expression& fromLookup(Compilation& compilation, const Subroutine& subroutine,
                                  const InvocationExpressionSyntax* syntax, SourceRange range,
                                  const BindContext& context);

    static Expression& fromSystemMethod(Compilation& compilation, const Expression& expr,
                                        const LookupResult::MemberSelector& selector,
                                        const InvocationExpressionSyntax* syntax,
                                        const BindContext& context);

    static bool isKind(ExpressionKind kind) { return kind == ExpressionKind::Call; }

private:
    static Expression& createSystemCall(Compilation& compilation,
                                        const SystemSubroutine& subroutine,
                                        const Expression* firstArg,
                                        const InvocationExpressionSyntax* syntax, SourceRange range,
                                        const BindContext& context);

    span<const Expression*> arguments_;
    LookupLocation lookupLocation;
};

struct CastExpressionSyntax;
struct SignedCastExpressionSyntax;

/// Represents a type conversion expression.
class ConversionExpression : public Expression {
public:
    bool isImplicit;

    ConversionExpression(const Type& type, bool isImplicit, Expression& operand,
                         SourceRange sourceRange) :
        Expression(ExpressionKind::Conversion, type, sourceRange),
        isImplicit(isImplicit), operand_(&operand) {}

    const Expression& operand() const { return *operand_; }
    Expression& operand() { return *operand_; }

    ConstantValue evalImpl(EvalContext& context) const;
    bool verifyConstantImpl(EvalContext& context) const;

    void toJson(json& j) const;

    static Expression& fromSyntax(Compilation& compilation, const CastExpressionSyntax& syntax,
                                  const BindContext& context);
    static Expression& fromSyntax(Compilation& compilation,
                                  const SignedCastExpressionSyntax& syntax,
                                  const BindContext& context);

    static bool isKind(ExpressionKind kind) { return kind == ExpressionKind::Conversion; }

private:
    Expression* operand_;
};

/// Adapts a data type for use in an expression tree. This is for cases where both an expression
/// and a data type is valid; for example, as an argument to a $bits() call or as a parameter
/// assignment (because of type parameters).
class DataTypeExpression : public Expression {
public:
    DataTypeExpression(const Type& type, SourceRange sourceRange) :
        Expression(ExpressionKind::DataType, type, sourceRange) {}

    ConstantValue evalImpl(EvalContext& context) const;
    bool verifyConstantImpl(EvalContext&) const { return true; }

    void toJson(json&) const {}

    static Expression& fromSyntax(Compilation& compilation, const DataTypeSyntax& syntax,
                                  const BindContext& context);

    static bool isKind(ExpressionKind kind) { return kind == ExpressionKind::DataType; }
};

/// Base class for assignment pattern expressions.
class AssignmentPatternExpressionBase : public Expression {
public:
    span<const Expression* const> elements() const { return elements_; }

    ConstantValue evalImpl(EvalContext& context) const;
    bool verifyConstantImpl(EvalContext&) const;

    void toJson(json& j) const;

protected:
    AssignmentPatternExpressionBase(ExpressionKind kind, const Type& type,
                                    span<const Expression* const> elements,
                                    SourceRange sourceRange) :
        Expression(kind, type, sourceRange),
        elements_(elements) {}

private:
    span<const Expression* const> elements_;
};

struct SimpleAssignmentPatternSyntax;

/// Represents an assignment pattern expression.
class SimpleAssignmentPatternExpression : public AssignmentPatternExpressionBase {
public:
    SimpleAssignmentPatternExpression(const Type& type, span<const Expression* const> elements,
                                      SourceRange sourceRange) :
        AssignmentPatternExpressionBase(ExpressionKind::SimpleAssignmentPattern, type, elements,
                                        sourceRange) {}

    static Expression& forStruct(Compilation& compilation,
                                 const SimpleAssignmentPatternSyntax& syntax,
                                 const BindContext& context, const Type& type,
                                 const Scope& structScope, SourceRange sourceRange);

    static Expression& forArray(Compilation& compilation,
                                const SimpleAssignmentPatternSyntax& syntax,
                                const BindContext& context, const Type& type,
                                const Type& elementType, bitwidth_t numElements,
                                SourceRange sourceRange);

    static bool isKind(ExpressionKind kind) {
        return kind == ExpressionKind::SimpleAssignmentPattern;
    }
};

struct StructuredAssignmentPatternSyntax;

/// Represents an assignment pattern expression.
class StructuredAssignmentPatternExpression : public AssignmentPatternExpressionBase {
public:
    struct MemberSetter {
        const Symbol* member = nullptr;
        const Expression* expr = nullptr;
    };

    struct TypeSetter {
        const Type* type = nullptr;
        const Expression* expr = nullptr;
    };

    struct IndexSetter {
        const Expression* index = nullptr;
        const Expression* expr = nullptr;
    };

    span<const MemberSetter> memberSetters;
    span<const TypeSetter> typeSetters;
    span<const IndexSetter> indexSetters;
    const Expression* defaultSetter;

    StructuredAssignmentPatternExpression(const Type& type, span<const MemberSetter> memberSetters,
                                          span<const TypeSetter> typeSetters,
                                          span<const IndexSetter> indexSetters,
                                          const Expression* defaultSetter,
                                          span<const Expression* const> elements,
                                          SourceRange sourceRange) :
        AssignmentPatternExpressionBase(ExpressionKind::StructuredAssignmentPattern, type, elements,
                                        sourceRange),
        memberSetters(memberSetters), typeSetters(typeSetters), indexSetters(indexSetters),
        defaultSetter(defaultSetter) {}

    void toJson(json& j) const;

    static Expression& forStruct(Compilation& compilation,
                                 const StructuredAssignmentPatternSyntax& syntax,
                                 const BindContext& context, const Type& type,
                                 const Scope& structScope, SourceRange sourceRange);

    static Expression& forArray(Compilation& compilation,
                                const StructuredAssignmentPatternSyntax& syntax,
                                const BindContext& context, const Type& type,
                                const Type& elementType, SourceRange sourceRange);

    static bool isKind(ExpressionKind kind) {
        return kind == ExpressionKind::StructuredAssignmentPattern;
    }
};

struct ReplicatedAssignmentPatternSyntax;

/// Represents a replicated assignment pattern expression.
class ReplicatedAssignmentPatternExpression : public AssignmentPatternExpressionBase {
public:
    ReplicatedAssignmentPatternExpression(const Type& type, const Expression& count,
                                          span<const Expression* const> elements,
                                          SourceRange sourceRange) :
        AssignmentPatternExpressionBase(ExpressionKind::ReplicatedAssignmentPattern, type, elements,
                                        sourceRange),
        count_(&count) {}

    const Expression& count() const { return *count_; }

    void toJson(json& j) const;

    static Expression& forStruct(Compilation& compilation,
                                 const ReplicatedAssignmentPatternSyntax& syntax,
                                 const BindContext& context, const Type& type,
                                 const Scope& structScope, SourceRange sourceRange);

    static Expression& forArray(Compilation& compilation,
                                const ReplicatedAssignmentPatternSyntax& syntax,
                                const BindContext& context, const Type& type,
                                const Type& elementType, bitwidth_t numElements,
                                SourceRange sourceRange);

    static bool isKind(ExpressionKind kind) {
        return kind == ExpressionKind::ReplicatedAssignmentPattern;
    }

private:
    static const Expression& bindReplCount(Compilation& comp, const ExpressionSyntax& syntax,
                                           const BindContext& context, size_t& count);

    const Expression* count_;
};

/// Represents an empty argument. There's no actual syntax to go along with this,
/// and the type is always invalid, but we use this as a placeholder to hold the fact
/// that the argument is empty.
class EmptyArgumentExpression : public Expression {
public:
    EmptyArgumentExpression(const Type& type, SourceRange sourceRange) :
        Expression(ExpressionKind::EmptyArgument, type, sourceRange) {}

    ConstantValue evalImpl(EvalContext&) const { return nullptr; }
    bool verifyConstantImpl(EvalContext&) const { return true; }

    void toJson(json&) const {}

    static bool isKind(ExpressionKind kind) { return kind == ExpressionKind::EmptyArgument; }
};

} // namespace slang
