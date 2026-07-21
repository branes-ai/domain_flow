#pragma once
// Text front-end for the SURE simulator: parse the recurrence-system DSL used
// throughout docs/SURE/ into an executable RecurrenceSystem (issues #15, #17).
//
// v2 grammar (see docs/SURE/matmul.sure for a worked example): data enters and
// leaves the domain of computation through symmetric input/output confluence
// declarations -- a tensor associated with an oriented face of the domain.
//
//   M = 2; K = 3;                              // integer parameters
//   system ((i,j,k) | 0 <= i < M, ...) {       // shared iteration domain
//       a(i,j,k) = a(i,j-1,k);                 // equations: taps are var(affine...)
//       c(i,j,k) = c(i,j,k-1) + a(i,j-1,k) * b(i-1,j,k);
//   }
//   input  A[M][K] ((i,j,k) | 0 <= i < M, 0 <= k < K, j = -1)  : a(i,j,k) = A[i][k];
//   input          ((i,j,k) | 0 <= i < M, 0 <= j < N, k = -1)  : c(i,j,k) = 0;
//   output C[M][N] ((i,j,k) | 0 <= i < M, 0 <= j < N, k = K-1) : C[i][j] = c(i,j,k);
//   tau = [1, 1, 1];                           // optional canonical linear schedule
//   data A = { 1, 2, 3, 4, 5, 6 };             // test-bench binding (row-major, or scalar)
//
// A face region is written in the domain's own coordinates: extent from the
// inequalities, location from exactly one equality, and orientation *derived*
// (the equality plane's outward normal w.r.t. the domain; inputs are influx,
// outputs are outflux).  Input faces sit on the one-step halo where boundary
// values are actually read (j = -1); output faces on the domain (k = K-1).
//
// Validated contracts:
//   - coverage: every out-of-domain tap image lands on exactly one input face
//     of its variable (uncovered accesses are hard, located errors)
//   - face well-formedness: exactly one equality per face; the face plane may
//     not cut the domain interior; every face index has constant bounds
//   - flux consistency: with a declared tau, tau.n < 0 on every input face
//     and tau.n > 0 on every output face (n = outward normal)
//   - element range: output tensor indices stay within the declared dims
//
// Notation rules: recurrence variables are read with parentheses (a(i,j-1,k),
// affine dependency taps; equation and output expressions), tensors with
// brackets (A[i][k]; input and output expressions only).  Expression bodies
// support + - * /, unary minus, parentheses, the pointwise maps sqrt/exp/abs,
// and the exact selection built-ins gt(a,b) (1 if a > b else 0) and
// select(c,x,y) (x if c != 0 else y) -- e.g. an argmax reduction (iamax).
//
// v1 -> v2: the `boundary` statement, the data-table `input`, and the
// projected `output` are replaced by the confluence declarations above.
// Scope: all equations share the system domain (per-equation domains, as QR
// needs, remain a follow-up).
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <cmath>
#include <cctype>
#include <limits>
#include <stdexcept>
#include <dfa/sim/sure_simulator.hpp>

namespace sw {
    namespace dfa {
        namespace sim {

            // named tensor data binding (row-major)
            struct SureInput {
                std::vector<long> dims;
                std::vector<double> data;
            };

            // input confluence summary (for reporting)
            struct SureInputInfo {
                std::string tensor;         // "" for a constant input
                std::string var;            // recurrence variable it feeds
                std::vector<int> normal;    // outward face normal
            };

            // one output confluence: enumerate `region` (a face of the domain);
            // each face point maps to a tensor element and a value
            struct SureOutput {
                std::string tensor;
                std::vector<long> dims;
                std::string var;                       // primary recurrence variable read
                Domain region;
                std::vector<int> normal;               // outward face normal
                std::function<std::vector<long>(const IndexPoint&)> elemIndex;
                std::function<double(SureSimulator<double>&, const IndexPoint&)> eval;
            };

            // a parsed, executable SURE program
            struct SureSpec {
                RecurrenceSystem<double> system;
                std::vector<std::string> indexNames;   // system index names, for reporting
                std::vector<SureInputInfo> inputs;
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
                    std::string name;                           // ArrayRef tensor / Call function
                    std::vector<std::vector<long>> idxCoeffs;   // ArrayRef: per-dim coeffs over system indices
                    std::vector<long> idxConst;                 // ArrayRef: per-dim constants
                    char op = 0;                                // Unary '-', Binary '+','-','*','/'
                    ExprPtr lhs, rhs;                           // Binary; Unary uses lhs
                    std::vector<ExprPtr> args;                  // Call arguments (1: sqrt/exp/abs; 2: gt; 3: select)
                };

                inline double evalExpr(const Expr& e,
                                       const std::vector<double>* taps,
                                       const IndexPoint* p,
                                       const std::map<std::string, SureInput>* inputs) {
                    switch (e.kind) {
                    case Expr::Kind::Number: return e.number;
                    case Expr::Kind::TapRef:
                        if (!taps) throw std::runtime_error("sure: tap reference in an input expression");
                        return (*taps)[e.tapSlot];
                    case Expr::Kind::ArrayRef: {
                        if (!p || !inputs) throw std::runtime_error("sure: tensor reference in an equation body");
                        auto it = inputs->find(e.name);
                        if (it == inputs->end())
                            throw std::runtime_error("sure: no data bound for tensor '" + e.name + "'");
                        const SureInput& arr = it->second;
                        long flat = 0;
                        for (std::size_t d = 0; d < arr.dims.size(); ++d) {
                            long v = e.idxConst[d];
                            for (std::size_t i = 0; i < e.idxCoeffs[d].size() && i < p->size(); ++i)
                                v += e.idxCoeffs[d][i] * (*p)[i];
                            if (v < 0 || v >= arr.dims[d])
                                throw std::runtime_error("sure: index out of range reading tensor '" + e.name + "'");
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
                        default:
                            if (b == 0.0) throw std::runtime_error("sure: division by zero in an expression");
                            return a / b;
                        }
                    }
                    case Expr::Kind::Call: {
                        double a = evalExpr(*e.args[0], taps, p, inputs);
                        if (e.name == "sqrt") return std::sqrt(a);
                        if (e.name == "exp")  return std::exp(a);
                        if (e.name == "abs")  return std::abs(a);
                        // two/three-argument selection built-ins (exact; ties well-defined)
                        double b = evalExpr(*e.args[1], taps, p, inputs);
                        if (e.name == "gt") return (a > b) ? 1.0 : 0.0;   // strict greater-than indicator
                        return (a != 0.0) ? b : evalExpr(*e.args[2], taps, p, inputs);   // "select"(c,x,y)
                    }
                    }
                    return 0.0;
                }

                inline void collectArrays(const Expr& e, std::vector<std::string>& names) {
                    if (e.kind == Expr::Kind::ArrayRef) names.push_back(e.name);
                    if (e.lhs) collectArrays(*e.lhs, names);
                    if (e.rhs) collectArrays(*e.rhs, names);
                    for (const auto& a : e.args) if (a) collectArrays(*a, names);
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
                            if (atIdent("input"))       parseInputFace();
                            else if (atIdent("output")) parseOutputFace();
                            else if (atIdent("data"))   parseData();
                            else if (atIdent("system")) parseSystem();
                            else if (atIdent("tau"))    parseTau();
                            else                        parseParam();
                        }
                        if (indexNames.empty()) fail("no system(...) block found");
                        assemble();
                        return std::move(spec);
                    }

                private:
                    std::vector<Token> toks;
                    std::size_t pos = 0;

                    std::map<std::string, long> params;
                    std::map<std::string, std::vector<long>> tensors;   // declared tensor dims
                    std::shared_ptr<std::map<std::string, SureInput>> data =
                        std::make_shared<std::map<std::string, SureInput>>();

                    std::vector<std::string> indexNames;      // system index vector
                    Domain systemDomain;
                    std::vector<LinCon> sysCons;              // the system-header constraints (for per-equation domains)
                    std::vector<long> sysLo, sysHi;           // domain bounding box
                    std::vector<IndexPoint> sysPoints;        // enumerated domain, for orientation + coverage

                    struct TapDecl { std::string source; std::vector<std::vector<int>> A; std::vector<int> b; };
                    struct EqDecl {
                        std::string name;
                        std::vector<TapDecl> taps;
                        ExprPtr body;
                        std::vector<LinCon> cons;             // optional per-equation domain restriction (e.g. j <= i)
                        int line = 0;
                    };
                    struct InFace {
                        std::string tensor;                   // "" for a constant input
                        std::string var;
                        Domain region;
                        std::vector<int> normal;
                        ExprPtr expr;
                        int line = 0;
                    };
                    struct OutFace {
                        std::string tensor;
                        std::vector<long> dims;
                        Domain region;
                        std::vector<int> normal;
                        std::vector<std::vector<long>> elemA; // tensor index affines over system indices
                        std::vector<long> elemB;
                        ExprPtr expr;
                        std::vector<TapDecl> taps;
                        int line = 0;
                    };
                    std::vector<EqDecl> eqs;
                    std::vector<InFace> inFaces;
                    std::vector<OutFace> outFaces;
                    SureSpec spec;
                    bool inFaceExpr = false;   // tensors are legal only in input/output expressions
                    const std::vector<std::string> kNoNames{};

                    // ---- token helpers ------------------------------------
                    const Token& cur() const { return toks[pos]; }
                    bool at(Token::Kind k) const { return cur().kind == k; }
                    bool atIdent(const char* s) const { return cur().kind == Token::Kind::Ident && cur().text == s; }
                    bool atPunct(const char* s) const { return cur().kind == Token::Kind::Punct && cur().text == s; }
                    [[noreturn]] void failAt(int line, const std::string& msg) const {
                        throw std::runtime_error("sure parser: line " + std::to_string(line) + ": " + msg);
                    }
                    [[noreturn]] void fail(const std::string& msg) const { failAt(cur().line, msg); }
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

                    std::string pointToString(const IndexPoint& p) const {
                        std::ostringstream os;
                        os << "(";
                        for (std::size_t i = 0; i < p.size(); ++i) os << p[i] << (i + 1 < p.size() ? "," : "");
                        os << ")";
                        return os.str();
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

                    // data NAME = { csv } ;   |   data NAME = number ;
                    void parseData() {
                        ++pos;                       // 'data'
                        int line = cur().line;
                        std::string name = expectIdent();
                        auto it = tensors.find(name);
                        if (it == tensors.end()) failAt(line, "data binding for undeclared tensor '" + name + "'");
                        long total = 1;
                        for (long d : it->second) total *= d;
                        expectPunct("=");
                        SureInput arr;
                        arr.dims = it->second;
                        if (atPunct("{")) {
                            ++pos;
                            while (!atPunct("}")) {
                                arr.data.push_back(expectNumber());
                                if (atPunct(",")) ++pos;
                            }
                            expectPunct("}");
                            if (static_cast<long>(arr.data.size()) != total)
                                failAt(line, "tensor '" + name + "' has " + std::to_string(arr.data.size()) +
                                             " values, expected " + std::to_string(total));
                        } else {
                            double fill = expectNumber();
                            arr.data.assign(static_cast<std::size_t>(total), fill);
                        }
                        expectPunct(";");
                        (*data)[name] = std::move(arr);
                    }

                    // TENSOR '[' constAffine ']'+  -- dims may use parameters
                    std::vector<long> parseTensorDims() {
                        std::vector<long> dims;
                        while (atPunct("[")) {
                            ++pos;
                            Affine a = parseAffine(indexNames.empty() ? kNoNames : indexNames);
                            if (!a.isConst() || a.b <= 0) fail("tensor dimensions must be positive constants");
                            dims.push_back(a.b);
                            expectPunct("]");
                        }
                        if (dims.empty()) fail("tensor needs at least one [dim]");
                        return dims;
                    }

                    void parseSystem() {
                        if (!indexNames.empty()) fail("only one system(...) block is supported");
                        ++pos;                       // 'system'
                        expectPunct("(");
                        parseIndexHeader(indexNames);
                        std::vector<LinCon> cons = parseConstraints(indexNames);
                        expectPunct(")");
                        sysCons = cons;                                   // kept for per-equation domain restrictions
                        systemDomain = buildDomain(indexNames, cons, sysLo, sysHi);
                        // the enumerated (constraint-filtered) domain: used to orient
                        // face planes as supporting hyperplanes and for coverage checks
                        sysPoints = systemDomain.enumerate();
                        if (sysPoints.empty()) fail("system domain is empty");
                        expectPunct("{");
                        while (!atPunct("}")) parseEquation();
                        expectPunct("}");
                    }

                    void parseIndexHeader(std::vector<std::string>& names) {
                        expectPunct("(");
                        while (!atPunct(")")) {
                            names.push_back(expectIdent());
                            if (atPunct(",")) ++pos;
                        }
                        expectPunct(")");
                        if (names.empty()) fail("empty index vector");
                        expectPunct("|");
                    }

                    std::vector<LinCon> parseConstraints(const std::vector<std::string>& names) {
                        std::vector<LinCon> cons;
                        cons.push_back(parseConstraint(names));
                        while (atPunct(",")) { ++pos; cons.push_back(parseConstraint(names)); }
                        for (auto& extra : pendingChain) cons.push_back(extra);
                        pendingChain.clear();
                        return cons;
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
                        if (atPunct("<=") || atPunct("<") || atPunct(">=") || atPunct(">") ||
                            atPunct("==") || atPunct("="))
                            return next().text;
                        fail("expected a relation (<=, <, >=, >, =, ==), got '" + cur().text + "'");
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
                        c.isEq = (rel == "==" || rel == "=");
                        return c;
                    }

                    Domain buildDomain(const std::vector<std::string>& names, const std::vector<LinCon>& cons,
                                       std::vector<long>& loOut, std::vector<long>& hiOut) {
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
                            if (single && nz >= 0 &&
                                (c.a[static_cast<std::size_t>(nz)] == 1 || c.a[static_cast<std::size_t>(nz)] == -1)) {
                                std::size_t d = static_cast<std::size_t>(nz);
                                if (c.isEq) {                                     //  +-x == rhs: pins the box
                                    long v = (c.a[d] == 1) ? c.rhs : -c.rhs;
                                    lo[d] = std::max(lo[d], v);
                                    hi[d] = std::min(hi[d], v);
                                } else if (c.a[d] == 1) lo[d] = std::max(lo[d], c.rhs);   //  x >= rhs
                                else                    hi[d] = std::min(hi[d], -c.rhs);  //  x <= -rhs
                            } else {
                                extra.push_back(&c);
                            }
                        }
                        Domain dom(static_cast<int>(dim));
                        for (std::size_t d = 0; d < dim; ++d) {
                            if (lo[d] == std::numeric_limits<long>::min() ||
                                hi[d] == std::numeric_limits<long>::max())
                                fail("index '" + names[d] + "' has no constant lower and upper bound");
                            if (lo[d] > hi[d]) fail("index '" + names[d] + "' has an empty range");
                            dom.axis(static_cast<int>(d), static_cast<int>(lo[d]), static_cast<int>(hi[d]) + 1);
                        }
                        for (const LinCon* c : extra) {
                            std::vector<int> a(c->a.begin(), c->a.end());
                            dom.add(HalfSpace{ a, static_cast<int>(c->rhs),
                                               c->isEq ? HalfSpace::EQ : HalfSpace::GE });
                        }
                        loOut = lo;
                        hiOut = hi;
                        return dom;
                    }

                    // face region: "((i,j,k) | extent..., one equality)" in system coordinates;
                    // returns the Domain and derives the outward face normal
                    Domain parseFaceRegion(std::vector<int>& normal, int line) {
                        expectPunct("(");
                        std::vector<std::string> names;
                        parseIndexHeader(names);
                        if (names != indexNames)
                            failAt(line, "face regions use the system index vector");
                        std::vector<LinCon> cons = parseConstraints(names);
                        expectPunct(")");

                        // exactly one equality pins the face plane (codimension 1)
                        const LinCon* eq = nullptr;
                        for (const auto& c : cons) {
                            if (c.isEq) {
                                if (eq) failAt(line, "a face region needs exactly one equality (found several)");
                                eq = &c;
                            }
                        }
                        if (!eq) failAt(line, "a face region needs exactly one equality to pin the face plane");

                        // the equality must have index terms (a constant equality like
                        // 0 = 1 has no plane to pin)
                        bool nonzero = false;
                        for (long a : eq->a) nonzero |= (a != 0);
                        if (!nonzero) failAt(line, "the face equality has no index terms");

                        // orientation: the plane a.x = rhs must be a supporting
                        // hyperplane of the domain -- classify every domain point and
                        // reject planes with points on both strict sides; the outward
                        // normal points away from the occupied side
                        bool hasNeg = false, hasPos = false;
                        for (const auto& p : sysPoints) {
                            long side = -eq->rhs;
                            for (std::size_t d = 0; d < indexNames.size(); ++d)
                                side += eq->a[d] * p[d];
                            hasNeg |= (side < 0);
                            hasPos |= (side > 0);
                        }
                        if (hasNeg && hasPos)
                            failAt(line, "the face plane cuts the domain interior; a face must lie on or outside the domain surface");
                        if (!hasNeg && !hasPos)
                            failAt(line, "face orientation is ambiguous: the entire domain lies on the face plane");
                        normal.clear();
                        for (std::size_t d = 0; d < indexNames.size(); ++d) {
                            long a = eq->a[d];
                            normal.push_back(static_cast<int>(hasPos ? -a : a));
                        }

                        std::vector<long> lo, hi;
                        return buildDomain(names, cons, lo, hi);
                    }

                    // ---- affine index expressions -------------------------
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
                        EqDecl eq;
                        eq.name = name;
                        eq.line = line;
                        expectSystemIndexList(eq);        // parses (i,j,k) and an optional | domain restriction
                        expectPunct("=");
                        eq.body = parseExpr(eq);
                        expectPunct(";");
                        eqs.push_back(std::move(eq));
                    }

                    // The equation's LHS index list, optionally restricted to a per-equation SUB-DOMAIN of the
                    // system domain: "name(i,j,k | j <= i) = ...". The restriction is intersected with the system
                    // domain, so the equation is computed only where it holds (e.g. a lower-triangular reduction).
                    void expectSystemIndexList(EqDecl& eq) {
                        expectPunct("(");
                        for (std::size_t i = 0; i < indexNames.size(); ++i) {
                            std::string id = expectIdent();
                            if (id != indexNames[i])
                                fail("indices must match the system index vector ('" + id + "')");
                            if (i + 1 < indexNames.size()) expectPunct(",");
                        }
                        if (atPunct("|")) { ++pos; eq.cons = parseConstraints(indexNames); }
                        expectPunct(")");
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
                            // built-in calls: 1-arg maps (sqrt/exp/abs), and the
                            // exact selection builtins gt(a,b) and select(c,x,y)
                            std::size_t arity = (id == "sqrt" || id == "exp" || id == "abs") ? 1
                                              : (id == "gt")     ? 2
                                              : (id == "select") ? 3 : 0;
                            if (arity != 0 && atPunct("(")) {
                                ++pos;
                                auto n = std::make_shared<Expr>();
                                n->kind = Expr::Kind::Call; n->name = id;
                                n->args.push_back(parseExpr(eq));
                                for (std::size_t k = 1; k < arity; ++k) {
                                    expectPunct(",");
                                    n->args.push_back(parseExpr(eq));
                                }
                                expectPunct(")");
                                return n;
                            }
                            if (atPunct("(")) return parseTapRef(eq, id);
                            if (atPunct("[")) {
                                if (!inFaceExpr)
                                    fail("tensors may only be read in input/output expressions ('" + id + "')");
                                return parseArrayRef(id);
                            }
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
                        auto it = tensors.find(name);
                        if (it == tensors.end()) fail("unknown tensor '" + name + "'");
                        if (n->idxCoeffs.size() != it->second.size())
                            fail("tensor '" + name + "' has " + std::to_string(it->second.size()) + " dimensions");
                        return n;
                    }

                    // ---- confluences --------------------------------------
                    // input [TENSOR[dims]] ((idx)|cons) : var(idx) = expr ;
                    void parseInputFace() {
                        int line = cur().line;
                        ++pos;                       // 'input'
                        if (indexNames.empty()) failAt(line, "input declared before the system(...) block");
                        InFace face;
                        face.line = line;
                        if (at(Token::Kind::Ident)) {
                            face.tensor = expectIdent();
                            std::vector<long> dims = parseTensorDims();
                            if (tensors.count(face.tensor)) failAt(line, "tensor '" + face.tensor + "' declared twice");
                            tensors[face.tensor] = dims;
                        }
                        face.region = parseFaceRegion(face.normal, line);
                        expectPunct(":");
                        face.var = expectIdent();
                        bool known = false;
                        for (const auto& eq : eqs) known |= (eq.name == face.var);
                        if (!known) failAt(line, "input for unknown recurrence variable '" + face.var + "'");
                        EqDecl scratch;             // input expressions may not contain taps (or a domain restriction)
                        expectSystemIndexList(scratch);
                        if (!scratch.cons.empty()) failAt(line, "an input face's LHS may not carry a domain restriction");
                        expectPunct("=");
                        inFaceExpr = true;
                        face.expr = parseExpr(scratch);
                        inFaceExpr = false;
                        if (!scratch.taps.empty())
                            failAt(line, "input expressions may not read recurrence variables");
                        // bind the metadata to the tensor actually read: a face that
                        // declares a tensor must read exactly that tensor; a tensorless
                        // face may read previously declared tensors (a flow variable can
                        // be seeded from several faces of one tensor, e.g. conv2d's
                        // image walk) and its metadata reflects what it reads
                        std::vector<std::string> reads;
                        collectArrays(*face.expr, reads);
                        if (face.tensor.empty()) {
                            for (const auto& r : reads) {
                                if (face.tensor.empty()) face.tensor = r;
                                else if (r != face.tensor)
                                    failAt(line, "an input face may read only one tensor (reads '" +
                                                 face.tensor + "' and '" + r + "')");
                            }
                        } else {
                            if (reads.empty())
                                failAt(line, "input expression must read its declared tensor '" + face.tensor + "'");
                            for (const auto& r : reads)
                                if (r != face.tensor)
                                    failAt(line, "input expression may only read its declared tensor '" +
                                                 face.tensor + "' (reads '" + r + "')");
                        }
                        expectPunct(";");
                        inFaces.push_back(std::move(face));
                    }

                    // output TENSOR[dims] ((idx)|cons) : TENSOR[aff]... = expr ;
                    void parseOutputFace() {
                        int line = cur().line;
                        ++pos;                       // 'output'
                        if (indexNames.empty()) failAt(line, "output declared before the system(...) block");
                        OutFace face;
                        face.line = line;
                        face.tensor = expectIdent();
                        face.dims = parseTensorDims();
                        if (tensors.count(face.tensor)) failAt(line, "tensor '" + face.tensor + "' declared twice");
                        tensors[face.tensor] = face.dims;
                        face.region = parseFaceRegion(face.normal, line);
                        expectPunct(":");
                        std::string lhs = expectIdent();
                        if (lhs != face.tensor)
                            failAt(line, "output element must be written to tensor '" + face.tensor + "'");
                        while (atPunct("[")) {
                            ++pos;
                            Affine aff = parseAffine(indexNames);
                            face.elemA.push_back(std::vector<long>(aff.a.begin(), aff.a.end()));
                            face.elemB.push_back(aff.b);
                            expectPunct("]");
                        }
                        if (face.elemA.size() != face.dims.size())
                            failAt(line, "output tensor '" + face.tensor + "' has " +
                                         std::to_string(face.dims.size()) + " dimensions");
                        expectPunct("=");
                        EqDecl scratch;
                        inFaceExpr = true;
                        face.expr = parseExpr(scratch);
                        inFaceExpr = false;
                        face.taps = std::move(scratch.taps);
                        if (face.taps.empty())
                            failAt(line, "output expressions must read at least one recurrence variable");
                        expectPunct(";");
                        outFaces.push_back(std::move(face));
                    }

                    // ---- assembly + validation ----------------------------
                    void assemble() {
                        spec.indexNames = indexNames;
                        if (outFaces.empty()) fail("no output(...) declaration found");

                        // every tap must read a declared recurrence variable; without
                        // this, an in-domain tap to a typo would only fail at eval time
                        auto isVar = [this](const std::string& n) {
                            for (const auto& e : eqs) if (e.name == n) return true;
                            return false;
                        };
                        for (const auto& eq : eqs)
                            for (const auto& t : eq.taps)
                                if (!isVar(t.source))
                                    failAt(eq.line, "equation for '" + eq.name +
                                                    "' reads unknown recurrence variable '" + t.source + "'");
                        for (const auto& f : outFaces)
                            for (const auto& t : f.taps)
                                if (!isVar(t.source))
                                    failAt(f.line, "output '" + f.tensor +
                                                   "' reads unknown recurrence variable '" + t.source + "'");
                        if (!spec.tau.empty() && spec.tau.size() != indexNames.size())
                            fail("tau has " + std::to_string(spec.tau.size()) + " components, expected " +
                                 std::to_string(indexNames.size()));

                        // every tensor referenced in a face expression needs a data binding
                        for (const auto& f : inFaces) requireData(*f.expr, f.line);
                        for (const auto& f : outFaces) requireData(*f.expr, f.line);

                        // flux consistency: tau.n < 0 on input faces, tau.n > 0 on output faces
                        if (!spec.tau.empty()) {
                            for (const auto& f : inFaces) {
                                if (flux(f.normal) >= 0)
                                    failAt(f.line, "input face of '" + f.var +
                                                   "' has non-negative flux (tau.n >= 0): data must flow into the wavefront sweep");
                            }
                            for (const auto& f : outFaces) {
                                if (flux(f.normal) <= 0)
                                    failAt(f.line, "output face of '" + f.tensor +
                                                   "' has non-positive flux (tau.n <= 0): results must exit ahead of the sweep");
                            }
                        }

                        // build equations with face-dispatched boundaries
                        auto ins = data;
                        for (auto& eq : eqs) {
                            std::vector<Equation<double>::Tap> taps;
                            for (const auto& t : eq.taps)
                                taps.push_back({ t.source, AffineDependency::map(t.A, t.b) });
                            std::vector<std::pair<Domain, ExprPtr>> faces;
                            for (const auto& f : inFaces)
                                if (f.var == eq.name) faces.push_back({ f.region, f.expr });
                            ExprPtr body = eq.body;
                            std::string name = eq.name;
                            // per-equation domain: the system domain intersected with any LHS restriction
                            // (e.g. j <= i). Points outside are boundaries, resolved via input faces like any tap.
                            Domain eqDomain = systemDomain;
                            if (!eq.cons.empty()) {
                                std::vector<LinCon> combined = sysCons;
                                combined.insert(combined.end(), eq.cons.begin(), eq.cons.end());
                                std::vector<long> lo, hi;
                                eqDomain = buildDomain(indexNames, combined, lo, hi);
                            }
                            spec.system.add(Equation<double>{
                                eq.name, eqDomain, std::move(taps),
                                [body](const std::vector<double>& t, const IndexPoint&) {
                                    return suredetail::evalExpr(*body, &t, nullptr, nullptr);
                                },
                                [faces, ins, name](const IndexPoint& q) -> double {
                                    const ExprPtr* hit = nullptr;
                                    for (const auto& f : faces) {
                                        if (f.first.isInside(q)) {
                                            if (hit) throw std::runtime_error(
                                                "sure: multiple input faces of '" + name + "' cover the same point");
                                            hit = &f.second;
                                        }
                                    }
                                    if (!hit) throw std::runtime_error(
                                        "sure: access to '" + name + "' at a boundary point not covered by any input face");
                                    return suredetail::evalExpr(**hit, nullptr, &q, ins.get());
                                } });
                        }

                        // coverage: every out-of-domain tap image lands on exactly one
                        // input face of its variable (equation taps and output taps). Each
                        // equation is checked over ITS domain -- a per-equation restriction
                        // (e.g. j <= i) means its taps are only exercised on that sub-domain.
                        for (const auto& eq : eqs) {
                            std::vector<IndexPoint> eqPoints = sysPoints;
                            if (!eq.cons.empty()) {
                                std::vector<LinCon> combined = sysCons;
                                combined.insert(combined.end(), eq.cons.begin(), eq.cons.end());
                                std::vector<long> lo, hi;
                                eqPoints = buildDomain(indexNames, combined, lo, hi).enumerate();
                            }
                            for (const auto& t : eq.taps)
                                checkCoverage(t, eqPoints, eq.line, "equation for '" + eq.name + "'");
                        }
                        for (const auto& f : outFaces) {
                            std::vector<IndexPoint> facePoints = f.region.enumerate();
                            if (facePoints.empty()) failAt(f.line, "output face region of '" + f.tensor + "' is empty");
                            for (const auto& t : f.taps)
                                checkCoverage(t, facePoints, f.line, "output '" + f.tensor + "'");
                            // element-map range check
                            for (const auto& p : facePoints) {
                                for (std::size_t r = 0; r < f.dims.size(); ++r) {
                                    long v = f.elemB[r];
                                    for (std::size_t i = 0; i < p.size(); ++i) v += f.elemA[r][i] * p[i];
                                    if (v < 0 || v >= f.dims[r])
                                        failAt(f.line, "output element index of '" + f.tensor +
                                                       "' out of range at face point " + pointToString(p));
                                }
                            }
                        }

                        // publish the confluence structure
                        for (const auto& f : inFaces)
                            spec.inputs.push_back({ f.tensor, f.var, f.normal });
                        for (const auto& f : outFaces) {
                            SureOutput out;
                            out.tensor = f.tensor;
                            out.dims = f.dims;
                            out.var = f.taps.front().source;
                            out.region = f.region;
                            out.normal = f.normal;
                            auto elemA = f.elemA;
                            auto elemB = f.elemB;
                            out.elemIndex = [elemA, elemB](const IndexPoint& p) {
                                std::vector<long> idx(elemA.size(), 0);
                                for (std::size_t r = 0; r < elemA.size(); ++r) {
                                    idx[r] = elemB[r];
                                    for (std::size_t i = 0; i < p.size() && i < elemA[r].size(); ++i)
                                        idx[r] += elemA[r][i] * p[i];
                                }
                                return idx;
                            };
                            std::vector<std::pair<std::string, AffineDependency>> otaps;
                            for (const auto& t : f.taps)
                                otaps.push_back({ t.source, AffineDependency::map(t.A, t.b) });
                            ExprPtr expr = f.expr;
                            out.eval = [otaps, expr, ins](SureSimulator<double>& sim, const IndexPoint& p) {
                                std::vector<double> tv;
                                tv.reserve(otaps.size());
                                for (const auto& t : otaps) tv.push_back(sim.eval(t.first, t.second.apply(p)));
                                return suredetail::evalExpr(*expr, &tv, &p, ins.get());
                            };
                            spec.outputs.push_back(std::move(out));
                        }
                    }

                    long flux(const std::vector<int>& normal) const {
                        long s = 0;
                        for (std::size_t d = 0; d < normal.size() && d < spec.tau.size(); ++d)
                            s += static_cast<long>(spec.tau[d]) * normal[d];
                        return s;
                    }

                    void requireData(const Expr& e, int line) const {
                        std::vector<std::string> names;
                        collectArrays(e, names);
                        for (const auto& n : names)
                            if (!data->count(n))
                                failAt(line, "tensor '" + n + "' has no data binding (add: data " + n + " = ...;)");
                    }

                    void checkCoverage(const TapDecl& tap, const std::vector<IndexPoint>& points,
                                       int line, const std::string& where) {
                        AffineDependency map = AffineDependency::map(tap.A, tap.b);
                        for (const auto& p : points) {
                            IndexPoint q = map.apply(p);
                            if (systemDomain.isInside(q)) continue;
                            int hits = 0;
                            for (const auto& f : inFaces)
                                if (f.var == tap.source && f.region.isInside(q)) ++hits;
                            if (hits == 0)
                                failAt(line, where + ": access to '" + tap.source + "' at " + pointToString(q) +
                                             " is not covered by any input face");
                            if (hits > 1)
                                failAt(line, where + ": access to '" + tap.source + "' at " + pointToString(q) +
                                             " is covered by multiple input faces");
                        }
                    }
                };

            } // namespace suredetail

            // flux consistency of a schedule vector against the declared confluences:
            // with outward normal n, an input face needs tau.n < 0 (influx) and an
            // output face tau.n > 0 (outflux).  The parser enforces this for the
            // program's own tau; callers overriding the schedule (e.g. dfactl --tau)
            // must revalidate with the effective vector.
            inline void validateSureFlux(const SureSpec& spec, const std::vector<int>& tau) {
                auto flux = [&tau](const std::vector<int>& n) {
                    long s = 0;
                    for (std::size_t d = 0; d < n.size() && d < tau.size(); ++d)
                        s += static_cast<long>(tau[d]) * n[d];
                    return s;
                };
                for (const auto& in : spec.inputs)
                    if (flux(in.normal) >= 0)
                        throw std::runtime_error("sure: input face " +
                            (in.tensor.empty() ? std::string("(const)") : in.tensor) + " -> " + in.var +
                            " has non-negative flux (tau.n >= 0) under the requested tau");
                for (const auto& out : spec.outputs)
                    if (flux(out.normal) <= 0)
                        throw std::runtime_error("sure: output face " + out.tensor +
                            " has non-positive flux (tau.n <= 0) under the requested tau");
            }

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
