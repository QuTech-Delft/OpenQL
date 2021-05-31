/** \file
 * Defines a shared instrument resource based on the qubits used for quantum
 * gates.
 */

#include "ql/resource/instrument.h"

/*#undef QL_DOUT
#define QL_DOUT(x) ::std::cout << x << ::std::endl
#undef QL_IF_LOG_DEBUG
#define QL_IF_LOG_DEBUG if (1)*/

namespace ql {
namespace resource {
namespace instrument {

/**
 * Represents a single qubit.
 */
using Qubit = utils::UInt;

/**
 * Represents an edge between two qubits.
 */
using Edge = utils::Pair<utils::UInt, utils::UInt>;

/**
 * Represents an instrument index.
 */
using Instrument = utils::UInt;

/**
 * Represents a list of instrument indices.
 */
using Instruments = utils::Vec<Instrument>;

/**
 * Represents an instrument function index.
 */
using Function = utils::UInt;

/**
 * Represents a gate predicate.
 */
using Predicate = utils::Pair<utils::Str, utils::Set<utils::Str>>;

/**
 * Represents a list of predicate.
 */
using Predicates = utils::Vec<Predicate>;

/**
 * Configuration structure. This does not need to be copied every time the
 * resource state is cloned; instead we keep a shared_ptr to it instead.
 */
struct Config {

    /**
     * Instrument name map for debugging.
     */
    utils::Vec<utils::Str> instrument_names;

    /**
     * Gate predicates. The array index is the number of qubit operands
     * minus one, clamped to 2 maximum (so three-or-more-qubit gates are
     * bunched together).
     */
    Predicates predicates[3];

    /**
     * Gate type keys, i.e. the (custom) JSON keys in the gate structures used
     * to determine the instrument function.
     */
    utils::Vec<utils::Str> function_keys;

    /**
     * Map from gate type combinations we've seen thus far to a number, to
     * keep the state tracker memory footprint down.
     */
    utils::Map<utils::Vec<utils::Str>, Function> function_map;

    /**
     * When set, function_keys is ignored, function_map is unused, and all
     * instrument usage is considered to be mutually exclusive.
     */
    utils::Bool mutually_exclusive;

    /**
     * When set, gates that require the same instrument and use the same
     * function do not necessarily have to start and end simultaneously.
     */
    utils::Bool allow_overlap;

    /**
     * Map from the qubit for single-qubit gates to instrument index.
     */
    utils::Map<Qubit, Instruments> single_qubit_instruments;

    /**
     * Map from the nth qubit of a two-qubit gate to instrument index.
     */
    utils::Map<Qubit, Instruments> two_qubit_instrument[2];

    /**
     * Map from the edge corresponding to a two-qubit gate to instrument
     * index.
     */
    utils::Map<Edge, Instruments> two_qubit_edge_instrument;

    /**
     * Map from the nth qubit of a three-or-more-qubit gate to instrument
     * index, with all qubit operands after the first two bunched together.
     */
    utils::Map<Qubit, Instruments> multi_qubit_instrument[3];

    /**
     * Defines the scheduling direction, if there is one. This controls whether
     * old reservations will be removed when a new reservation is added. For
     * forward scheduling, any reservations before the first cycle of the
     * incoming instruction removed; for backward scheduling, all reservations
     * after the last cycle of the incoming instruction are removed. This is
     * just a space-saving measure.
     */
    rmgr::Direction direction;

    /**
     * Cycle time of the platform.
     */
    utils::UInt cycle_time;

    /**
     * The (desugared) JSON configuration of this resource. Only retained for
     * dumping the configuration.
     */
    utils::Json json;

};

/**
 * Initializes this resource.
 */
void InstrumentResource::on_initialize(rmgr::Direction direction) {
#define ERROR(s) throw utils::Exception(utils::Str("instrument resource configuration error: ") + (s))

    // Create a new configuration structure.
    utils::Ptr<Config> cfg;
    cfg.emplace();

    // Set the easy stuff.
    cfg->direction = direction;
    cfg->cycle_time = context->platform->cycle_time;

    // Parse the JSON configuration.
    cfg->json = context->configuration;
    if (!cfg->json.is_object()) {
        ERROR("configuration must be a JSON object");
    }

    // Desugar the JSON structure for backward-compatibility with CC-light
    // custom resources.
    if (context->type_name == "arch.cc_light.qwgs") {

        // Handle QWG resource.
        cfg->json = R"(
        {
            "predicate": { "type": "mw" },
            "function": [ "cc_light_instr" ],
            "instruments": []
        }
        )"_json;

        auto it = context->configuration.find("connection_map");
        if (it == context->configuration.end()) {
            ERROR("missing connection_map key while desugaring QWG resource");
        } else if (!it->is_object()) {
            ERROR("QWG connection_map key must be an object");
        }
        for (auto it2 = it->begin(); it2 != it->end(); ++it2) {
            if (!it2->is_array()) {
                ERROR("QWG connection_map values must be arrays of qubit indices");
            }
            cfg->json["instruments"].push_back(
                {
                    {"name", "QWG" + it2.key()},
                    {"qubit", *it2}
                }
            );
        }

    } else if (context->type_name == "arch.cc_light.meas_units") {

        // Handle measurement unit resource.
        cfg->json = R"(
        {
            "predicate": { "type": "readout" },
            "instruments": []
        }
        )"_json;

        auto it = context->configuration.find("connection_map");
        if (it == context->configuration.end()) {
            ERROR("missing connection_map key while desugaring measurement unit resource");
        } else if (!it->is_object()) {
            ERROR("measurement unit connection_map key must be an object");
        }
        for (auto it2 = it->begin(); it2 != it->end(); ++it2) {
            if (!it2->is_array()) {
                ERROR("measurement unit connection_map values must be arrays of qubit indices");
            }
            cfg->json["instruments"].push_back(
                {
                    {"name", "MEAS" + it2.key()},
                    {"qubit", *it2}
                }
            );
        }

    } else if (context->type_name == "arch.cc_light.edges") {

        // Handle edge resource.
        cfg->json = R"(
        {
            "predicate": { "type": "flux" },
            "function": "exclusive",
            "instruments": []
        }
        )"_json;

        auto it = context->configuration.find("connection_map");
        if (it == context->configuration.end()) {
            ERROR("missing connection_map key while desugaring edge resource");
        } else if (!it->is_object()) {
            ERROR("edge connection_map key must be an object");
        }
        for (auto it2 = it->begin(); it2 != it->end(); ++it2) {
            if (!it2->is_array()) {
                ERROR("edge connection_map values must be arrays of edges");
            }
            utils::Json instrument_def = {
                {"name", "MEAS" + it2.key()},
                {"edge", *it2}
            };
            instrument_def["edge"].push_back(utils::parse_uint(it2.key()));
            cfg->json["instruments"].push_back(instrument_def);
        }

    } else if (context->type_name == "arch.cc_light.detuned_qubits") {

        // Handle detuned qubit resource.
        // Handle edge resource.
        cfg->json = R"(
        {
            "predicate_1q": { "type": "mw" },
            "predicate_2q": { "type": "flux" },
            "function": [ "type" ],
            "allow_overlap": true,
            "instruments": []
        }
        )"_json;

        auto it = context->configuration.find("connection_map");
        if (it == context->configuration.end()) {
            ERROR("missing connection_map key while desugaring edge resource");
        } else if (!it->is_object()) {
            ERROR("edge connection_map key must be an object");
        }
        utils::Map<utils::UInt, utils::Set<utils::UInt>> qubit_to_detuning_edges;
        for (auto it2 = it->begin(); it2 != it->end(); ++it2) {
            if (!it2->is_array()) {
                ERROR("edge connection_map values must be arrays of qubits");
            }
            if (it2->empty()) {
                continue;
            }
            auto edge_id = utils::parse_uint(it2.key());
            auto edge = context->platform->topology->get_edge_qubits(edge_id);
            if (edge == Edge(0, 0)) {
                ERROR(
                    "edge connection_map keys must be edges "
                    "(edge " + utils::to_string(edge_id) + " not found)"
                );
            }
            for (auto it3 = it2->begin(); it3 != it2->end(); ++it3) {
                if (!it3->is_number_unsigned()) {
                    ERROR("edge connection_map values must be arrays of qubits");
                }
                auto qubit = it3->get<utils::UInt>();
                qubit_to_detuning_edges.set(qubit).insert(edge_id);
            }
        }
        for (auto it2 : qubit_to_detuning_edges) {
            auto qubit = it2.first;
            const auto &edges = it2.second;
            cfg->json["instruments"].push_back(
                {
                    {"name", "qubit-" + utils::to_string(qubit)},
                    {"1q_qubit", {qubit}},
                    {"edge", edges}
                }
            );
        }
    }

    // Now actually parse our native structure.
    cfg->mutually_exclusive = false;
    cfg->allow_overlap = false;
    utils::RawPtr<utils::Json> instruments = nullptr;
    for (auto it = cfg->json.begin(); it != cfg->json.end(); ++it) {
        if (
            it.key() == "predicate"
            || it.key() == "predicate_1q"
            || it.key() == "predicate_2q"
            || it.key() == "predicate_nq"
        ) {
            if (!it->is_object()) {
                ERROR("predicate specification must be an object");
            }
            for (auto it2 = it->begin(); it2 != it->end(); ++it2) {
                utils::Set<utils::Str> preds;
                if (it2->is_array()) {
                    for (const auto &it3 : *it2) {
                        if (it3.is_string()) {
                            preds.insert(it3.get<utils::Str>());
                        } else {
                            ERROR("predicate value must be an array of strings or a single string");
                        }
                    }
                } else if (it2->is_string()) {
                    preds.insert(it2->get<utils::Str>());
                } else {
                    ERROR("predicate value must be an array of strings or a single string");
                }
                if (it.key() == "predicate_1q" || it.key() == "predicate") {
                    cfg->predicates[0].emplace_back(it2.key(), preds);
                }
                if (it.key() == "predicate_2q" || it.key() == "predicate") {
                    cfg->predicates[1].emplace_back(it2.key(), preds);
                }
                if (it.key() == "predicate_nq" || it.key() == "predicate") {
                    cfg->predicates[2].emplace_back(it2.key(), preds);
                }
            }
        } else if (it.key() == "function") {
            if (it->is_array()) {
                cfg->mutually_exclusive = false;
                for (const auto &it2 : *it) {
                    if (it2.is_string()) {
                        cfg->function_keys.push_back(it2.get<utils::Str>());
                    } else {
                        ERROR("function array values must be strings");
                    }
                }
            } else if (it->is_string() || it->get<utils::Str>() == "exclusive") {
                cfg->mutually_exclusive = true;
            } else {
                ERROR("function must be an array of strings or the string \"exclusive\"");
            }
        } else if (it.key() == "allow_overlap") {
            if (it->is_boolean()) {
                cfg->allow_overlap = it->get<utils::Bool>();
            } else {
                ERROR("allow_overlap must be boolean if specified");
            }
        } else if (it.key() == "instruments") {
            if (it->is_array()) {
                instruments = &*it;
            } else {
                ERROR("instruments must be an array of objects");
            }
        } else {
            ERROR("unknown key in configuration structure: " + it.key());
        }
    }
    if (!instruments) {
        ERROR("missing instruments key in configuration structure");
    }

    // Parse instruments substructure.
    for (const auto &instrument : *instruments) {
        if (!instrument.is_object()) {
            ERROR("instrument elements must be objects");
        }
        auto index = cfg->instrument_names.size();
        auto name = utils::to_string(index);
        for (auto it = instrument.begin(); it != instrument.end(); ++it) {
            if (it.key() == "name") {
                if (it->is_string()) {
                    name = it->get<utils::Str>();
                } else {
                    ERROR("instrument name must be a string");
                }
                continue;
            }
            if (!it->is_array()) {
                ERROR("all instrument keys except name must be arrays of integers");
            }
            utils::Set<utils::UInt> elements;
            for (const auto &it2 : *it) {
                if (it2.is_number_unsigned()) {
                    elements.insert(it2.get<utils::UInt>());
                } else {
                    ERROR("all instrument keys except name must be arrays of integers");
                }
            }
            utils::Bool recognized_as_qubits = false;
            if (it.key() == "1q_qubit" || it.key() == "qubit") {
                for (const auto &qubit : elements) {
                    cfg->single_qubit_instruments.set(qubit).push_back(index);
                }
                recognized_as_qubits = true;
            }
            if (it.key() == "2q_qubit0" || it.key() == "qubit") {
                for (const auto &qubit : elements) {
                    cfg->two_qubit_instrument[0].set(qubit).push_back(index);
                }
                recognized_as_qubits = true;
            }
            if (it.key() == "2q_qubit1" || it.key() == "qubit") {
                for (const auto &qubit : elements) {
                    cfg->two_qubit_instrument[1].set(qubit).push_back(index);
                }
                recognized_as_qubits = true;
            }
            if (it.key() == "nq_qubit0" || it.key() == "qubit") {
                for (const auto &qubit : elements) {
                    cfg->multi_qubit_instrument[0].set(qubit).push_back(index);
                }
                recognized_as_qubits = true;
            }
            if (it.key() == "nq_qubit1" || it.key() == "qubit") {
                for (const auto &qubit : elements) {
                    cfg->multi_qubit_instrument[1].set(qubit).push_back(index);
                }
                recognized_as_qubits = true;
            }
            if (it.key() == "nq_qubitn" || it.key() == "qubit") {
                for (const auto &qubit : elements) {
                    cfg->multi_qubit_instrument[2].set(qubit).push_back(index);
                }
                recognized_as_qubits = true;
            }
            if (recognized_as_qubits) {
                for (const auto &qubit : elements) {
                    if (qubit >= context->platform->qubit_count) {
                        ERROR(
                            "qubit index out of range in " + it.key() +
                            " list: " + utils::to_string(qubit)
                        );
                    }
                }
            } else if (it.key() == "edge") {
                for (const auto &edge_id : elements) {
                    auto edge = context->platform->topology->get_edge_qubits(edge_id);
                    if (edge == Edge(0, 0)) {
                        ERROR("invalid edge ID in edge list: " + utils::to_string(edge_id));
                    }
                    cfg->two_qubit_edge_instrument.set(edge).push_back(index);
                }
            }

        }
        cfg->instrument_names.push_back(name);
    }

    // Whew, what a mouthful. But now we're done.
    config = cfg;

    // Initialize state.
    state.clear();
    state.resize(cfg->instrument_names.size());

    // Print result if debug is enabled.
    QL_IF_LOG_DEBUG {
        std::cout << "====================================" << std::endl;
        std::cout << "resource instance " << context->instance_name;
        std::cout << " of type " << context->type_name << std::endl;
        std::cout << "------------------------------------" << std::endl;
        std::cout << "incoming JSON:" << std::endl;
        std::cout << context->configuration.dump(4) << std::endl;
        std::cout << "------------------------------------" << std::endl;
        std::cout << "desugared JSON:" << std::endl;
        std::cout << config->json.dump(4) << std::endl;
        std::cout << "------------------------------------" << std::endl;
        std::cout << "names: " << config->instrument_names << std::endl;
        std::cout << "preds[1q]: " << config->predicates[0] << std::endl;
        std::cout << "preds[2q]: " << config->predicates[1] << std::endl;
        std::cout << "preds[nq]: " << config->predicates[2] << std::endl;
        std::cout << "function: " << config->function_keys << std::endl;
        std::cout << "mutex: " << config->mutually_exclusive << std::endl;
        std::cout << "allow overlap: " << config->allow_overlap << std::endl;
        std::cout << "1q_instr: " << config->single_qubit_instruments << std::endl;
        std::cout << "2q_instr_q0: " << config->two_qubit_instrument[0] << std::endl;
        std::cout << "2q_instr_q1: " << config->two_qubit_instrument[1] << std::endl;
        std::cout << "2q_instr_edge: " << config->two_qubit_edge_instrument << std::endl;
        std::cout << "nq_instr_q0: " << config->multi_qubit_instrument[0] << std::endl;
        std::cout << "nq_instr_q1: " << config->multi_qubit_instrument[1] << std::endl;
        std::cout << "nq_instr_qn: " << config->multi_qubit_instrument[2] << std::endl;
        std::cout << "====================================" << std::endl;
    }
#undef ERROR

}

/**
 * Checks availability of and/or reserves a gate.
 */
utils::Bool InstrumentResource::on_gate(
    utils::UInt cycle,
    const ir::GateRef &gate,
    utils::Bool commit
) {
    QL_DOUT(
        "instrument resource " << context->instance_name
        << " got gate " << gate->qasm()
        << " for cycle " << cycle
        << " with commit set to " << commit
    );

    // We don't do anything with gates that don't have qubit operands.
    if (gate->operands.size() == 0) {
        QL_DOUT(" -> available: gate has no qubit operands");
        return true;
    }

    // Fetch the JSON data for this gate.
    const auto &gate_json = context->platform->find_instruction(gate->name);

    // Check predicates. If the gate doesn't match, we don't care about it, so
    // we can return true, such that it can be started in any cycle.
    auto op_count_pos = utils::min<utils::UInt>(gate->operands.size() - 1, 2);
    for (const auto &predicate : config->predicates[op_count_pos]) {
        auto it = gate_json.find(predicate.first);
        if (it == gate_json.end()) {
            QL_DOUT(
                " -> available: gate does not match predicate "
                << predicate.first << ": key does not exist"
            );
            return true;
        } else if (!it->is_string()) {
            QL_DOUT(
                " -> available: gate does not match predicate "
                << predicate.first << ": key is not a string"
            );
            return true;
        } else if (predicate.second.count(it->get<utils::Str>()) == 0) {
            QL_DOUT(
                " -> available: gate does not match predicate "
                << predicate.first << ": value " << it->get<utils::Str>()
                << " not in " << predicate.second
            );
            return true;
        }
    }

    // Check operands to see which instruments are affected.
    utils::Set<Instrument> affected;
    switch (gate->operands.size()) {
        case 1: {
            // Single-qubit gate.
            auto it = config->single_qubit_instruments.find(gate->operands[0]);
            if (it != config->single_qubit_instruments.end()) {
                affected.insert(it->second.begin(), it->second.end());
            }
            break;
        }
        case 2: {
            // Two-qubit gate.
            for (auto i = 0; i < 2; i++) {
                auto it = config->two_qubit_instrument[i].find(gate->operands[i]);
                if (it != config->two_qubit_instrument[i].end()) {
                    affected.insert(it->second.begin(), it->second.end());
                }
            }
            auto it = config->two_qubit_edge_instrument.find(
                Edge(gate->operands[0], gate->operands[1])
            );
            if (it != config->two_qubit_edge_instrument.end()) {
                affected.insert(it->second.begin(), it->second.end());
            }
            break;
        }
        default: {
            // Three-or-more-qubit gate.
            for (utils::UInt i = 0; i < gate->operands.size(); i++) {
                auto j = utils::min<utils::UInt>(i, 2);
                auto it = config->two_qubit_instrument[j].find(gate->operands[i]);
                if (it != config->two_qubit_instrument[j].end()) {
                    affected.insert(it->second.begin(), it->second.end());
                }
            }
            break;
        }
    }

    // If no instruments are affected, short-circuit here.
    if (affected.empty()) {
        QL_DOUT(" -> available: no instruments are affected");
        return true;
    }

    // Compute cycle range for this gate.
    State::Range range = {
        cycle,
        cycle + utils::div_ceil(gate->duration, config->cycle_time)
    };

    // If function is set to exclusive, just check/reserve the cycle range for
    // this gate for all affected instruments without caring about the function
    // value.
    Function function = 0;
    if (config->mutually_exclusive) {
        for (auto index : affected) {
            if (state[index].find(range).type != utils::RangeMatchType::NONE) {
                QL_DOUT(" -> not available because of instrument " << config->instrument_names[index]);
                return false;
            }
        }
    } else {

        // If not mutually exclusive, determine the function based on keys in
        // the gate's JSON.
        utils::Vec<utils::Str> function_key;
        function_key.resize(config->function_keys.size());
        for (utils::UInt i = 0; i < function_key.size(); i++) {
            auto it = gate_json.find(config->function_keys[i]);
            if (it != gate_json.end() && it->is_string()) {
                function_key[i] = it->get<utils::Str>();
            }
        }
        QL_DOUT("    function key = " << function_key);

        // Because storing vectors of strings in the resource state is a bit
        // ridiculous, we map these string tuples to unique integers. We just
        // generate a new integer whenever we see a function that we haven't
        // seen before. Note that this is fine even when resources are cloned
        // (remember: config is NOT cloned!) because we only ever add indices
        // here. Doing so doesn't affect the state. At worst, it may change
        // *future* indices added by other clones of this resource.
        auto it = config->function_map.find(function_key);
        if (it == config->function_map.end()) {
            function = config->function_map.size();
            config->function_map.set(function_key) = function;
        } else {
            function = it->second;
        }
        QL_DOUT("    function index = " << function);

        // Check the resources based on function index.
        for (auto index : affected) {
            QL_DOUT("    reservations for instrument " << config->instrument_names[index] << ":");
            QL_IF_LOG_DEBUG {
                state[index].dump_state(std::cout, "      ");
            }
            auto result = state[index].find(range);
            switch (result.type) {
                case utils::RangeMatchType::NONE:

                    // No overlap, cleared to place gate here for this
                    // instrument.
                    break;

                case utils::RangeMatchType::EXACT:

                    // Exact overlap; instrument is already in use, but the
                    // cycle range for its function overlaps exactly. If the
                    // current function is the same as the function required by
                    // the incoming gate, everything is fine. Otherwise, the
                    // gate can't go here.
                    if (result.begin->second != function) {
                        QL_DOUT(
                            " -> not available because of instrument "
                            << config->instrument_names[index]
                            << ", function mismatch"
                        );
                        return false;
                    }
                    break;

                default:

                    // Partial overlap; gate doesn't start synchronized with
                    // an existing gate for this instrument. If allow_overlap is
                    // not set, we already know we can't place it here.
                    if (!config->allow_overlap) {
                        QL_DOUT(
                            " -> not available because of instrument "
                            << config->instrument_names[index]
                            << ", overlapping wrong"
                        );
                        return false;
                    }

                    // If overlap is allowed, we have to check whether the
                    // function of all overlapping ranges matches.
                    for (auto it2 = result.begin; it2 != result.end; ++it2) {
                        if (it2->second != function) {
                            QL_DOUT(
                                " -> not available because of instrument "
                                << config->instrument_names[index]
                                << ", function mismatch in overlapping range"
                            );
                            return false;
                        }
                    }
                    break;

            }
        }

    }

    // If we get here, the gate can be placed. If commit is set, we also need
    // to reserve it.
    if (commit) {
        QL_DOUT(
            " -> gate is available, committing to "
            << affected.size() << " instruments"
        );
        for (auto index : affected) {
            if (config->direction == rmgr::Direction::FORWARD) {
                state[index].erase({ir::FIRST_CYCLE, range.first});
            } else if (config->direction == rmgr::Direction::BACKWARD) {
                state[index].erase({range.second, ir::MAX_CYCLE});
            }
            state[index].set(range, function);
        }
    } else {
        QL_DOUT(
            " -> gate is available, but not committing yet"
        );
    }

    // Gate can be placed.
    return true;
}

/**
 * Dumps documentation for this resource.
 */
void InstrumentResource::on_dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    if (context->type_name == "arch.cc_light.qwgs") {
        utils::dump_str(os, line_prefix, R"(
        Compatibility wrapper for the CC-light QWG resource. This is the same
        resource type as ``Instrument``, but accepting a different JSON
        configuration structure for backward compatibility. Refer to the
        documentation of the ``Instrument`` resource for more information;
        everything is explained there.
        )");
        return;
    } else if (context->type_name == "arch.cc_light.meas_units") {
        utils::dump_str(os, line_prefix, R"(
        Compatibility wrapper for the CC-light measurement unit resource. This
        is the same resource type as ``Instrument``, but accepting a different
        JSON configuration structure for backward compatibility. Refer to the
        documentation of the ``Instrument`` resource for more information;
        everything is explained there.
        )");
        return;
    } else if (context->type_name == "arch.cc_light.edges") {
        utils::dump_str(os, line_prefix, R"(
        Compatibility wrapper for the CC-light edge resource. This is the same
        resource type as ``Instrument``, but accepting a different JSON
        configuration structure for backward compatibility. Refer to the
        documentation of the ``Instrument`` resource for more information;
        everything is explained there.
        )");
        return;
    } else if (context->type_name == "arch.cc_light.detuned_qubits") {
        utils::dump_str(os, line_prefix, R"(
        Compatibility wrapper for the CC-light detuned qubit resource. This is
        the same resource type as ``Instrument``, but accepting a different JSON
        configuration structure for backward compatibility. Refer to the
        documentation of the ``Instrument`` resource for more information;
        everything is explained there.
        )");
        return;
    }
    utils::dump_str(os, line_prefix, R"(
    This resource models an instrument or group of instruments that is needed to
    apply a certain kind of quantum gate, with the constraint that the
    instrument is shared between a number of qubits/edges, and can only perform
    one function at a time. That is, two gates that share an instrument can be
    parallelized if and only if they use the same instrument function. By
    default, parallel gates requiring the same instrument also need to start at
    the same time and have the same duration, but this can be disabled.

    The instrument function is configurable for each particular gate based on
    one or more custom keys in the instruction set definition of the platform
    configuration file. It's also possible to specify that there is only a
    single function (i.e., all gates requiring access to the instrument can be
    parallelized, but only if they start at the same time and have the same
    duration), or to specify that all functions are mutually exclusive (in which
    case gates using the same instrument can never be parallelized).

    The instrument(s) affected by the gate, if any, are selected based on the
    qubit operands of the gate and upon whether the gate matches a set of
    predicates. Like the instrument function selection, the predicates are based
    on custom keys in the instruction definition in the platform configuration
    file. A different set of predicates can be provided based on the number of
    qubit operands of the gate.
)" /* in case anyone's wondering, MSVC has a string literal length limit */ R"(
    * Configuration structure *

      The shared instrument resource is configured using the following JSON
      structure.

      ```
      {
          "predicate": {
              "<gate-key>": ["<value>", ...],
              ...
          },
          "predicate_1q": ...,
          "predicate_2q": ...,
          "predicate_nq": ...,
          "function": [
              "<gate-key>",
              ...
          ],
          "allow_overlap": [true, false],
          "instruments": [
              {
                  "name": "<optional instrument name>",
                  "qubit":     [<qubits>],
                  "edge":      [<edges>],
                  "1q_qubit":  [<qubits>],
                  "2q_qubit0": [<qubits>],
                  "2q_qubit1": [<qubits>],
                  "nq_qubit0": [<qubits>],
                  "nq_qubit1": [<qubits>],
                  "nq_qubitn": [<qubits>]
              }
          ]
      }
      ```

      All sections except ``"instruments"`` are optional. Unrecognized sections
      throw an error.
)" R"(
      * Predicates *

        The predicate section must be a map of string-string or
        string-list(string) key-value pairs, representing (custom) keys and
        values in the instruction set definition section for the incoming gate
        that must be matched. An incoming gate matches the predicate if and only
        if:

         - its instruction set definition object has values for all keys
           specified;
         - these keys all map to strings; and
         - the string values match (one of) the specified value(s) for each key.

        For example, if an instruction definition looks like this:

        ```
        "x": {
            "duration": 40,
            "type": "mw",
            "instr": "x"
        }
        ```

        the following predicate configuration will match it:

        ```
        "predicate": {
            "type": "mw"
        }
        ```

        but will reject a gate defined like this:

        ```
        "cnot": {
            "duration": 80,
            "type": "flux",
            "instr": "cnot"
        }
        ```

        because its `"type"` is `"flux"`.

        NOTE: It will also silently reject gates which don't have the `"type"`
        key, so beware of typos!

        Should you want to match both types, but not any other type, you could
        do

        ```
        "predicate": {
            "type": ["mw", "flux"]
        }
        ```

        Different predicates can be specified for single-, two-, and
        more-than-two-qubit gates. Both the common predicate and the
        size-specific predicate must match.
)" R"(
      * Instrument function selection *

        The function section can be one of three things:

         - a list of gate keys as specified in the structure above, in which
           case the selected function is the combination of the (string) values
           of these keys in the gate definition;
         - an empty list or unspecified, in which case function matching is
           disabled, always allowing two gates to execute in parallel (but still
           requiring them to start and end simultaneously); or
         - the string ``"exclusive"``, in which case exclusive access is
           modelled, i.e. matching gates can never execute in parallel.

        For example, say that an `x` gate requires a different instrument
        function than an `y` gate, i.e. they cannot be done in parallel, but
        multiple `x` gates on different qubits can be parallelized (idem for
        `y`), you might use:

        ```
        "function": [
            "instr"
        ]
        ```

        for gates defined as follows:

        ```
        "x": {
            "duration": 40,
            "type": "mw",
            "instr": "x"
        },
        "y": {
            "duration": 40,
            "type": "mw",
            "instr": "y"
        }
        ```

        In some cases, it is not necessary for parallel operations requiring the
        same instrument function to actually start at the same time. For
        example, an instrument resource modelling qubit detuning would have two
        states, one indicating that the qubit is detuned, and one indicating
        that it is not, but as long as it is in one of these states, it doesn't
        matter when gates requiring it start and end. This behavior can be
        specified with the `"allow_overlap"` option. If specified, it must be a
        boolean, defaulting to `false`.

        NOTE: it makes little sense to combine `"allow_overlap"` with an empty
        `"function"` section, because this would disable all constraints on
        parallelism.
)" R"(
      * Instrument definition *

        A single instrument resource can define multiple independent instruments
        with the same behavior, distinguished by which qubits or edges they're
        connected to. This is done using the instruments section. It consists of
        a list of objects, where each object represents an instrument. The
        contents of the object define its connectivity, by way of predicates on
        the qubit operand list of the incoming gates.

         - For single-qubit gates, the instrument is used if and only if the
           single qubit operand is in the ``"1q_qubit"`` or ``"qubit"`` list.
         - For two-qubit gates, the instrument is used if any of the following
           is true:
            - either qubit is in the ``"qubit"`` list;
            - the first qubit operand is in the ``"2q_qubit0"`` list;
            - the second qubit operand is in the ``"2q_qubit1"`` list; or
            - the edge index (from the topology section of the platform)
              corresponding to the qubit operand pair is in the ``"edge"``
              list.
         - For three-or-more-qubit gates, the instrument is used if any of the
           following is true:
            - any of the qubit operands is in the ``"qubit"`` list;
            - the first qubit operand is in the ``"nq_qubit0"`` list;
            - the second qubit operand is in the ``"nq_qubit1"`` list; or
            - any of the remaining qubit operands are in the ``"nq_qubitn"``
              list.

        For example, an instrument defined as follows:

        ```
        "instruments": [
            {
                "name": "qubit-2",
                "1q_qubit": [2],
                "edge": [1, 9]
            }
        ]
        ```

        will be used by single-qubit gates acting on qubit 2, and by two-qubit
        gates acting on edge 1 or 9 (as defined in the topology section of the
        platform configuration file). Of course the gate also has to match the
        gate predicates defined for this resource for it to be considered. When
        a gate matches all of the above, the instrument function as defined in
        the function section will (need to) be reserved for this instrument for
        the duration of that gate, and thus the gate will be postponed if a
        conflicting reservation already exists.

)" R"(
    * QWG example *

      In CC-light, single-qubit rotation gates (instructions with
      ``"type": "mw"``) are controlled by QWGs. Each QWG controls a
      particular set of qubits. It can control multiple qubits at a time, but
      only when they perform the same gate (configured using
      ``"cc_light_instr"``) and start at the same time. There are three QWGs,
      the first of which is connected to qubits 0 and 1, the second one to
      qubits 2, 3, and 4, and the third to qubits 5 and 6.

      This can be modelled with the following configuration:

      ```
      {
          "predicate": { "type": "mw" },
          "function": [ "cc_light_instr" ],
          "instruments": [
              {
                  "name": "QWG0",
                  "qubit": [0, 1]
              },
              {
                  "name": "QWG1",
                  "qubit": [2, 3, 4]
              },
              {
                  "name": "QWG2",
                  "qubit": [5, 6]
              }
          ]
      }
      ```

      Before resources were generalized, QWGs were modelled with a specific
      resource type. Its configuration structure instead looked like this for
      the same thing:

      ```
      {
          "count": 3,
          "connection_map": {
              "0" : [0, 1],
              "1" : [2, 3, 4],
              "2" : [5, 6]
          }
      }
      ```

      For backward compatibility, this structure is desugared to the complete
      structure when the instrument resource is constructed using
      ``arch.cc_light.qwgs``.
)" R"(
    * Measurement unit example *

      In CC-light, single-qubit measurements (instructions with
      ``"type": "readout"``) are controlled by measurement units.
      Each one controls a private set of qubits. A measurement unit can control
      multiple qubits at the same time, but only when they start at the same
      time. There were two measurement units, the first of which being connected
      to qubits 0, 2, 4, 5, and 6, and the second to qubits 1 and 4.

      This can be modelled with the following configuration:

      ```
      {
          "predicate": { "type": "readout" },
          "instruments": [
              {
                  "name": "MEAS0",
                  "qubit": [0, 2, 4, 5, 6]
              },
              {
                  "name": "MEAS1",
                  "qubit": [1, 4]
              }
          ]
      }
      ```

      Before resources were generalized, this was modelled with a specific
      resource type. Its configuration structure instead looked like this for
      the same thing:

      ```
      {
          "count": 2,
          "connection_map": {
              "0" : [0, 2, 3, 5, 6],
              "1" : [1, 4]
          }
      }
      ```

      For backward compatibility, this structure is desugared to the complete
      structure when the instrument resource is constructed using
      ``arch.cc_light.meas_units``.
)" R"(
    * Qubit detuning example *

      In CC-light, two-qubit flux gates (``"type": "flux"``) for
      particular edges require a third qubit to be detuned (also known as
      parked). This moves the qubit out of the way frequency-wise to prevent it
      from being affected by the flux gate as well. However, doing so prevents
      single-qubit microwave quantum gates (``"type": "mw"``) from
      using the detuned qubit (and vice versa). Specifically:

       - a two-qubit gate operating on qubits 0 and 2 detunes qubit 3;
       - a two-qubit gate operating on qubits 0 and 3 detunes qubit 2;
       - a two-qubit gate operating on qubits 1 and 3 detunes qubit 4;
       - a two-qubit gate operating on qubits 1 and 4 detunes qubit 3;
       - a two-qubit gate operating on qubits 3 and 5 detunes qubit 6; and
       - a two-qubit gate operating on qubits 3 and 6 detunes qubit 5.

      This can be modelled with the following configuration:

      ```
      {
          "predicate_1q": { "type": "mw" },
          "predicate_2q": { "type": "flux" },
          "function": [ "type" ],
          "allow_overlap": true,
          "instruments": [
              {
                  "name": "qubit-2",
                  "1q_qubit": [2],
                  "edge": [1, 9]
              },
              {
                  "name": "qubit-3",
                  "1q_qubit": [3],
                  "edge": [0, 3, 8, 11]
              },
              {
                  "name": "qubit-4",
                  "1q_qubit": [4],
                  "edge": [2, 10]
              },
              {
                  "name": "qubit-5",
                  "1q_qubit": [5],
                  "edge": [6, 14]
              },
              {
                  "name": "qubit-6",
                  "1q_qubit": [6],
                  "edge": [5, 13]
              }
          ]
      }
      ```
)" R"(
      Before resources were generalized, this was modelled with a specific
      resource type. Its configuration structure instead looked like this for
      the same thing:

      ```
      {
          "count": 7,
          "connection_map": {
              "0": [3],
              "1": [2],
              "2": [4],
              "3": [3],
              "4": [],
              "5": [6],
              "6": [5],
              "7": [],
              "8": [3],
              "9": [2],
              "10": [4],
              "11": [3],
              "12": [],
              "13": [6],
              "14": [5],
              "15": []
          }
      }
      ```

      To be specific, the connection map mapped each edge index to a list of
      qubits detuned by doing a flux gate on that edge. For backward
      compatibility, this structure is desugared to the complete structure when
      the instrument resource is constructed using
      ``arch.cc_light.detuned_qubits``.
)" R"(
    * Mutably-exclusive edge example *

      The above qubit detuning has additional implications for two-qubit gates.
      Specifically, the following edges are mutably exclusive:

       - q0-q2 (edge 0 and 8) is mutually-exclusive with q1-q3 (edge 2 and 10);
       - q0-q3 (edge 1 and 9) is mutually-exclusive with q1-q4 (edge 3 and 11);
       - q2-q5 (edge 4 and 12) is mutually-exclusive with q3-q6 (edge 6 and 14);
         and
       - q3-q5 (edge 5 and 13) is mutually-exclusive with q4-q6 (edge 7 and 15).

      ```
      {
          "predicate": { "type": "flux" },
          "function": "exclusive",
          "instruments": [
              {
                  "name": "edge-0_2-1_3",
                  "edge": [0, 2, 8, 10]
              },
              {
                  "name": "edge-0_3-1_4",
                  "edge": [1, 3, 9, 11]
              },
              {
                  "name": "edge-2_5-3_6",
                  "edge": [4, 6, 12, 14]
              },
              {
                  "name": "edge-3_5-4_6",
                  "edge": [5, 7, 13, 15]
              }
          ]
      }
      ```
)" R"(
      Before resources were generalized, this was modelled with a specific
      resource type. Its configuration structure instead looked like this for
      the same thing:

      ```
      {
          "count": 16,
          "connection_map": {
              "0": [2, 10],
              "1": [3, 11],
              "2": [0, 8],
              "3": [1, 9],
              "4": [6, 14],
              "5": [7, 15],
              "6": [4, 12],
              "7": [5, 13],
              "8": [2, 10],
              "9": [3, 11],
              "10": [0, 8],
              "11": [1, 9],
              "12": [6, 14],
              "13": [7, 15],
              "14": [4, 12],
              "15": [5, 13]
          }
      }
      ```

      Here, the keys and values in ``"connection_map"`` specify edge indices,
      where usage of the edge in key indicates that the edge indices it maps to
      can no longer be used. Note that the i+8 edge is not in the second list
      because it maps to the inversed-direction edge, which is already excluded
      by mutual exclusion of the qubits themselves. For backward compatibility,
      this structure is desugared to the complete structure when the instrument
      resource is constructed using ``arch.cc_light.edges``.
    )");
}

/**
 * Dumps the configuration of this resource.
 */
void InstrumentResource::on_dump_config(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    if (state.empty()) {
        os << line_prefix << "Not yet initialized" << std::endl;
        return;
    }
    utils::dump_str(os, line_prefix, config->json.dump(4));
    os << std::endl;
}

/**
 * Dumps the state of this resource.
 */
void InstrumentResource::on_dump_state(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    if (state.empty()) {
        os << line_prefix << "Not yet initialized" << std::endl;
        return;
    }
    for (utils::UInt i = 0; i < state.size(); i++) {
        os << line_prefix << "Instrument " << config->instrument_names[i] << ":\n";
        state[i].dump_state(
            os,
            line_prefix + "  ",
            [this](std::ostream &os, const utils::UInt &val) {
                if (this->config->mutually_exclusive) {
                    os << "reserved";
                } else {
                    for (const auto &it : this->config->function_map) {
                        if (val == it.second) {
                            os << it.first;
                            return;
                        }
                    }
                    os << "INVALID";
                }
            }
        );
    }
    os.flush();
}

/**
 * Constructs the resource. No error checking here; this is up to the
 * resource manager.
 */
InstrumentResource::InstrumentResource(
    const rmgr::Context &context
) :
    rmgr::resource_types::Base(context)
{ }

/**
 * Returns a user-friendly type name for this resource.
 */
utils::Str InstrumentResource::get_friendly_type() const {
    if (context->type_name == "arch.cc_light.qwgs") {
        return "CC-light qwgs resource";
    } else if (context->type_name == "arch.cc_light.meas_units") {
        return "CC-light meas_units resource";
    } else if (context->type_name == "arch.cc_light.edges") {
        return "CC-light edges resource";
    } else if (context->type_name == "arch.cc_light.detuned_qubits") {
        return "CC-light detuned_qubits resource";
    }
    return "Instrument resource";
}

} // namespace instrument
} // namespace resource
} // namespace ql
