#pragma once

#include "../common.h"
#include "glob.h"
#include "glob_compiler.h"
// #include <iostream>

namespace brex {
    template <typename TStr, typename TIter>
    class GlobExecutor {
        public:
            GlobExecutor(FragmentMachine* machine) : machine(machine), pathsep('/') {;}
            ~GlobExecutor() = default;

            bool match(TStr* str) {
                size_t final_state = machine->states.size();
                // If the size of the machine is zero, just return whether or
                // not the length of the string is zero, if it isn't that's an
                // instant fail.
                if (final_state == 0) {
                    return str->length() == 0;
                }

                TIter it = TIter{str, 0, (int64_t) str->size() - 1, 0};

                size_t current_state = 0;
                size_t backtrack_point = 0;
                bool backtrack_available;

                do {
                    if (current_state == final_state) {
                        if (backtrack_available) {
                            current_state = backtrack_point;
                        }
                        else {
                            return false;
                        }
                    }
                    
                    // I don't think this is needed anymore but I'm keeping it until all tests are written.
                    // RegexChar current = it.get();
                    // if (current == this->pathsep) {
                    //     it.inc();
                    //     continue;
                    // }
                    
                    if (machine->states[current_state]->tag == GlobFragmentTag::RecursiveWildcard) {
                        current_state++;
                        backtrack_point = current_state;
                        backtrack_available = true;
                    }
                    else {
                        bool matches = this->matchExpr(((const CompiledExpressionFragment*) machine->states[current_state])->exprMachine, &it);
                        if (matches) {
                            current_state++;
                        }
                        else if (backtrack_available) {
                            current_state = backtrack_point;
                        }
                        else {
                            return false;
                        }
                    }
                } while (it.valid());
                return current_state == final_state;
            }
        private:
            FragmentMachine* machine;
            RegexChar pathsep;

            bool matchExpr(ExpressionMachine* machine, TIter* it) {
                // Copy start states into current.
                std::set<size_t> current_state = std::set<size_t>(*(machine->start_states));

                while (it->valid()) {
                    RegexChar curr = it->get();
                    if (curr == this->pathsep) {
                        break;
                    }

                    std::set<size_t> next_state = std::set<size_t>();
                    for (auto state_id = current_state.begin(); state_id != current_state.cend(); state_id++) {
                        const CompiledState* state = machine->states[*state_id];
                        if (state->tag == CompiledStateTag::GroundState && ((const GroundState*) state)->activator == curr) {
                            for (auto ns = state->next_states->cbegin(); ns != state->next_states->cend(); ns++) {
                                next_state.insert(*ns);
                            }
                        }
                        if (state->default_states != nullptr) {
                            for (auto ds = state->default_states->cbegin(); ds != state->default_states->cend(); ds++) {
                                next_state.insert(*ds);
                            }
                        }
                    }
                    current_state = next_state;
                    it->inc();
                }

                return current_state.contains(0);
            }
    };

    typedef GlobExecutor<CString, CRegexIterator> CGlobExecutor;
    typedef GlobExecutor<UnicodeString, UnicodeRegexIterator> UnicodeGlobExecutor;
}