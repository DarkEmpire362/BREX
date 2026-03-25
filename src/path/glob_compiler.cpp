#include "glob.h"
#include "glob_compiler.h"
#include <algorithm>
#include <iostream>

namespace brex
{
    std::set<size_t>* GlobExpressionCompiler::compileExpression(const GlobExpression* expr, std::set<size_t>* next_states) {
        if (expr->tag == GlobExpressionTag::Literal) {
            return this->compileLiteral((LiteralExpression*) expr, next_states);
        }
        else if (expr->tag == GlobExpressionTag::Sequence) {
            return this->compileSequence((SequenceExpression*) expr, next_states);
        }
        else if (expr->tag == GlobExpressionTag::Union) {
            return this->compileUnion((UnionExpression*) expr, next_states);
        }
        else if (expr->tag == GlobExpressionTag::Wildcard) {
            return this->compileWildcard((WildcardExpression*) expr, next_states);
        }
        else if (expr->tag == GlobExpressionTag::Substitution) {
            return this->compileSubstitution((SubstitutionExpression*) expr, next_states);
        } 
        // Unreachable.
        return nullptr;
    }

    std::set<size_t>* GlobExpressionCompiler::compileLiteral(LiteralExpression* expr, std::set<size_t>* next_states) {
        std::set<size_t>* cnext = new std::set<size_t>(*next_states);
        for (auto it = expr->codes.rbegin(); it != expr->codes.rend(); it++) {
            this->max_index++;
            this->states.push_back(new GroundState(*it, cnext, nullptr));
            cnext = new std::set<size_t>({this->max_index});
        }
        return new std::set<size_t>({this->max_index});
    }

    std::set<size_t>* GlobExpressionCompiler::compileSequence(SequenceExpression* expr, std::set<size_t>* next_states) {
        for (auto it = expr->subexprs.rbegin(); it != expr->subexprs.rend(); it++) {
            next_states = this->compileExpression(*it, next_states);
        }
        return next_states;
    }

    std::set<size_t>* GlobExpressionCompiler::compileWildcard(WildcardExpression* expr, std::set<size_t>* next_states) {
        this->max_index++;
        std::set<size_t>* wildcards = new std::set<size_t>({ this->max_index });
        for (auto it = next_states->begin(); it != next_states->end(); it++) {
            std::set<size_t>* cds = this->states[*it]->default_states;
            if (cds != nullptr && cds->size() > 0) {
                for (auto jt = cds->cbegin(); jt != cds->cend(); jt++) {
                    wildcards->insert(*jt);
                }
            }
            wildcards->insert(*it);
        }
        
        this->states.push_back(new WildcardState({}, wildcards));
        return wildcards;
    }

    std::set<size_t>* GlobExpressionCompiler::compileUnion(UnionExpression* expr, std::set<size_t>* next_states) {
        std::set<size_t>* next_set = new std::set<size_t>({});
        for (auto it = expr->exprs.begin(); it != expr->exprs.end(); it++) {
            std::set<size_t>* next_branch = this->compileExpression(*it, next_states);
            for (auto jt = next_branch->cbegin(); jt != next_branch->cend(); jt++) {
                next_set->insert(*jt);
            }
        }
        return next_set;
    }

    std::set<size_t>* GlobExpressionCompiler::compileSubstitution(SubstitutionExpression* expr, std::set<size_t>* next_states) {
        this->states.push_back(new PlaceholderState(expr->name, next_states, nullptr));
        this->max_index++;
        return new std::set<size_t>({this->max_index});
    }

    ExpressionMachine* GlobExpressionCompiler::compile(const GlobExpression* expr) {
        GlobExpressionCompiler compiler = GlobExpressionCompiler();
        std::set<size_t>* start_states = compiler.compileExpression(expr, new std::set<size_t>({0}));
        return new ExpressionMachine(new std::set<size_t>(*start_states), compiler.states);
    }

    void GlobCompiler::compileFragments(std::vector<const GlobFragment*> fragments) {
        for (auto it = fragments.cbegin(); it != fragments.cend(); it++) {
            if ((*it)->tag == GlobFragmentTag::Expression) {
                const ExpressionFragment* f = (const ExpressionFragment*) (*it);
                ExpressionMachine* machine = GlobExpressionCompiler::compile(f->expression);
                this->states.push_back(new CompiledExpressionFragment(machine));
            }
            else if ((*it)->tag == GlobFragmentTag::RecursiveWildcard) {
                this->states.push_back(new CompiledRecursiveWildcardFragment());
            }
        }
    }

    FragmentMachine* GlobCompiler::compile(Glob* glob) {
        GlobCompiler c = GlobCompiler();
        c.compileFragments(glob->fragments);
        return new FragmentMachine(c.states);
    }
} // namespace brex
