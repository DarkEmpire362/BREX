#pragma once

#include "../common.h"
#include "glob_machine.h"
#include "glob_executor.h"

namespace brex {
    // == Compiler Levels ==

    class GlobExpressionCompiler {
        private:
            // const GlobExpression* data;
            size_t max_index;
            std::vector<const CompiledState*> states;

            std::set<size_t>* compileExpression(const GlobExpression* expr, std::set<size_t>* next_states);           
            std::set<size_t>* compileLiteral(LiteralExpression* expr, std::set<size_t>* next_states);
            std::set<size_t>* compileSequence(SequenceExpression* expr, std::set<size_t>* next_states);
            std::set<size_t>* compileWildcard(WildcardExpression* expr, std::set<size_t>* next_states);
            std::set<size_t>* compileUnion(UnionExpression* expr, std::set<size_t>* next_states);
            std::set<size_t>* compileSubstitution(SubstitutionExpression* expr, std::set<size_t>* next_states);
        
        public:
            GlobExpressionCompiler(/*GlobExpression* data*/) : /* data(data),*/ max_index(0), states(std::vector<const CompiledState*>({new WildcardState(nullptr, nullptr)})) {;}
            virtual ~GlobExpressionCompiler() = default;
            static ExpressionMachine* compile(const GlobExpression* expr);
    };

    class GlobCompiler {
        private:
            std::vector<const CompiledFragment*> states;
            void compileFragments(std::vector<const GlobFragment*> fragments);
        public:
            GlobCompiler() : states(std::vector<const CompiledFragment*>()) {;}
           
            static FragmentMachine* compile(Glob* glob);
    };
}