/** \file
 * Past implementation.
 */

#include "past.h"

#include "ql/utils/filesystem.h"

// uncomment next line to enable multi-line dumping
// #define MULTI_LINE_LOG_DEBUG

namespace ql {
namespace pass {
namespace map {
namespace qubits {
namespace map {
namespace detail {

/**
 * Past initializer.
 */
void Past::initialize(const ir::compat::KernelRef &k, const OptionsRef &opt) {
    QL_DOUT("Past::initialize");
    platform = k->platform;
    kernel = k;
    options = opt;

    nq = platform->qubit_count;
    nb = platform->breg_count;
    ct = platform->cycle_time;

    QL_ASSERT(kernel->gates.empty());     // kernelp->c will be used by new_gate to return newly created gates into
    v2r.resize(                       // v2r initializtion until v2r is imported from context
        nq,
        true,
        options->assume_initialized ? com::map::QubitState::INITIALIZED : com::map::QubitState::NONE
    );
    fc.initialize(platform, options); // fc starts off with all qubits free, is updated after schedule of each gate
    waiting_gates.clear();            // no gates pending to be scheduled in; Add of gate to past entered here
    gates.clear();                    // no gates scheduled yet in this past; after schedule of gate, it gets here
    output_gates.clear();             // no gates output yet by flushing from or bypassing this past
    num_swaps_added = 0;              // no swaps or moves added yet to this past; AddSwap adds one here
    num_moves_added = 0;              // no moves added yet to this past; AddSwap may add one here
    cycle.clear();                    // no gates have cycles assigned in this past; scheduling gate updates this
}

/**
 * Copies the given qubit mapping into our mapping.
 */
void Past::import_mapping(const com::map::QubitMapping &v2r_value) {
    v2r = v2r_value;
}

/**
 * Copies our qubit mapping into the given mapping.
 */
void Past::export_mapping(com::map::QubitMapping &v2r_destination) const {
    v2r_destination = v2r;
}

/**
 * Prints the state of the embedded FreeCycle object.
 */
void Past::print_fc() const{
    fc.print("");
}

/**
 * Prints the state of the embedded FreeCycle object only when verbosity
 * is at least debug.
 */
void Past::debug_print_fc() const {
#ifdef MULTI_LINE_LOG_DEBUG
    QL_IF_LOG_DEBUG {
        QL_DOUT("FreeCycle dump:");
        fc.print("");
    }
#else
    QL_DOUT("FreeCycle dump (disabled)");
#endif
}

/**
 * Prints the state of this object along with the given string.
 */
void Past::print(const utils::Str &s) const {
    std::cout << "... Past " << s << ":";
    v2r.dump_state();
    fc.print("");
    // QL_DOUT("... list of gates in past");
    for (auto &gp : gates) {
        QL_DOUT("[" << cycle.at(gp) << "] " << gp->qasm());
    }
}

/**
 * Schedules all waiting gates into the main gates list. Note that these
 * gates all are mapped and so have real operand qubit indices. The
 * FreeCycle map reflects for each qubit the first free cycle. All new
 * gates, now in waitinglist, get such a cycle assigned below, increased
 * gradually, until definitive.
 */
void Past::schedule() {
    // the copy includes the resource manager.
    // QL_DOUT("Schedule ...");

    while (!waiting_gates.empty()) {
        utils::UInt start_cycle = ir::compat::MAX_CYCLE;
        utils::List<ir::compat::GateRef>::iterator gate_it;

        // Find the gate with the minimum start cycle.
        //
        // IMPORTANT: this assumes that the waiting gates list is in topological
        // order, which is ok because the pair of swap lists use distinct qubits
        // and the gates of each are added to the back of the list in the order
        // of execution. Using tryfc.add, the tryfc (try FreeCycle map) reflects
        // the earliest start cycle per qubit, and so dependencies are
        // respected, so we can find the gate that can start first...
        //
        // Note that tryfc includes the free cycle vector AND the resource map,
        // so using tryfc.StartCycle/tryfc.Add we get a realistic ASAP rc
        // schedule. We use a copy of fc and not fc itself, since the latter
        // reflects the really scheduled gates and that shouldn't be changed.
        //
        // This search is really a hack to avoid the construction of a
        // dependency graph and a set of schedulable gates.
        FreeCycle tryfc = fc;
        for (auto try_gate_it = waiting_gates.begin(); try_gate_it != waiting_gates.end(); ++try_gate_it) {
            utils::UInt try_start_cycle = tryfc.get_start_cycle(*try_gate_it);
            tryfc.add(*try_gate_it, try_start_cycle);

            if (try_start_cycle < start_cycle) {
                start_cycle = try_start_cycle;
                gate_it = try_gate_it;
            }
        }

        auto gate = *gate_it;

        // Add this gate to the maps, scheduling the gate (doing the cycle
        // assignment).
        // QL_DOUT("... add " << gp->qasm() << " startcycle=" << startCycle << " cycles=" << ((gp->duration+ct-1)/ct) );
        fc.add(gate, start_cycle);
        cycle.set(gate) = start_cycle; // cycle[gp] is private to this past but gp->cycle is private to gp
        gate->cycle = start_cycle; // so gp->cycle gets assigned for each alter' Past and finally definitively for mainPast
        // QL_DOUT("... set " << gp->qasm() << " at cycle " << startCycle);

        // Insert gate into the list of gates, in cycle[gp] order, and inside
        // this order, as late as possible.
        //
        // Reverse iterate because the insertion is near the end of the list.
        // Insert so that cycle values are in order afterwards and the new one
        // is nearest to the end.
        auto rigp = gates.rbegin();
        utils::Bool inserted = false;
        for (; rigp != gates.rend(); rigp++) {
            if (cycle.at(*rigp) <= start_cycle) {

                // rigp.base() because insert doesn't work with reverse iteration
                // rigp.base points after the element that rigp is pointing at
                // which is lucky because insert only inserts before the given
                // element. The net result is inserting after rigp.
                gates.insert(rigp.base(), gate);

                inserted = true;
                break;
            }
        }

        // When list was empty or no element was found, just put it in front.
        if (!inserted) {
            gates.push_front(gate);
        }

        // Having added it to the main list, remove it from the waiting list.
        waiting_gates.erase(gate_it);
    }

    // DPRINT("Schedule:");
}

/**
 * Computes the costs in cycle extension of optionally scheduling
 * init_circuit before the inevitable circuit.
 */
utils::Int Past::get_insertion_cost(
    const ir::compat::GateRefs &init_circuit,
    const ir::compat::GateRefs &circuit
) const {

    // First fake-schedule init_circuit followed by circuit in a private
    // FreeCycle map.
    FreeCycle fc_with_init = fc;
    for (const auto &gate : init_circuit) {
        utils::UInt try_start_cycle = fc_with_init.get_start_cycle_no_rc(gate);
        fc_with_init.add_no_rc(gate, try_start_cycle);
    }
    for (const auto &gate : circuit) {
        utils::UInt tryStartCycle = fc_with_init.get_start_cycle_no_rc(gate);
        fc_with_init.add_no_rc(gate, tryStartCycle);
    }
    utils::UInt max_with_init = fc_with_init.get_max(); // this reflects the depth afterwards

    // Then fake-schedule circuit alone in a private FreeCycle map.
    FreeCycle fc_without_init = fc;
    for (auto &gate : circuit) {
        utils::UInt try_start_cycle = fc_without_init.get_start_cycle_no_rc(gate);
        fc_without_init.add_no_rc(gate, try_start_cycle);
    }
    utils::UInt max_without_init = fc_without_init.get_max(); // this reflects the depth afterwards

    QL_DOUT("... scheduling init+circ => depth " << max_with_init << ", scheduling circ => depth " << max_without_init << ", init insertion cost " << (max_with_init - max_without_init));
    QL_ASSERT(max_with_init >= max_without_init);
    // scheduling initcirc would be for free when initmax == max, so the cost is (initmax - max)
    return max_with_init - max_without_init;
}

/**
 * Adds the given mapped gate to the current past. This means adding it to
 * the current past's waiting list, waiting for it to be scheduled later.
 */
void Past::add(const ir::compat::GateRef &gate) {
    waiting_gates.push_back(gate);
}

/**
 * Creates a new gate with given name and qubits, returning whether this was
 * successful. Return the created gate(s) in circ, which is supposed to be
 * empty on entry.
 *
 * Since kernel.h only provides a gate interface as method of class
 * Kernel, that adds the gate to kernel.c, and we want the gate (or its
 * decomposed sequence) here to be added to circ, the kludge is implemented
 * to make sure that kernel.c (the current kernel's mapper input/output
 * circuit) is available for this. In class Future, kernel.c is copied into
 * the dependence graph or copied to a local circuit, and in
 * Mapper::route, a temporary local output circuit is used, which is
 * written to kernel.c only at the very end.
 */
utils::Bool Past::new_gate(
    ir::compat::GateRefs &circ,
    const utils::Str &gname,
    const utils::Vec<utils::UInt> &qubits,
    const utils::Vec<utils::UInt> &cregs,
    utils::UInt duration,
    utils::Real angle,
    const utils::Vec<utils::UInt> &bregs,
    ir::compat::ConditionType gcond,
    const utils::Vec<utils::UInt> &gcondregs
) const {
    utils::Bool added;
    QL_ASSERT(circ.empty());
    QL_ASSERT(kernel->gates.empty());
    // create gate(s) in kernelp->c
    added = kernel->gate_nonfatal(gname, qubits, cregs, duration, angle, bregs, gcond, gcondregs);
    circ = kernel->gates;
    kernel->gates.reset();
    for (const auto &gate : circ) {
        QL_DOUT("new_gate added: " << gate->qasm());
    }
    QL_ASSERT(!(added && circ.empty()));
    return added;
}

/**
 * Returns the number of swaps added to this past.
 */
utils::UInt Past::get_num_swaps_added() const {
    return num_swaps_added;
}

/**
 * Returns the number of moves added to this past.
 */
utils::UInt Past::get_num_moves_added() const {
    return num_moves_added;
}

/**
 * Shorthand for throwing an exception for a non-existant gate.
 */
static void new_gate_exception(const utils::Str &s) {
    QL_FATAL("gate is not supported by the target platform: '" << s << "'");
}

/**
 * Returns whether swap(fr0,fr1) starts earlier than swap(sr0,sr1). This is
 * really a short-cut ignoring config file and perhaps several other
 * details.
 */
utils::Bool Past::is_first_swap_earliest(
    utils::UInt fr0,
    utils::UInt fr1,
    utils::UInt sr0,
    utils::UInt sr1
) const {
    return fc.is_first_swap_earliest(fr0, fr1, sr0, sr1);
}

/**
 * Generates a move into circ with parameters r0 and r1 (which
 * generate_move() may reverse). Whether this was successfully done can be
 * seen from whether circ was extended. Please note that the reversal of
 * operands may have been done also when generate_move() was not successful.
 */
void Past::generate_move(ir::compat::GateRefs &circuit, utils::UInt &r0, utils::UInt &r1) {
    if (v2r.get_state(r0) != com::map::QubitState::LIVE) {
        QL_ASSERT(
            v2r.get_state(r0) == com::map::QubitState::NONE ||
            v2r.get_state(r0) == com::map::QubitState::INITIALIZED
        );
        // Interchange r0 and r1, so that r1 (right-hand operand of move) will
        // be the state-less one.
        utils::UInt tmp = r1; r1 = r0; r0 = tmp;
        // QL_DOUT("... reversed operands for move to become move(q" << r0 << ",q" << r1 << ") ...");
    }
    QL_ASSERT(v2r.get_state(r0) == com::map::QubitState::LIVE);    // and r0 will be the one with state
    QL_ASSERT(v2r.get_state(r1) != com::map::QubitState::LIVE);    // and r1 will be the one without state (QubitState::NONE || com::QubitState::INITIALIZED)

    // First (optimistically) create the move circuit and add it to circuit.
    utils::Bool created;
    if (platform->topology->is_inter_core_hop(r0, r1)) {
        if (options->heuristic == Heuristic::MAX_FIDELITY) {
            created = new_gate(circuit, "tmove_prim", {r0, r1});    // gates implementing tmove returned in circ
        } else {
            created = new_gate(circuit, "tmove_real", {r0, r1});    // gates implementing tmove returned in circ
        }
        if (!created) {
            created = new_gate(circuit, "tmove", {r0, r1});
            if (!created) {
                new_gate_exception("tmove or tmove_real");
            }
        }
    } else {
        if (options->heuristic == Heuristic::MAX_FIDELITY) {
            created = new_gate(circuit, "move_prim", {r0, r1});    // gates implementing move returned in circ
        } else {
            created = new_gate(circuit, "move_real", {r0, r1});    // gates implementing move returned in circ
        }
        if (!created) {
            created = new_gate(circuit, "move", {r0, r1});
            if (!created) {
                new_gate_exception("move or move_real");
            }
        }
    }

    if (v2r.get_state(r1) == com::map::QubitState::NONE) {
        // r1 is not in inited state, generate in initcirc the circuit to do so
        // QL_DOUT("... initializing non-inited " << r1 << " to |0> (inited) state preferably using move_init ...");
        ir::compat::GateRefs init_circuit;

        created = new_gate(init_circuit, "move_init", {r1});
        if (!created) {
            created = new_gate(init_circuit, "prepz", {r1});
            // if (created)
            // {
            //     created = new_gate(initcirc, "h", {r1});
            //     if (!created) new_gate_exception("h");
            // }
            if (!created) {
                new_gate_exception("move_init or prepz");
            }
        }

        // When difference in extending circuit after scheduling
        // init_circuit+circuit or just circuit is less equal than threshold
        // cycles (0 would mean scheduling initcirc was for free), commit to it,
        // otherwise abort.
        if (get_insertion_cost(init_circuit, circuit) <= static_cast<utils::Int>(options->max_move_penalty)) {

            // So we go for it! circuit contains move; it must get the
            // init_circuit before it. Do this by appending circ's gates to
            // init_circuit, and then swapping circuit and init_circuit's
            // contents.
            QL_DOUT("... initialization is for free, do it ...");
            for (auto &gp : circuit) {
                init_circuit.add(gp);
            }
            circuit.get_vec().swap(init_circuit.get_vec());
            v2r.set_state(r1, com::map::QubitState::INITIALIZED);

        } else {

            // Undo damage done, will not do move but swap, i.e. nothing created
            // thus far.
            QL_DOUT("... initialization extends circuit, don't do it ...");
            circuit.reset();       // circ being cleared also indicates creation wasn't successful

        }
        // initcirc going out of scope here so it gets destroyed.
    }
}

/**
 * Generates a single swap/move with real operands and adds it to the
 * current past's waiting list. Note that the swap/move may be implemented
 * by a series of gates (circuit circ below), and that a swap/move
 * essentially is a commutative operation, interchanging the states of the
 * two qubits.
 *
 * A move is implemented by 2 CNOTs, while a swap is 3 CNOTs, provided the
 * target qubit is in |0> (inited) state. So, when one of the operands is
 * the current location of an unused virtual qubit, use a move with that
 * location as 2nd operand, after first having initialized the target qubit
 * in |0> (inited) state when that has not been done already. However, this
 * initialization must not extend the depth (beyond the configured limit),
 * so this can only be done when cycles for it are for free.
 */
void Past::add_swap(utils::UInt r0, utils::UInt r1) {
    utils::Bool created = false;

    QL_DOUT("... extending with swap(q" << r0 << ",q" << r1 << ") ...");
    QL_DOUT("... adding swap/move: " << v2r.real_to_string(r0) << ", " << v2r.real_to_string(r1));

    QL_ASSERT(v2r.get_state(r0) == com::map::QubitState::INITIALIZED ||
                  v2r.get_state(r0) == com::map::QubitState::NONE ||
                  v2r.get_state(r0) == com::map::QubitState::LIVE);
    QL_ASSERT(v2r.get_state(r1) == com::map::QubitState::INITIALIZED ||
                  v2r.get_state(r1) == com::map::QubitState::NONE ||
                  v2r.get_state(r1) == com::map::QubitState::LIVE);

    if (v2r.get_state(r0) != com::map::QubitState::LIVE &&
        v2r.get_state(r1) != com::map::QubitState::LIVE) {
        QL_DOUT("... no state in both operand of intended swap/move; don't add swap/move gates");
        v2r.swap(r0, r1);
        return;
    }

    // Store the virtual qubits corresponding to each real qubit.
    utils::UInt v0 = v2r.get_virtual(r0);
    utils::UInt v1 = v2r.get_virtual(r1);

    ir::compat::GateRefs circuit;   // current kernel copy, clear circuit
    if (options->use_move_gates && (v2r.get_state(r0) != com::map::QubitState::LIVE ||
        v2r.get_state(r1) != com::map::QubitState::LIVE)) {
        generate_move(circuit, r0, r1);
        created = circuit.size() != 0;
        if (created) {
            // generated move
            // move is in circ, optionally with initialization in front of it
            // also rs of its 2nd operand is 'QubitState::INITIALIZED'
            // note that after swap/move, r0 will be in this state then
            num_moves_added++;                       // for reporting at the end
            QL_DOUT("... move(q" << r0 << ",q" << r1 << ") ...");
        } else {
            QL_DOUT("... move(q" << r0 << ",q" << r1 << ") cancelled, go for swap");
        }
    }
    if (!created) {
        // no move generated so do swap
        if (options->reverse_swap_if_better) {
            // swap(r0,r1) is about to be generated
            // it is functionally symmetrical,
            // but in the implementation r1 starts 1 cycle earlier than r0 (we should derive this from json file ...)
            // so swap(r0,r1) with interchanged operands might get scheduled 1 cycle earlier;
            // when fcv[r0] < fcv[r1], r0 is free for use 1 cycle earlier than r1, so a reversal will help
            if (fc.is_first_operand_earlier(r0, r1)) {
                utils::UInt  tmp = r1; r1 = r0; r0 = tmp;
                QL_DOUT("... reversed swap to become swap(q" << r0 << ",q" << r1 << ") ...");
            }
        }
        if (platform->topology->is_inter_core_hop(r0, r1)) {
            if (options->heuristic == Heuristic::MAX_FIDELITY) {
                created = new_gate(circuit, "tswap_prim", {r0, r1});    // gates implementing tswap returned in circ
            } else {
                created = new_gate(circuit, "tswap_real", {r0, r1});    // gates implementing tswap returned in circ
            }
            if (!created) {
                created = new_gate(circuit, "tswap", {r0, r1});
                if (!created) {
                    new_gate_exception("tswap or tswap_real");
                }
            }
            QL_DOUT("... tswap(q" << r0 << ",q" << r1 << ") ...");
        } else {
            if (options->heuristic == Heuristic::MAX_FIDELITY) {
                created = new_gate(circuit, "swap_prim", {r0, r1});    // gates implementing swap returned in circ
            } else {
                created = new_gate(circuit, "swap_real", {r0, r1});    // gates implementing swap returned in circ
            }
            if (!created) {
                created = new_gate(circuit, "swap", {r0, r1});
                if (!created) {
                    new_gate_exception("swap or swap_real");
                }
            }
            QL_DOUT("... swap(q" << r0 << ",q" << r1 << ") ...");
        }
    }
    num_swaps_added++; // for reporting at the end

    // Add each gate in the resulting circuit.
    for (auto &gp : circuit) {
        add(gp);
        // each gate in circ is part of a swap or move, so add the parameters
        //TODO: uint to int conversion
        const ir::compat::SwapParameters swap_params {true, (utils::Int) r0, (utils::Int) r1, (utils::Int) v1, (utils::Int) v0};
        gp->swap_params = swap_params;
    }

    // Reflect in v2r that r0 and r1 interchanged state, i.e. update the map to
    // reflect the swap.
    v2r.swap(r0, r1);
}

/**
 * Adds the mapped gate (with real qubit indices as operands) to the past
 * by adding it to the waiting list and scheduling it into the past.
 */
void Past::add_and_schedule(const ir::compat::GateRef &gate) {
    add(gate);
    schedule();
}

/**
 * Returns the real qubit index implementing virtual qubit index.
 */
utils::UInt Past::get_real_qubit(utils::UInt virt) {
    utils::UInt r = v2r[virt];
    QL_ASSERT(r != com::map::UNDEFINED_QUBIT);
    QL_DOUT("get_real_qubit(virt=" << virt << " mapped to real=" << r);
    return r;
}

/**
 * Strips the fixed qubit operands (if any) from the given gate name.
 */
static void strip_name(utils::Str &name) {
    QL_DOUT("strip_name(name=" << name << ")");
    utils::UInt p = name.find(' ');
    if (p != utils::Str::npos) {
        name = name.substr(0,p);
    }
    QL_DOUT("... after strip_name name='" << name << "'");
}

/**
 * Turns the given gate into a "real" gate.
 *
 * See header file for more information.
 */
void Past::make_real(const ir::compat::GateRef &gate, ir::compat::GateRefs &circuit) {
    QL_DOUT("make_real: " << gate->qasm());

    utils::Str gname = gate->name;
    strip_name(gname);

    utils::Vec<utils::UInt> real_qubits = gate->operands; // starts off as copy of virtual qubits!
    for (auto &qi : real_qubits) {
        qi = get_real_qubit(qi);          // and now they are real
        if (options->assume_prep_only_initializes && (gname == "prepz" || gname == "Prepz")) {
            v2r.set_state(qi, com::map::QubitState::INITIALIZED);
        } else {
            v2r.set_state(qi, com::map::QubitState::LIVE);
        }
    }

    utils::Str real_gname = gname;
    if (options->heuristic == Heuristic::MAX_FIDELITY) {
        QL_DOUT("make_real: with mapper==maxfidelity generate _prim");
        real_gname.append("_prim");
    } else {
        real_gname.append("_real");
    }

    utils::Bool created = new_gate(
        circuit,
        real_gname,
        real_qubits,
        gate->creg_operands,
        gate->duration,
        gate->angle,
        gate->breg_operands,
        gate->condition,
        gate->cond_operands
    );
    if (!created) {
        created = new_gate(
            circuit,
            gname,
            real_qubits,
            gate->creg_operands,
            gate->duration,
            gate->angle,
            gate->breg_operands,
            gate->condition,
            gate->cond_operands
        );
        if (!created) {
            QL_FATAL("make_real: failed creating gate " << real_gname << " or " << gname);
        }
    }
    QL_DOUT("... make_real: new gate created for: " << real_gname << " or " << gname);

    if (gate->swap_params.part_of_swap) {
        QL_DOUT("original gate was swap/move, adding swap/move parameters for gates in decomposed circuit");
        for (ir::compat::GateRef &it : circuit) {
            it->swap_params = gate->swap_params;
        }
    }
}

/**
 * Mapper after-burner. Used to make primitives of all gates that also have
 * a config file entry with _prim appended to their name, decomposing it
 * according to the config file gate decomposition.
 */
void Past::make_primitive(const ir::compat::GateRef &gate, ir::compat::GateRefs &circuit) const {
    utils::Str gname = gate->name;
    strip_name(gname);
    utils::Str prim_gname = gname;
    prim_gname.append("_prim");
    utils::Bool created = new_gate(
        circuit,
        prim_gname,
        gate->operands,
        gate->creg_operands,
        gate->duration,
        gate->angle,
        gate->breg_operands,
        gate->condition,
        gate->cond_operands
    );
    if (!created) {
        created = new_gate(
            circuit,
            gname,
            gate->operands,
            gate->creg_operands,
            gate->duration,
            gate->angle,
            gate->breg_operands,
            gate->condition,
            gate->cond_operands
        );
        if (!created) {
            QL_FATAL("make_primitive: failed creating gate " << prim_gname << " or " << gname);
        }
        QL_DOUT("... make_primitive: new gate created for: " << gname);
    } else {
        QL_DOUT("... make_primitive: new gate created for: " << prim_gname);
    }

    if (gate->swap_params.part_of_swap) {
        QL_DOUT("original gate was swap/move, adding swap/move parameters for gates in decomposed circuit");
        for (const ir::compat::GateRef &it : circuit) {
            it->swap_params = gate->swap_params;
        }
    }
}

/**
 * Returns the first completely free cycle.
 */
utils::UInt Past::get_max_free_cycle() const {
    return fc.get_max();
}

/**
 * Non-quantum and quantum gates follow separate flows through Past:
 *
 *  - Quantum gates are put in the waiting gate list when added, are then
 *    scheduled, and are finally ordered by cycle into the main gate list.
 *    They wait there to be inspected and scheduled, until there are too
 *    many, a non-quantum gate comes by, or the end of the circuit is
 *    reached.
 *  - Non-quantum nonq gates first cause the main gate list to be
 *    flushed/cleared to output before the non-quantum gate is output.
 *
 * All gates in the output gate list are out of view for scheduling/mapping
 * optimization and can be taken out to someplace else.
 */
void Past::flush_all() {
    for (const auto &gate : gates) {
        output_gates.push_back(gate);
    }
    gates.clear();         // so effectively, lg's content was moved to outlg

    // fc.Init(platformp, nb); // needed?
    // cycle.clear();      // needed?
    // cycle is initialized to empty map
    // is ok without windowing, but with window, just delete the ones outside the window
}

/**
 * Add the given non-qubit gate directly to the output list.
 */
void Past::bypass(const ir::compat::GateRef &gate) {
    if (!gates.empty()) {
        flush_all();
    }
    output_gates.push_back(gate);
}

/**
 * Flushes the output gate list to the given circuit.
 */
void Past::flush_to_circuit(ir::compat::GateRefs &output_circuit) {
    for (const auto &gate : output_gates) {
        output_circuit.add(gate);
    }
    output_gates.clear();
}

} // namespace detail
} // namespace map
} // namespace qubits
} // namespace map
} // namespace pass
} // namespace ql
