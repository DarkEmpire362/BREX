#include "glob_machine.h"

namespace brex {
    void ExpressionMachine::link(std::u8string symbol, ExpressionMachine* machine) {
        // Precondition check: `machine` must be fully linked.
        for (auto it = machine->states.cbegin(); it != machine->states.cend(); it++) {
            if ((*it)->tag == CompiledStateTag::Placeholder) {
                // TODO: Errors
                exit(-1);
            }
        }

        size_t end = this->states.size();
        for (size_t i = 0; i < end; i++) {
            if (this->states[i]->tag == CompiledStateTag::Placeholder) {
                if (((PlaceholderState*)(this->states[i]))->symbol == symbol) {
                    this->innerLink(i, machine);
                    // This is just a completely empty unreachable state. We can
                    // do some optimization later to remove these.
                    this->states[i] = new WildcardState({}, {});
                }
            }
        }
    }

    void ExpressionMachine::innerLink(size_t id, ExpressionMachine* machine) {
        // Current maximum state index
        size_t n = this->states.size();

        // Next States for the one we're replacing
        std::set<size_t>* next = this->states[id]->next_states;

        // Start states for the machine we're introducing
        std::set<size_t>* new_starts = machine->start_states;

        // Phase 1: Merge the state machines. Add all states from `machine` to
        // `this`. This first loop should create an array matching old state ids
        // to new state ids.

        std::vector<size_t> map = std::vector<size_t>();
        // We can actually guarantee that we don't need the zeroth state. That
        // will always be the final state and have no transitions. Even if we
        // end on a substitution, we want to link to this state machine's final
        // state, instead of the one we're adding.
        map.push_back(0);

        for (size_t i = 1; i < machine->states.size(); i++) {
            map.push_back(n+i-1); // Maps i to n+i-1
            this->states.push_back(machine->states[i]);
        }
        
        // Phase 2: Loop through added states. Replace all old state ids with
        // new state ids. Replace the final state from `machine` (state 0) with
        // the next states of the one we're replacing

        size_t k = this->states.size();
        for (size_t i = n; i < k; i++) {
            // If next states contains 'machine's final state, update to the
            // next_states of the new machine.
            if (this->states[i]->next_states->contains(0)) {
                this->states[i]->next_states->erase(0);
                for (auto s = next->cbegin(); s != next->cend(); s++) {
                    this->states[i]->next_states->insert(*s);
                }
            }
            // Same as above, but for default_states
            if (this->states[i]->default_states->contains(0)) {
                this->states[i]->default_states->erase(0);
                for (auto s = next->cbegin(); s != next->cend(); s++) {
                    this->states[i]->default_states->insert(*s);
                }
            }

            // Loop from 1 to k-n, if the set contains 'j', swap it with map[j]
            for (size_t j = 1; j < machine->states.size(); j++) {
                if (this->states[i]->next_states->contains(j)) {
                    this->states[i]->next_states->erase(j);
                    this->states[i]->next_states->insert(map[j]);
                }
                if (this->states[i]->default_states->contains(j)) {
                    this->states[i]->default_states->erase(j);
                    this->states[i]->default_states->insert(map[j]);
                }
            }
        }

        // Phase 3: Loop through pre-existing states. If any state contains the
        // state-id of the one we're replacing, we can safely replace it with
        // the starting states of `machine` (assuming we've updated those
        // properly)

        // Update starting states of `machine` to their replacements...
        std::set<size_t> updated_starts = std::set<size_t>();
        for (auto it = new_starts->cbegin(); it != new_starts->cend(); it++) {
            if (*it == 0) {
                for (auto s = next->cbegin(); s != next->cend(); s++) {
                    updated_starts.insert(*s);
                }
            }
            else {
                updated_starts.insert(map[*it]);
            }
        }

        // Replace `id` in the current start states with `updated_starts`.
        if (this->start_states->contains(id)) {
            this->start_states->erase(id);
            for (auto jt = updated_starts.cbegin(); jt != updated_starts.cend(); jt++) {
                this->start_states->insert(*jt);
            }
        }

        // Replace all instances of `id` with `updated_starts`
        for (size_t i = 0; i < n; i++) {
            if (this->states[i]->next_states->contains(id)) {
                this->states[i]->next_states->erase(id);
                for (auto jt = updated_starts.cbegin(); jt != updated_starts.cend(); jt++) {
                    this->states[i]->next_states->insert(*jt);
                }
            }
            if (this->states[i]->default_states->contains(id)) {
                this->states[i]->default_states->erase(id);
                for (auto jt = updated_starts.cbegin(); jt != updated_starts.cend(); jt++) {
                    this->states[i]->default_states->insert(*jt);
                }
            }
        }
    }

    void FragmentMachine::link(std::u8string symbol, ExpressionMachine* machine) {
        // Loop through all contained expression machines and run link with the above args.
        for (auto it = this->states.begin(); it != this->states.end(); it++) {
            if ((*it)->tag == GlobFragmentTag::Expression) {
                ((CompiledExpressionFragment*)(*it))->exprMachine->link(symbol, machine);
            }
        }
    }
}