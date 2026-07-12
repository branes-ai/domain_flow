#pragma once
// Text front-end for the SURE simulator: parse the recurrence-system DSL used
// throughout docs/SURE/ into an executable RecurrenceSystem (issue #15).
//
// Accepted grammar (see docs/SURE/matmul.sure for a worked example):
//
//   M = 2; K = 3;                              // integer parameters
//   input A[2][3] = { 1, 2, 3, 4, 5, 6 };      // row-major data, or scalar fill
//   system ((i,j,k) | 0 <= i < M, ...) {       // shared iteration domain
//       a(i,j,k) = a(i,j-1,k);                 // equations: taps are var(affine...)
//       c(i,j,k) = c(i,j,k-1) + a(i,j-1,k) * b(i-1,j,k);
//   }
//   boundary a(i,j,k) = A[i][k];               // value when a tap exits the domain
//   boundary c(i,j,k) = 0;                     //   (default 0 when omitted)
//   tau = [1, 1, 1];                           // optional canonical linear schedule
//   output ((i,j) | 0 <= i < M, ...) c(i, j, K-1);
//
// Notation notes:
//   - recurrence variables are read with parentheses: a(i,j-1,k) -- these become
//     AffineDependency taps; input arrays are read with brackets: A[i][k] and may
//     only appear in boundary expressions
//   - constraints are affine: chains over bare indices (0 <= i,j,k < N) or binary
//     relations between affine expressions (k <= j); every index needs constant
//     lower and upper bounds so the domain enumeration box is defined
//   - expression bodies support + - * /, unary minus, parentheses, and
//     sqrt/exp/abs; the docs' ternary boundary idiom ((k==0) ? ... : ...) is
//     expressed through the boundary mechanism instead
//
// v1 scope: all equations share the system domain (matches docs/SURE/matmul.md
// and conv2d.md); per-equation domains (QR) are a follow-up.
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cmath>
#include <cctype>
#include <limits>
#include <stdexcept>
#include <dfa/sim/sure_simulator.hpp>

namespace sw {
    namespace dfa {
        namespace sim {

            // named input data array (row-major)
            struct SureInput {
                std::vector<long> dims;
                std::vector<double> data;
            };

            // one output declaration: enumerate `domain` and read var at map(p)
            struct SureOutput {
                std::string var;
                std::vector<std::string> indexNames;   // output index names, for printing
                Domain domain;
                AffineDependency map;                  // output point -> system point

                SureOutput() : var{}, indexNames{}, domain{}, map(AffineDependency::shift({})) {}
            };

            // a parsed, executable SURE program
            struct SureSpec {
                RecurrenceSystem<double> system;
                std::vector<std::string> indexNames;   // system index names, for reporting
                std::vector<SureOutput> outputs;
                std::vector<int> tau;                  // empty => free schedule only
            };

            namespace suredetail {

                // ── expression AST ─────────────────────────────────────────
                struct Expr;
                using ExprPtr = std::shared_ptr<Expr>;

                struct Expr {
                    enum class Kind { Number, TapRef, ArrayRef, Unary, Binary, Call };
                    Kind kind = Kind::Number;
                    double number = 0.0;
                    std::size_t tapSlot = 0;                    // TapRef
                    std::string name;                           // ArrayRef array / Call function
                    std::vector<std::vector<long>> idxCoeffs;   // ArrayRef: per-dim coeffs over system indices
                    std::vector<long> idxConst;                 // ArrayRef: per-dim constants
                    char op = 0;                                // Unary '-', Binary '+','-','*','/'
                    ExprPtr lhs, rhs;                           // Binary; Unary/Call use lhs
                };

                inline double evalExpr(const Expr& e,
                                       const std::vector<double>* taps,
                                       const IndexPoint* p,
                                       const std::map<std::string, SureInput>* inputs) {
                    switch (e.kind) {
                    case Expr::Kind::Number: return e.number;
                    case Expr::Kind::TapRef:
                        if (!taps) throw std::runtime_error("sure: tap reference in a boundary expression");
                        return (*taps)[e.tapSlot];
                    case Expr::Kind::ArrayRef: {
                        if (!p || !inputs) throw std::runtime_error("sure: array reference in an equation body");
                        auto it = inputs->find(e.name);
                        if (it == inputs->end()) throw std::runtime_error("sure: unknown input array '" + e.name + "'");
                        const SureInput& arr = it->second;
                        long flat = 0;
                        for (std::size_t d = 0; d < arr.dims.size(); ++d) {
                            long v = e.idxConst[d];
                            for (std::size_t i = 0; i < e.idxCoeffs[d].size() && i < p->size(); ++i)
                                v += e.idxCoeffs[d][i] * (*p)[i];
                            if (v < 0 || v >= arr.dims[d])
                                throw std::runtime_error("sure: index out of range reading input '" + e.name + "'");
                            flat = flat * arr.dims[d] + v;
                        }
                        return arr.data[static_cast<std::size_t>(flat)];
                    }
                    case Expr::Kind::Unary:
                        return -evalExpr(*e.lhs, taps, p, inputs);
                    case Expr::Kind::Binary: {
                        double a = evalExpr(*e.lhs, taps, p, inputs);
                        double b = evalExpr(*e.rhs, taps, p, inputs);
                        switch (e.op) {
                        case '+': return a + b;
                        case '-': return a - b;
                        case '*': return a * b;
                        default:  return b != 0.0 ? a / b : 0.0;
                        }
                    }
                    case Expr::Kind::Call: {
                        double a = evalExpr(*e.lhs, taps, p, inputs);
                        if (e.name == "sqrt") return std::sqrt(a);
                        if (e.name == "exp")  return std::exp(a);
                        return std::abs(a);   // "abs"
                    }
                    }
                    return 0.0;
                }

                // ── tokenizer ──────────────────────────────────────────────
                struct Token {
                    enum class Kind { Ident, Number, Punct, End };
                    Kind kind = Kind::End;
                    std::string text;
                    double number = 0.0;
                    int line = 0;
                };

                inline std::vector<Token> tokenize(const std::string& src) {
                    std::vector<Token> toks;
                    int line = 1;
                    std::size_t i = 0;
                    const std::size_t n = src.size();
                    while (i < n) {
                        char c = src[i];
                        if (c == '\n') { ++line; ++i; continue; }
                        if (std::isspace(static_cast<unsigned char>(c))) { ++i; continue; }
                        if (c == '/' && i + 1 < n && src[i + 1] == '/') {          // // comment
                            while (i < n && src[i] != '\n') ++i;
                            continue;
                        }
                        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                            std::size_t s = i;
                            while (i < n && (std::isalnum(static_cast<unsigned char>(src[i])) || src[i] == '_')) ++i;
                            toks.push_back({ Token::Kind::Ident, src.substr(s, i - s), 0.0, line });
                            continue;
                        }
                        if (std::isdigit(static_cast<unsigned char>(c)) ||
                            (c == '.' && i + 1 < n && std::isdigit(static_cast<unsigned char>(src[i + 1])))) {
                            std::size_t s = i;
                            while (i < n && (std::isdigit(static_cast<unsigned char>(src[i])) || src[i] == '.')) ++i;
                            std::string t = src.substr(s, i - s);
                            toks.push_back({ Token::Kind::Number, t, std::stod(t), line });
                            continue;
                        }
                        // multi-char relations
                        if ((c == '<' || c == '>' || c == '=' || c == '!') && i + 1 < n && src[i + 1] == '=') {
                            toks.push_back({ Token::Kind::Punct, src.substr(i, 2), 0.0, line });
                            i += 2;
                            continue;
                        }
                        toks.push_back({ Token::Kind::Punct, std::string(1, c), 0.0, line });
                        ++i;
                    }
                    toks.push_back({ Token::Kind::End, "", 0.0, line });
                    return toks;
                }

                // affine form over the current index vector: a . x + b
                struct Affine {
                    std::vector<long> a;
                    long b = 0;
                    explicit Affine(std::size_t dim) : a(dim, 0), b(0) {}
                    bool isConst() const {
                        for (long c : a) if (c != 0) return false;
                        return true;
                    }
                };

                // one normalized constraint:  a . x  {>=,==}  rhs
                struct LinCon {
                    std::vector<long> a;
                    long rhs = 0;
                    bool isEq = false;
                };

                // ── parser ─────────────────────────────────────────────────
                class Parser {
                public:
                    explicit Parser(const std::string& src) : toks(tokenize(src)) {}

                    SureSpec parse() {
                        while (!at(Token::Kind::End)) {
                            if (atIdent("input"))         parseInput();
                            else if (atIdent("system"))   parseSystem();
                            else if (atIdent("boundary")) parseBoundary();
                            else if (atIdent("output"))   parseOutput();
                            else if (atIdent("tau"))      parseTau();
                            else                          parseParam();
                        }
                        if (indexNames.empty()) fail("no system(...) block found");
                        assemble();
                        return std::move(spec);
                    }

                private:
                    std::vector<Token> toks;
                    std::size_t pos = 0;

                    std::map<std::string, long> params;
                    std::shared_ptr<std::map<std::string, SureInput>> inputs =
                        std::make_shared<std::map<std::string, SureInput>>();

                    std::vector<std::string> indexNames;      // system index vector
                    Domain systemDomain;

                    struct TapDecl { std::string source; std::vector<std::vector<int>> A; std::vector<int> b; };
                    struct EqDecl {
                        std::string name;
                        std::vector<TapDecl> taps;
                        ExprPtr body;
                        ExprPtr boundary;                     // filled from boundary decls; may stay null
                        int line = 0;
                    };
                    std::vector<EqDecl> eqs;
                    SureSpec spec;

                    // ---- token helpers ------------------------------------
                    const Token& cur() const { return toks[pos]; }
                    bool at(Token::Kind k) const { return cur().kind == k; }
                    bool atIdent(const char* s) const { return cur().kind == Token::Kind::Ident && cur().text == s; }
                    bool atPunct(const char* s) const { return cur().kind == Token::Kind::Punct && cur().text == s; }
                    [[noreturn]] void fail(const std::string& msg) const {
                        throw std::runtime_error("sure parser: line " + std::to_string(cur().line) + ": " + msg);
                    }
                    Token next() { Token t = cur(); if (!at(Token::Kind::End)) ++pos; return t; }
                    void expectPunct(const char* s) {
                        if (!atPunct(s)) fail(std::string("expected '") + s + "', got '" + cur().text + "'");
                        ++pos;
                    }
                    std::string expectIdent() {
                        if (!at(Token::Kind::Ident)) fail("expected an identifier, got '" + cur().text + "'");
                        return next().text;
                    }
                    long expectInt() {
                        bool neg = false;
                        if (atPunct("-")) { neg = true; ++pos; }
                        if (!at(Token::Kind::Number)) fail("expected an integer, got '" + cur().text + "'");
                        long v = static_cast<long>(next().number);
                        return neg ? -v : v;
                    }
                    double expectNumber() {
                        bool neg = false;
                        if (atPunct("-")) { neg = true; ++pos; }
                        if (!at(Token::Kind::Number)) fail("expected a number, got '" + cur().text + "'");
                        double v = next().number;
                        return neg ? -v : v;
                    }

                    int indexOf(const std::vector<std::string>& names, const std::string& s) const {
                        for (std::size_t i = 0; i < names.size(); ++i)
                            if (names[i] == s) return static_cast<int>(i);
                        return -1;
                    }

                    // ---- statements ---------------------------------------
                    void parseParam() {
                        std::string name = expectIdent();
                        expectPunct("=");
                        long v = expectInt();
                        expectPunct(";");
                        params[name] = v;
                    }

                    void parseTau() {
                        ++pos;                       // 'tau'
                        expectPunct("=");
                        expectPunct("[");
                        std::vector<int> tau;
                        while (!atPunct("]")) {
                            tau.push_back(static_cast<int>(expectInt()));
                            if (atPunct(",")) ++pos;
                        }
                        expectPunct("]");
                        expectPunct(";");
                        spec.tau = tau;
                    }

                    void parseInput() {
                        ++pos;                       // 'input'
                        std::string name = expectIdent();
                        SureInput arr;
                        long total = 1;
                        while (atPunct("[")) {
                            ++pos;
                            long d = expectInt();
                            if (d <= 0) fail("input dimension must be positive");
                            arr.dims.push_back(d);
                            total *= d;
                            expectPunct("]");
                        }
                        if (arr.dims.empty()) fail("input '" + name + "' needs at least one [dim]");
                        expectPunct("=");
                        if (atPunct("{")) {
                            ++pos;
                            while (!atPunct("}")) {
                                arr.data.push_back(expectNumber());
                                if (atPunct(",")) ++pos;
                            }
                            expectPunct("}");
                            if (static_cast<long>(arr.data.size()) != total)
                                fail("input '" + name + "' has " + std::to_string(arr.data.size()) +
                                     " values, expected " + std::to_string(total));
                        } else {
                            double fill = expectNumber();
                            arr.data.assign(static_cast<std::size_t>(total), fill);
                        }
                        expectPunct(";");
                        (*inputs)[name] = std::move(arr);
                    }

                    void parseSystem() {
                        if (!indexNames.empty()) fail("only one system(...) block is supported");
                        ++pos;                       // 'system'
                        expectPunct("(");
                        systemDomain = parseDomainHeader(indexNames);
                        expectPunct(")");
                        expectPunct("{");
                        while (!atPunct("}")) parseEquation();
                        expectPunct("}");
                    }

                    // parses "(i,j,k) | constraints" given an empty name vector;
                    // returns the Domain (also used by output declarations)
                    Domain parseDomainHeader(std::vector<std::string>& names) {
                        expectPunct("(");
                        while (!atPunct(")")) {
                            names.push_back(expectIdent());
                            if (atPunct(",")) ++pos;
                        }
                        expectPunct(")");
                        if (names.empty()) fail("empty index vector");
                        expectPunct("|");
                        std::vector<LinCon> cons;
                        cons.push_back(parseConstraint(names));
                        while (atPunct(",")) { ++pos; cons.push_back(parseConstraint(names)); }
                        // chains produce multiple constraints; flatten
                        for (auto& extra : pendingChain) cons.push_back(extra);
                        pendingChain.clear();
                        return buildDomain(names, cons);
                    }

                    std::vector<LinCon> pendingChain;    // extra constraints from chained relations

                    // affine REL bare-index-chain REL affine  |  affine REL affine
                    LinCon parseConstraint(const std::vector<std::string>& names) {
                        Affine lhs = parseAffine(names);
                        std::string rel = parseRel();
                        // try a chain: IDENT (',' IDENT)* REL affine
                        std::size_t save = pos;
                        std::vector<int> chainVars;
                        bool chained = false;
                        while (at(Token::Kind::Ident)) {
                            int v = indexOf(names, cur().text);
                            if (v < 0) break;
                            chainVars.push_back(v);
                            ++pos;
                            if (atPunct(",")) { ++pos; continue; }
                            if (atPunct("<") || atPunct("<=") || atPunct(">") || atPunct(">=")) { chained = true; }
                            break;
                        }
                        if (chained && !chainVars.empty()) {
                            std::string rel2 = parseRel();
                            Affine rhs = parseAffine(names);
                            // lhs REL v  for every v, and v REL2 rhs for every v
                            LinCon first = makeCon(lhs, rel, unitVar(names.size(), chainVars[0]));
                            for (std::size_t i = 1; i < chainVars.size(); ++i)
                                pendingChain.push_back(makeCon(lhs, rel, unitVar(names.size(), chainVars[i])));
                            for (int v : chainVars)
                                pendingChain.push_back(makeCon(unitVar(names.size(), v), rel2, rhs));
                            return first;
                        }
                        pos = save;
                        Affine rhs = parseAffine(names);
                        return makeCon(lhs, rel, rhs);
                    }

                    static Affine unitVar(std::size_t dim, int v) {
                        Affine a(dim);
                        a.a[static_cast<std::size_t>(v)] = 1;
                        return a;
                    }

                    std::string parseRel() {
                        if (atPunct("<=") || atPunct("<") || atPunct(">=") || atPunct(">") || atPunct("=="))
                            return next().text;
                        fail("expected a relation (<=, <, >=, >, ==), got '" + cur().text + "'");
                    }

                    // normalize L REL R into  a . x >= rhs  (or ==)
                    LinCon makeCon(const Affine& L, const std::string& rel, const Affine& R) const {
                        Affine diff(L.a.size());
                        const Affine* lo = &L;
                        const Affine* hi = &R;
                        long strict = 0;
                        if (rel == ">" || rel == ">=") { lo = &R; hi = &L; }
                        if (rel == "<" || rel == ">") strict = 1;
                        for (std::size_t i = 0; i < diff.a.size(); ++i) diff.a[i] = hi->a[i] - lo->a[i];
                        diff.b = hi->b - lo->b - strict;
                        // diff.a . x + diff.b >= 0   =>   diff.a . x >= -diff.b
                        LinCon c;
                        c.a = diff.a;
                        c.rhs = -diff.b;
                        c.isEq = (rel == "==");
                        return c;
                    }

                    Domain buildDomain(const std::vector<std::string>& names, const std::vector<LinCon>& cons) {
                        const std::size_t dim = names.size();
                        std::vector<long> lo(dim, std::numeric_limits<long>::min());
                        std::vector<long> hi(dim, std::numeric_limits<long>::max());
                        std::vector<const LinCon*> extra;
                        for (const auto& c : cons) {
                            int nz = -1;
                            bool single = true;
                            for (std::size_t i = 0; i < dim; ++i) {
                                if (c.a[i] != 0) {
                                    if (nz >= 0) { single = false; break; }
                                    nz = static_cast<int>(i);
                                }
                            }
                            if (single && nz >= 0 && !c.isEq &&
                                (c.a[static_cast<std::size_t>(nz)] == 1 || c.a[static_cast<std::size_t>(nz)] == -1)) {
                                std::size_t d = static_cast<std::size_t>(nz);
                                if (c.a[d] == 1) lo[d] = std::max(lo[d], c.rhs);       //  x >= rhs
                                else             hi[d] = std::min(hi[d], -c.rhs);      // -x >= rhs  =>  x <= -rhs
                            } else {
                                extra.push_back(&c);
                            }
                        }
                        Domain dom(static_cast<int>(dim));
                        for (std::size_t d = 0; d < dim; ++d) {
                            if (lo[d] == std::numeric_limits<long>::min() ||
                                hi[d] == std::numeric_limits<long>::max())
                                fail("index '" + names[d] + "' has no constant lower and upper bound");
                            dom.axis(static_cast<int>(d), static_cast<int>(lo[d]), static_cast<int>(hi[d]) + 1);
                        }
                        for (const LinCon* c : extra) {
                            std::vector<int> a(c->a.begin(), c->a.end());
                            dom.add(HalfSpace{ a, static_cast<int>(c->rhs),
                                               c->isEq ? HalfSpace::EQ : HalfSpace::GE });
                        }
                        return dom;
                    }

                    // ---- affine index expressions -------------------------
                    // grammar: aff := term (('+'|'-') term)* ; term := factor ('*' factor)*
                    // factor := INT | param | index | '-' factor | '(' aff ')'
                    Affine parseAffine(const std::vector<std::string>& names) {
                        Affine acc = parseAffineTerm(names);
                        while (atPunct("+") || atPunct("-")) {
                            bool minus = next().text == "-";
                            Affine t = parseAffineTerm(names);
                            for (std::size_t i = 0; i < acc.a.size(); ++i)
                                acc.a[i] += minus ? -t.a[i] : t.a[i];
                            acc.b += minus ? -t.b : t.b;
                        }
                        return acc;
                    }
                    Affine parseAffineTerm(const std::vector<std::string>& names) {
                        Affine acc = parseAffineFactor(names);
                        while (atPunct("*")) {
                            ++pos;
                            Affine f = parseAffineFactor(names);
                            if (!acc.isConst() && !f.isConst())
                                fail("non-affine index expression (product of two index terms)");
                            const Affine& cst = acc.isConst() ? acc : f;
                            const Affine& lin = acc.isConst() ? f : acc;
                            Affine r(acc.a.size());
                            for (std::size_t i = 0; i < r.a.size(); ++i) r.a[i] = lin.a[i] * cst.b;
                            r.b = lin.b * cst.b;
                            acc = r;
                        }
                        return acc;
                    }
                    Affine parseAffineFactor(const std::vector<std::string>& names) {
                        if (atPunct("-")) {
                            ++pos;
                            Affine f = parseAffineFactor(names);
                            for (auto& c : f.a) c = -c;
                            f.b = -f.b;
                            return f;
                        }
                        if (atPunct("(")) {
                            ++pos;
                            Affine f = parseAffine(names);
                            expectPunct(")");
                            return f;
                        }
                        if (at(Token::Kind::Number)) {
                            Affine f(names.size());
                            f.b = static_cast<long>(next().number);
                            return f;
                        }
                        if (at(Token::Kind::Ident)) {
                            std::string id = next().text;
                            int v = indexOf(names, id);
                            if (v >= 0) return unitVar(names.size(), v);
                            auto it = params.find(id);
                            if (it != params.end()) {
                                Affine f(names.size());
                                f.b = it->second;
                                return f;
                            }
                            fail("unknown identifier '" + id + "' in an index expression");
                        }
                        fail("expected an index expression, got '" + cur().text + "'");
                    }

                    // ---- equations ----------------------------------------
                    void parseEquation() {
                        int line = cur().line;
                        std::string name = expectIdent();
                        expectPunct("(");
                        for (std::size_t i = 0; i < indexNames.size(); ++i) {
                            std::string id = expectIdent();
                            if (id != indexNames[i])
                                fail("equation LHS indices must match the system index vector (" + id + ")");
                            if (i + 1 < indexNames.size()) expectPunct(",");
                        }
                        expectPunct(")");
                        expectPunct("=");
                        EqDecl eq;
                        eq.name = name;
                        eq.line = line;
                        eq.body = parseExpr(eq);
                        expectPunct(";");
                        eqs.push_back(std::move(eq));
                    }

                    // full arithmetic expression; taps registered into eq
                    ExprPtr parseExpr(EqDecl& eq) {
                        ExprPtr lhs = parseTerm(eq);
                        while (atPunct("+") || atPunct("-")) {
                            char op = next().text[0];
                            ExprPtr rhs = parseTerm(eq);
                            auto n = std::make_shared<Expr>();
                            n->kind = Expr::Kind::Binary; n->op = op; n->lhs = lhs; n->rhs = rhs;
                            lhs = n;
                        }
                        return lhs;
                    }
                    ExprPtr parseTerm(EqDecl& eq) {
                        ExprPtr lhs = parseFactor(eq);
                        while (atPunct("*") || atPunct("/")) {
                            char op = next().text[0];
                            ExprPtr rhs = parseFactor(eq);
                            auto n = std::make_shared<Expr>();
                            n->kind = Expr::Kind::Binary; n->op = op; n->lhs = lhs; n->rhs = rhs;
                            lhs = n;
                        }
                        return lhs;
                    }
                    ExprPtr parseFactor(EqDecl& eq) {
                        if (atPunct("-")) {
                            ++pos;
                            auto n = std::make_shared<Expr>();
                            n->kind = Expr::Kind::Unary; n->op = '-'; n->lhs = parseFactor(eq);
                            return n;
                        }
                        if (atPunct("(")) {
                            ++pos;
                            ExprPtr e = parseExpr(eq);
                            expectPunct(")");
                            return e;
                        }
                        if (at(Token::Kind::Number)) {
                            auto n = std::make_shared<Expr>();
                            n->kind = Expr::Kind::Number; n->number = next().number;
                            return n;
                        }
                        if (at(Token::Kind::Ident)) {
                            std::string id = next().text;
                            if ((id == "sqrt" || id == "exp" || id == "abs") && atPunct("(")) {
                                ++pos;
                                auto n = std::make_shared<Expr>();
                                n->kind = Expr::Kind::Call; n->name = id; n->lhs = parseExpr(eq);
                                expectPunct(")");
                                return n;
                            }
                            if (atPunct("(")) return parseTapRef(eq, id);
                            if (atPunct("[")) return parseArrayRef(id);
                            auto it = params.find(id);
                            if (it != params.end()) {
                                auto n = std::make_shared<Expr>();
                                n->kind = Expr::Kind::Number; n->number = static_cast<double>(it->second);
                                return n;
                            }
                            fail("unknown identifier '" + id + "' (bare index variables are not allowed in bodies)");
                        }
                        fail("expected an expression, got '" + cur().text + "'");
                    }

                    ExprPtr parseTapRef(EqDecl& eq, const std::string& source) {
                        expectPunct("(");
                        std::vector<std::vector<int>> A;
                        std::vector<int> b;
                        while (!atPunct(")")) {
                            Affine aff = parseAffine(indexNames);
                            std::vector<int> row(aff.a.begin(), aff.a.end());
                            A.push_back(row);
                            b.push_back(static_cast<int>(aff.b));
                            if (atPunct(",")) ++pos;
                        }
                        expectPunct(")");
                        if (A.size() != indexNames.size())
                            fail("tap '" + source + "' must have " + std::to_string(indexNames.size()) + " indices");
                        // dedupe identical taps to one slot
                        for (std::size_t s = 0; s < eq.taps.size(); ++s) {
                            if (eq.taps[s].source == source && eq.taps[s].A == A && eq.taps[s].b == b) {
                                auto n = std::make_shared<Expr>();
                                n->kind = Expr::Kind::TapRef; n->tapSlot = s;
                                return n;
                            }
                        }
                        eq.taps.push_back({ source, A, b });
                        auto n = std::make_shared<Expr>();
                        n->kind = Expr::Kind::TapRef; n->tapSlot = eq.taps.size() - 1;
                        return n;
                    }

                    ExprPtr parseArrayRef(const std::string& name) {
                        auto n = std::make_shared<Expr>();
                        n->kind = Expr::Kind::ArrayRef; n->name = name;
                        while (atPunct("[")) {
                            ++pos;
                            Affine aff = parseAffine(indexNames);
                            n->idxCoeffs.push_back(std::vector<long>(aff.a.begin(), aff.a.end()));
                            n->idxConst.push_back(aff.b);
                            expectPunct("]");
                        }
                        auto it = inputs->find(name);
                        if (it == inputs->end()) fail("unknown input array '" + name + "'");
                        if (n->idxCoeffs.size() != it->second.dims.size())
                            fail("input '" + name + "' has " + std::to_string(it->second.dims.size()) + " dimensions");
                        return n;
                    }

                    // ---- boundary / output --------------------------------
                    void parseBoundary() {
                        ++pos;                       // 'boundary'
                        if (indexNames.empty()) fail("boundary declared before the system(...) block");
                        std::string name = expectIdent();
                        expectPunct("(");
                        for (std::size_t i = 0; i < indexNames.size(); ++i) {
                            std::string id = expectIdent();
                            if (id != indexNames[i]) fail("boundary indices must match the system index vector");
                            if (i + 1 < indexNames.size()) expectPunct(",");
                        }
                        expectPunct(")");
                        expectPunct("=");
                        EqDecl scratch;             // boundary bodies may not contain taps
                        ExprPtr body = parseExpr(scratch);
                        if (!scratch.taps.empty()) fail("boundary expressions may not read recurrence variables");
                        expectPunct(";");
                        for (auto& eq : eqs) {
                            if (eq.name == name) { eq.boundary = body; return; }
                        }
                        fail("boundary for unknown recurrence variable '" + name + "'");
                    }

                    void parseOutput() {
                        ++pos;                       // 'output'
                        if (indexNames.empty()) fail("output declared before the system(...) block");
                        SureOutput out;
                        expectPunct("(");
                        out.domain = parseDomainHeader(out.indexNames);
                        expectPunct(")");
                        out.var = expectIdent();
                        bool known = false;
                        for (const auto& eq : eqs) known |= (eq.name == out.var);
                        if (!known) fail("output references unknown recurrence variable '" + out.var + "'");
                        expectPunct("(");
                        std::vector<std::vector<int>> A;
                        std::vector<int> b;
                        while (!atPunct(")")) {
                            Affine aff = parseAffine(out.indexNames);
                            A.push_back(std::vector<int>(aff.a.begin(), aff.a.end()));
                            b.push_back(static_cast<int>(aff.b));
                            if (atPunct(",")) ++pos;
                        }
                        expectPunct(")");
                        expectPunct(";");
                        if (A.size() != indexNames.size())
                            fail("output read must have " + std::to_string(indexNames.size()) + " indices");
                        out.map = AffineDependency::map(std::move(A), std::move(b));
                        spec.outputs.push_back(std::move(out));
                    }

                    // ---- assembly -----------------------------------------
                    void assemble() {
                        spec.indexNames = indexNames;
                        auto ins = inputs;           // shared with all boundary closures
                        for (auto& eq : eqs) {
                            std::vector<Equation<double>::Tap> taps;
                            for (const auto& t : eq.taps)
                                taps.push_back({ t.source, AffineDependency::map(t.A, t.b) });
                            ExprPtr body = eq.body;
                            ExprPtr bnd = eq.boundary;
                            spec.system.add(Equation<double>{
                                eq.name, systemDomain, std::move(taps),
                                [body](const std::vector<double>& t, const IndexPoint&) {
                                    return suredetail::evalExpr(*body, &t, nullptr, nullptr);
                                },
                                [bnd, ins](const IndexPoint& p) {
                                    return bnd ? suredetail::evalExpr(*bnd, nullptr, &p, ins.get()) : 0.0;
                                } });
                        }
                        if (spec.outputs.empty()) fail("no output(...) declaration found");
                        if (!spec.tau.empty() && spec.tau.size() != indexNames.size())
                            fail("tau has " + std::to_string(spec.tau.size()) + " components, expected " +
                                 std::to_string(indexNames.size()));
                    }
                };

            } // namespace suredetail

            inline SureSpec parseSureString(const std::string& src) {
                return suredetail::Parser(src).parse();
            }

            inline SureSpec parseSure(std::istream& is) {
                std::ostringstream ss;
                ss << is.rdbuf();
                return parseSureString(ss.str());
            }

            inline SureSpec parseSureFile(const std::string& path) {
                std::ifstream ifs(path);
                if (!ifs) throw std::runtime_error("sure parser: cannot open '" + path + "'");
                return parseSure(ifs);
            }

        }
    }
}
