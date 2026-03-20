#pragma once

#include "../common.h"
#include "glob.h"
#include "glob_compiler.h"
// #include <iostream>

namespace brex {
    std::pair<bool, size_t> matchExpr(ExpressionMachine* machine, std::vector<RegexChar> cc, size_t start_pos) {
        assert(cc.size() - start_pos > 0);

        // Copy constructor
        std::set<size_t> current_state = std::set<size_t>(*(machine->start_states));

        for (auto it = cc.cbegin() + start_pos; it != cc.cend(); it++) {
            // TODO: Path separator variable for when these functions are inside a class
            if ((*it) == '/') {
                break;
            }

            std::set<size_t> next_state = std::set<size_t>();

            for (auto state_id = current_state.begin(); state_id != current_state.cend(); state_id++) {
                const CompiledState* state = machine->states[*state_id];
                if (state->tag == CompiledStateTag::GroundState && ((const GroundState*)state)->activator == *it) {
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
            start_pos++;
        }

        return std::pair<bool, size_t>(current_state.contains(0), start_pos);
    }

    bool match(FragmentMachine* machine, std::string str, bool unicode) {
        std::optional<std::vector<RegexChar>> char_codes;
        if (unicode) {
            char_codes = unescapeUnicodeRegexLiteral((uint8_t*) str.data(), str.length());
        }
        else {
            char_codes = unescapeCRegexLiteral((uint8_t*) str.data(), str.length());
        }

        if (!char_codes.has_value()) {
            // TODO: Errors, Class
            return false;
        }


        std::vector<RegexChar> cc = char_codes.value();

        size_t start_pos = 0;
        size_t end_pos = cc.size();
       
        bool reset_available = false;
        size_t reset_state = 0;
        size_t current_state = 0;
        size_t final_state = machine->states.size();
        while (start_pos < end_pos) {
            // std::cout << (int) current_state << " " << final_state << std::endl;
            if (cc[start_pos] == '/') {
                start_pos++;
            }
            // If we make it to a recursive wildcard, it becomes an anchor. If we fail we can backtrack to it. There can only be one anchor.
            if (machine->states[current_state]->tag == GlobFragmentTag::RecursiveWildcard) {
                current_state++;
                reset_state = current_state;
                reset_available = true;
            }
            else {
                std::pair<bool, size_t> match = matchExpr(((const CompiledExpressionFragment*) machine->states[current_state])->exprMachine, cc, start_pos);
                start_pos = match.second;
                if (match.first) {
                    current_state++;
                }
                else if (reset_available) {
                    current_state = reset_state;
                }
                else {
                    return false;
                }
            }

            if (current_state == final_state) {
                if (start_pos >= end_pos) {
                    return true;
                }
                current_state = reset_state;
            }
        }
        return false;
    }
}