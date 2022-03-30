/** \file
 * Defines the channel resource. This limits the amount of parallel
 * communication between cores in a multi-core system.
 */

#include "ql/resource/inter_core_channel.h"

// uncomment next line to enable multi-line dumping
// #define MULTI_LINE_LOG_DEBUG

namespace ql {
namespace resource {
namespace inter_core_channel {

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
     * Gate predicates. The array index is the number of qubit operands
     * minus one, clamped to 2 maximum (so three-or-more-qubit gates are
     * bunched together).
     */
    Predicates predicates[3];

    /**
     * Whether a gate must use qubits belonging to at least two different cores
     * to match the predicate.
     */
    utils::Bool inter_core_required;

    /**
     * Whether all qubit operands or only communication qubit operands require
     * communication channel resources.
     */
    utils::Bool communication_qubit_only;

    /**
     * Number of cores.
     */
    utils::UInt num_cores;

    /**
     * Limit on number of incoming/outgoing channels for each core.
     */
    utils::UInt num_channels;

    /**
     * Limit on number of incoming/outgoing channels system-wide.
     */
    utils::UInt num_system_wide_channels;

    /**
     * When set, there is a defined scheduling direction, which means it's
     * sufficient to only track the latest reservation for each qubit.
     */
    utils::Bool optimize;

    /**
     * The (desugared) JSON configuration of this resource. Only retained for
     * dumping the configuration.
     */
    utils::Json json;

};

/**
 * Initializes this resource.
 */
void InterCoreChannelResource::on_initialize(rmgr::Direction direction) {
#define ERROR(s) throw utils::Exception(utils::Str("channel resource configuration error: ") + (s))

    // Create a new configuration structure.
    utils::Ptr<Config> cfg;
    cfg.emplace();

    // Set the easy stuff.
    cfg->num_cores = context->platform->topology->get_num_cores();
    cfg->optimize = direction != rmgr::Direction::UNDEFINED;

    // Parse the JSON configuration.
    cfg->json = context->configuration;
    if (!cfg->json.is_object()) {
        ERROR("configuration must be a JSON object");
    }

    // Desugar the JSON structure for backward-compatibility with CC-light
    // custom resources.
    if (context->type_name == "arch.cc_light.channels") {
        cfg->json = R"(
        {
            "predicate": { "type": "extern" },
            "inter_core_required": true,
            "communication_qubit_only": false,
            "num_channels": 1,
            "num_system_wide_channels": 0
        }
        )"_json;
        auto it = context->configuration.find("count");
        if (it != context->configuration.end()) {
            if (!it->is_number_unsigned()) {
                ERROR("count key must be an unsigned integer if specified");
            } else {
                cfg->json["num_channels"] = *it;
            }
        }
        auto it2 = context->configuration.find("system_wide_count");
        if (it2 != context->configuration.end()) {
            if (!it2->is_number_unsigned()) {
                ERROR("system_wide_count key must be an unsigned integer if specified");
            } else {
                cfg->json["num_system_wide_channels"] = *it2;
            }
        } else {
            // upperbound for backward compatibility without this field specified
            cfg->json["num_system_wide_channels"] = context->platform->qubit_count;
        }
    }

    // Now parse our native structure.
    cfg->inter_core_required = false;
    cfg->communication_qubit_only = false;
    cfg->num_channels = 1;
    cfg->num_system_wide_channels = context->platform->qubit_count;
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
        } else if (it.key() == "inter_core_required") {
            if (it->is_boolean()) {
                cfg->inter_core_required = it->get<utils::Bool>();
            } else {
                ERROR("inter_core_required must be a boolean if specified");
            }
        } else if (it.key() == "communication_qubit_only") {
            if (it->is_boolean()) {
                cfg->communication_qubit_only = it->get<utils::Bool>();
            } else {
                ERROR("communication_qubit_only must be a boolean if specified");
            }
        } else if (it.key() == "num_channels") {
            if (it->is_number_unsigned()) {
                cfg->num_channels = it->get<utils::UInt>();
            } else {
                ERROR("num_channels must be an unsigned integer if specified");
            }
        } else if (it.key() == "num_system_wide_channels") {
            if (it->is_number_unsigned()) {
                cfg->num_system_wide_channels = it->get<utils::UInt>();
            } else {
                ERROR("num_system_wide_channels must be an unsigned integer if specified");
            }
        } else {
            ERROR("unknown key in configuration structure: " + it.key());
        }
    }
    if (cfg->num_channels < 1) {
        ERROR("illegal number of communication channels per core, need at least one per core");
    }
    if (cfg->num_system_wide_channels < 2) {
        ERROR("illegal number of communication channels system-wide, need at least two to communicate");
    }

    // Configuration load successful; save the constructed configuration.
    config = cfg;

    // Initialize state.
    state.clear();
    state.resize(cfg->num_cores);
    for (auto &s : state) {
        s.resize(cfg->num_channels);
    }

    // Print result if debug is enabled.
#ifdef MULTI_LINE_LOG_DEBUG
    QL_IF_LOG_DEBUG {
        QL_DOUT("Print result of initializing inter-core channel resource: ");
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
        std::cout << "preds[1q]: " << config->predicates[0] << std::endl;
        std::cout << "preds[2q]: " << config->predicates[1] << std::endl;
        std::cout << "preds[nq]: " << config->predicates[2] << std::endl;
        std::cout << "inter_core_required: " << config->inter_core_required << std::endl;
        std::cout << "communication_qubit_only: " << config->communication_qubit_only << std::endl;
        std::cout << "num_cores: " << config->num_cores << std::endl;
        std::cout << "num_channels: " << config->num_channels << std::endl;
        std::cout << "num_system_wide_channels: " << config->num_system_wide_channels << std::endl;
        std::cout << "====================================" << std::endl;
    }
#else
    QL_DOUT("Print result of initializing inter-core channel resource (disabled)");
#endif

#undef ERROR
}

/**
 * Checks availability of and/or reserves a gate.
 */
utils::Bool InterCoreChannelResource::on_gate(
    utils::Int cycle,
    const rmgr::resource_types::GateData &gate,
    utils::Bool commit
) {
    QL_DOUT(
        "channel resource " << context->instance_name
        << " got gate with name " << gate.name
        << " and qubit operands " << gate.qubits
        << " for cycle " << cycle
        << " with commit set to " << commit
    );

    const auto &grid = *context->platform->topology;

    // We don't do anything with gates that don't have qubit operands.
    if (gate.qubits.empty()) {
        QL_DOUT(" -> available: gate has no qubit operands");
        return true;
    }

    // Fetch the JSON data for this gate.
    const auto &gate_json = *gate.data;

    // Check predicates. If the gate doesn't match, we don't care about it, so
    // we can return true, such that it can be started in any cycle.
    auto op_count_pos = utils::min<utils::UInt>(gate.qubits.size() - 1, 2);
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

    // Figure out which cores are affected.
    utils::Set<utils::UInt> affected;
    for (auto qubit : gate.qubits) {
        if (!config->communication_qubit_only || grid.is_comm_qubit(qubit)) {
            affected.insert(grid.get_core_index(qubit));
        }
    }

    // If inter_core_required, check whether the gate uses qubits from more than
    // one core.
    if (config->inter_core_required && gate.qubits.size() >= 2 && affected.size() < 2) {
        QL_DOUT(" -> available: gate does not match inter-core predicate");
        return true;
    }

    // If we get here, all relevant predicates have matched, so this resource must be acquired.
    // When acquisition fails, return false.
    
    // Compute cycle range for this gate.
    State::Range range = {
        cycle,
        cycle + gate.duration_cycles
    };

    // Check availability wrt number of channels per core.
    for (auto core : affected) {
        utils::Bool core_available = false;
        for (auto &s : state[core]) {
            if (s.find(range).type == utils::RangeMatchType::NONE) {
                core_available = true;
                break;
            }
        }
        if (!core_available) {
            QL_DOUT(" -> not available because core " << core << " number of channels is saturated");
            return false;
        }
    }

    // Check availability wrt number of channels system-wide
    utils::UInt num_channels_in_use = 0;
    for (auto &core : state) {
        for (auto &channel : core) {
            if (channel.find(range).type != utils::RangeMatchType::NONE) {
                num_channels_in_use++;
            }
        }
    }
    if (num_channels_in_use + gate.qubits.size() > config->num_system_wide_channels) {
        QL_DOUT(" -> not available because system-wide number of channels is saturated");
        return false;
    }

    // If we get here, the gate can be placed. If commit is set, we also need
    // to reserve it.
    if (commit) {
        QL_DOUT(
            " -> gate is available, committing to I/O channels for "
            << affected.size() << " cores"
        );
        for (auto core : affected) {
            utils::Bool core_found = false;
            for (auto &s : state[core]) {
                if (s.find(range).type == utils::RangeMatchType::NONE) {
                    if (config->optimize) {
                        s.clear();
                    }
                    s.set(range);
                    core_found = true;
                    break;
                }
            }
            QL_ASSERT(core_found);
        }
    } else {
        QL_DOUT(
            " -> gate is available, but not committing yet"
        );
    }

    return true;
}

/**
 * Dumps documentation for this resource.
 */
void InterCoreChannelResource::on_dump_docs(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    if (context->type_name == "arch.cc_light.channels") {
        utils::dump_str(os, line_prefix, R"(
        Compatibility wrapper for the CC-light channels resource. This does
        exactly the same thing as the InterCoreChannel resource, but accepts the
        following configuration structure, with ``"system_wide_count"`` optional:

        ```
        {
            "count": <number of channels per core>
            "system_wide_count": <number of channels system-wide>
        }
        ```

        This structure is converted to the following for use with the
        InterCoreChannel resource:

        ```
        {
            "predicate": { "type": "extern" },
            "inter_core_required": false,
            "communication_qubit_only": false,
            "num_channels": <taken from "count">,
            "num_system_wide_channels": <taken from "system_wide_count">
        }
        ```
        In absence of ``"system_wide_count"``, ``"num_system_wide_channels"``.
        defaults to the number of qubits in the platform.
        )");
        return;
    }

    utils::dump_str(os, line_prefix, R"(
    This resource models inter-core communication with limited connectivity
    between cores. This is modelled as follows.

    Each core has a limited number of channels, with which it can connect
    to other cores. The channels are drawn from a system-wide pool of
    limited size.  The connectivity between the channels of each core is
    assumed to be fully connected, but the number of channels per core can
    be adjusted, as can the size of the system-wide pool. Gates matching
    the predicate (if any) use one of the available channels for each
    of the gate's operand qubits.  The applicable core per qubit is of
    course determined by the qubit index, but the channel is undefined;
    the resource will use the first available channel.

    The resource is configured using the following structure.

    ```
    {
        "predicate": {
            "<gate-key>": ["<value>", ...],
            ...
        },
        "predicate_1q": ...,
        "predicate_2q": ...,
        "predicate_nq": ...,
        "inter_core_required": <boolean, default true>,
        "communication_qubit_only": <boolean, default false>,
        "num_channels": <number of channels per core, default 1>,
        "num_system_wide_channels": <number of channels system-wide,
            default the number of qubits in the platform>,
    }
    ```
)" R"(
    Note that the number of cores and communication qubits per core are
    configured in the topology section of the platform JSON configuration data.
    These settings are not repeated here.

    The predicate section must be a map of string-string or
    string-list(string) key-value pairs, representing (custom) keys and values
    in the instruction set definition section for the incoming gate that must
    be matched. An incoming gate matches the predicate if and only if:

     - its instruction set definition object has values for all keys
       specified;
     - these keys all map to strings; and
     - the string values match (one of) the specified value(s) for each key.

    Different predicates can be specified for single-, two-, and
    more-than-two-qubit gates. Both the common predicate and the size-specific
    predicate must match.

    Furthermore, gates can be predicated based on whether they actually use
    qubits from multiple cores. This is controlled by ``"inter_core_required"``.
    When set or unspecified, a gate must operate on qubits belonging to at least
    two different cores to match the predicate. Otherwise, this is not required.
    That is, a gate matching the other predicates that uses only qubits from one
    core would still use channel resources for that core. This is mostly for
    compatibility with the original channel resource, which didn't check for
    this.

    The ``"communication_qubit_only"`` flag controls whether all qubits of a
    gate use communication channel resources, or whether only qubits marked as
    communication qubits are considered.

    The ``"num_channels"`` key specifies how many independent channels
    each core has. The default and minimum value is 1.

    Finally, the ``"num_system_wide_channels"`` key specifies how large
    the pool of channels in the system is from which the core channels
    draw.  The default value is the platform's number of qubits. This
    conforms to absence of this constraint.

    )");
}

/**
 * Dumps the configuration of this resource.
 */
void InterCoreChannelResource::on_dump_config(
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
void InterCoreChannelResource::on_dump_state(
    std::ostream &os,
    const utils::Str &line_prefix
) const {
    if (state.empty()) {
        os << line_prefix << "Not yet initialized" << std::endl;
        return;
    }
    for (utils::UInt core = 0; core < state.size(); core++) {
        os << line_prefix << "Core " << core << ":\n";
        const auto &core_state = state[core];
        for (utils::UInt channel = 0; channel < core_state.size(); channel++) {
            os << line_prefix << "  Channel " << channel << ":\n";
            core_state[channel].dump_state(os, line_prefix + "    ");
        }
    }
    os.flush();
}

/**
 * Constructs the resource. No error checking here; this is up to the
 * resource manager.
 */
InterCoreChannelResource::InterCoreChannelResource(
    const rmgr::Context &context
) :
    rmgr::resource_types::Base(context)
{ }

/**
 * Returns a user-friendly type name for this resource.
 */
utils::Str InterCoreChannelResource::get_friendly_type() const {
    if (context->type_name == "arch.cc_light.channels") {
        return "CC-light channels resource";
    }
    return "Multi-core channel resource";
}

} // namespace inter_core_channel
} // namespace resource
} // namespace ql
