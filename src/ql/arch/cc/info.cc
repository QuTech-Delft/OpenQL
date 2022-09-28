/** \file
 * Defines information about the CC architecture.
 */

#include "ql/arch/cc/info.h"

#include <iomanip>
#include "ql/com/options.h"
#include "ql/arch/cc/pass/gen/vq1asm/detail/options.h"
#include "ql/arch/cc/pass/gen/vq1asm/detail/settings.h"
#include "ql/arch/cc/resources/hwconf_default.inc"

namespace ql {
namespace arch {
namespace cc {

bool Info::is_architecture_registered = Factory::register_architecture<Info>();

// local constants
const utils::Str predicateKeyInstructionType = "cc-desugar-instruction-type";
const utils::Str predicateValueMeas = "cc-desugar-meas";
const utils::Str predicateValueFlux = "cc-desugar-flux";


/**
 * Writes the documentation for this architecture to the given output
 * stream.
 */
void Info::dump_docs(std::ostream &os, const utils::Str &line_prefix) const {
    // NOTE JvS -> Wouter: indentation and structure of these doc dumps is
    // significant. Please read the section on runtime documentation and dump()
    // functions in CONTRIBUTING.md or the developer documentation section
    // in the Sphinx/ReadTheDocs documentation (generated from it) before
    // modifying this.

    // FIXME: contents are outdated
    utils::dump_str(os, line_prefix, R"(
    This architecture allows compilation for the QuTech Central Controller, as
    currently in use in DiCarloLab for the Starmon chip.

    * Platform configuration file additions *

      For the CC backend, contrary to the original one for CC-light, the final
      hardware output is *entirely determined* by the contents of the
      configuration file. That is, there is no built-in knowledge of instrument
      connectivity or codeword organization. This requires a few additional
      target-specific sections in the platform configuration.

      * Instrument definitions *

        The CC-specific `"instrument_definitions"` section of the configuration
        file defines immutable properties of instruments, i.e. independent of
        the actual control setup. The required structure is as follows:

        ```javascript
        "instrument_definitions": {
            "qutech-qwg": {
                "channels": 4,
                "control_group_sizes": [1, 4],
            },
            "zi-hdawg": {
                "channels": 8,
                // NB: size=1 needs special treatment of waveforms because one
                // AWG unit drives 2 channels
                "control_group_sizes": [1, 2, 4, 8],
            },
            "qutech-vsm": {
                "channels": 32,
                "control_group_sizes": [1],
            },
            "zi-uhfqa": {
                "channels": 9,
                "control_group_sizes": [1],
            }
        }
        ```
)" R"(
        In this:

         - `"channels"` defines the number of logical channels of the
           instrument. For most instruments there is one logical channel per
           physical channel, but the 'zi-uhfqa' provides 9 logical channels on
           one physical channel pair.
         - `"control_group_sizes"` states possible arrangements of channels
           operating as a vector.

      * Control modes *

        The `"control_modes"` section defines modes to control instruments.
        These define which bits are used to control groups of channels
        and/or get back measurement results. The expected structure is as
        follows:
)" R"(
        ```javascript
        "control_modes": {
            "awg8-mw-vsm-hack": {                     // ZI_HDAWG8.py::cfg_codeword_protocol() == 'microwave'. Old hack to skip DIO[8]
                "control_bits": [
                    [7,6,5,4,3,2,1,0],                // group 0
                    [16,15,14,13,12,11,10,9]          // group 1
                ],
                "trigger_bits": [31]
            },
            "awg8-mw-vsm": {                          // the way the mode above should have been
                "control_bits": [
                    [7,6,5,4,3,2,1,0],                // group 0
                    [15,14,13,12,11,10,9,8]           // group 1
                ],
                "trigger_bits": [31]
            },
            "awg8-mw-direct-iq": {                    // just I&Q to generate microwave without VSM. HDAWG8: "new_novsm_microwave"
                "control_bits": [
                    [6,5,4,3,2,1,0],                  // group 0
                    [13,12,11,10,9,8,7],              // group 1
                    [22,21,20,19,18,17,16],           // group 2. NB: starts at bit 16 so twin-QWG can also support it
                    [29,28,27,26,25,24,23]            // group 4
                ],
                "trigger_bits": [15,31]
            },
            "awg8-flux": {                             // ZI_HDAWG8.py::cfg_codeword_protocol() == 'flux'
                // NB: please note that internally one AWG unit handles 2 channels, which requires special handling of the waveforms
                "control_bits": [
                    [2,1,0],                          // group 0
                    [5,4,3],
                    [8,7,6],
                    [11,10,9],
                    [18,17,16],                       // group 4. NB: starts at bit 16 so twin-QWG can also support it
                    [21,20,19],
                    [24,23,22],
                    [27,26,25]                        // group 7
                ],
                "trigger_bits": [31]
            },
            "awg8-flux-vector-8": {                    // single code word for 8 flux channels.
                "control_bits": [
                    [7,6,5,4,3,2,1,0]
                ],
                "trigger_bits": [31]
            },
            "uhfqa-9ch": {
                "control_bits": [[17],[18],[19],[20],[21],[22],[23],[24],[25]],    // group[0:8]
                "trigger_bits": [16],
                "result_bits": [[1],[2],[3],[4],[5],[6],[7],[8],[9]],              // group[0:8]
                "data_valid_bits": [0]
            },
            "vsm-32ch":{
                "control_bits": [
                    [0],[1],[2],[3],[4],[5],[6],[7],                      // group[0:7]
                    [8],[9],[10],[11],[12],[13],[14],[15],                // group[8:15]
                    [16],[17],[18],[19],[20],[21],[22],[23],              // group[16:23]
                    [24],[25],[26],[27],[28],[28],[30],[31]               // group[24:31]
                ],
                "trigger_bits": []                                       // no trigger
            }
        }
        ```
)" R"(
        In this:

         - `<key>` is a name which can be referred to from key
           `instruments/[]/ref_control_mode`.
         - `"control_bits"` defines G groups of B bits, where:
            - G determines which `instrument_definitions/<key>/control_group_sizes`
              is used; and
            - B is an ordered list of bits (MSB to LSB) used for the code word.
         - `"trigger_bits"` must be a vector of bits, used to trigger the
           instrument. Must either be size 1 (common trigger) or size G (separate
           trigger per group), or 2 (common trigger duplicated on 2 bits, to
           support dual-QWG).
         - `"result_bits"` is reserved for future use.
         - `"data_valid_bits"` is reserved for future use.


      * Signals *

        The `"signals"` section provides a signal library that gate definitions
        can refer to. The expected structure is as follows:
)" R"(
        ```javascript
        "signals": {
            "single-qubit-mw": [
                {   "type": "mw",
                    "operand_idx": 0,
                    "value": [
                        "{gateName}-{instrumentName}:{instrumentGroup}-gi",  // FIXME: outdated
                        "{gateName}-{instrumentName}:{instrumentGroup}-gq",
                        "{gateName}-{instrumentName}:{instrumentGroup}-di",
                        "{gateName}-{instrumentName}:{instrumentGroup}-dq"
                    ]
                },
                {   "type": "switch",
                    "operand_idx": 0,
                    "value": ["dummy"]                                  // NB: no actual signal is generated
                }
            ],
            "two-qubit-flux": [
                {   "type": "flux",
                    "operand_idx": 0,                                   // control
                    "value": ["flux-0-{qubit}"]
                },
                {   "type": "flux",
                    "operand_idx": 1,                                   // target
                    "value": ["flux-1-{qubit}"]
                }
            ]
        }
        ```
)" R"(
        In this, the toplevel object key is a name which can be referred to
        from key `instructions/<>/cc/ref_signal`. It defines an array of
        records with the fields below:

         - `"type"` defines a signal type. This is used to select an instrument
           that provides that signal type through key ``instruments/*/signal_type``.
           The types are entirely user defined, there is no builtin notion of
           their meaning.
         - `"operand_idx"` states the operand index of the instruction/gate
           this signal refers to. Signals must be defined for all `operand_idx`
           the gate refers to, so a 3-qubit gate needs to define 0 through 2.
           Several signals with the same operand_idx can be defined to select
           several signal types, as shown in "single-qubit-mw" which has both
           "mw" (provided by an AWG) and "switch" (provided by a VSM).
         - `"value"` defines the signal name. The CC backend itself makes no
           assumptions on this other that different instructions have different
           signal names.
    )"

    // FIXME:
    //  - rewrite field 'operand_idx
    //  - describe the (future) use of field 'value'
    //  - expand

    R"(
      * Instruments *

        The `"instruments"` section defines instruments used in this setup,
        their configuration and connectivity. The expected structure is as
        follows:

        ```javascript
        "instruments": [
            // readout.
            {
                "name": "ro_0",
                "qubits": [[6], [11], [], [], [], [], [], [], []],
                "signal_type": "measure",
                "ref_instrument_definition": "zi-uhfqa",
                "ref_control_mode": "uhfqa-9ch",
                "controller": {
                    "name": "cc",
                    "slot": 0,
                    "io_module": "CC-CONN-DIO"
                }
            },
            // ...

            // microwave.
            {
                "name": "mw_0",
                "qubits": [                                             // data qubits:
                    [2, 8, 14],                                         // [freq L]
                    [1, 4, 6, 10, 12, 15]                               // [freq H]
                ],
                "signal_type": "mw",
                "ref_instrument_definition": "zi-hdawg",
                "ref_control_mode": "awg8-mw-vsm-hack",
                "controller": {
                    "name": "cc",
                    "slot": 3,
                    "io_module": "CC-CONN-DIO-DIFF"
                }
            },
            // ...
)" R"(
            // VSM
            {
                "name": "vsm_0",
                "qubits": [
                    [2], [8], [14], [],  [], [], [], [],                // [freq L]
                    [1], [4], [6], [10], [12], [15], [], [],            // [freq H]
                    [0], [5], [9], [13], [], [], [], [],                // [freq Mg]
                    [3], [7], [11], [16], [], [], [], []                // [freq My]
                ],
                "signal_type": "switch",
                "ref_instrument_definition": "qutech-vsm",
                "ref_control_mode": "vsm-32ch",
                "controller": {
                    "name": "cc",
                    "slot": 5,
                    "io_module": "cc-conn-vsm"
                }
            },

            // flux
            {
                "name": "flux_0",
                "qubits": [[0], [1], [2], [3], [4], [5], [6], [7]],
                "signal_type": "flux",
                "ref_instrument_definition": "zi-hdawg",
                "ref_control_mode": "awg8-flux",
                "controller": {
                    "name": "cc",
                    "slot": 6,
                    "io_module": "CC-CONN-DIO-DIFF"
                }
            },
            // ...
        ]
        ```
)" R"(
        In this:

         - `"name"` is a friendly name for the instrument.
         - `"ref_instrument_definition"` selects a record under
           `"instrument_definitions"`, which must exist or an error is raised.
         - `"ref_control_mode"` selects a record under `"control_modes"`, which
           must exist or an error is raised.
         - `"signal_type"` defines which signal type this instrument instance
           provides.
         - `"qubits"` G groups of 1 or more qubits. G must match one of the
           available group sizes of `instrument_definitions/<ref_instrument_definition>/control_group_sizes`.
           If more than 1 qubits are stated per group - e.g. for an AWG used in
           conjunction with a VSM - they may not produce conflicting signals at
           any time slot, or an error is raised.
         - `"force_cond_gates_on"` is optional, reserved for future use.
         - `"controller/slot"` is the slot number of the CC this instrument is
           connected to.
         - `"controller/name"` is reserved for future use.
         - `"controller/io_module"` is reserved for future use.

    )"

    // FIXME: describe matching process of 'signal_type' against 'signals/*/type'

    R"(
      * Additional instruction data *

        The CC backend extends the instruction definitions with a `"cc"`
        subsection, as shown in the example below:

        ```javascript
        "ry180": {
            "duration": 20,
            "cc": {
                "ref_signal": "single-qubit-mw",
                "static_codeword_override": [2]
            }
        },
        "cz_park": {
            "duration": 40,
            "cc": {
                "signal": [
                    {   "type": "flux",
                        "operand_idx": 0,                                   // control
                        "value": ["flux-0-{qubit}"]
                    },
                    {   "type": "flux",
                        "operand_idx": 1,                                   // target
                        "value": ["flux-1-{qubit}"]
                    },
                    {   "type": "flux",
                        "operand_idx": 2,                                   // park
                        "value": ["park_cz-{qubit}"]
                    }
                ],
                "static_codeword_override": [1,2,3]
            }
        }
        "_wait_uhfqa": {
            "duration": 220,
            "cc": {
                "signal": []
            }
        },)" R"(
        "_dist_dsm": {
            "duration": 20,
            "cc": {
                "readout_mode": "feedback",
                "signal": [
                    {	"type": "measure",
                        "operand_idx": 0,
                        "value": []
                    }
                ]
            }
        },
        "_wait_dsm": {
            "duration": 80,
            "cc": {
                "signal": []
            }
        }
        ```
)" R"(
        In this:

         - `"ref_signal"` points to a signal definition in
           `hardware_settings/eqasm_backend_cc/signals`, which must exist or
           an error is raised.
         - `"signal"` defines a signal in place, in an identical fashion as
           `hardware_settings/eqasm_backend_cc/signals`. May be empty (`[]`)
           to disable signal generation.
         - `"static_codeword_override"` provides a user defined array of
           codeword (one entry per operand) for this instruction. Currently,
           this key is compulsory (if signal is non-empty), but in the future,
           codewords will be assigned automatically to make better use of the
           limited codeword space.
         - if `"readout_mode"` with value "feedback" is used, code is generated to
           read and distribute the instrument result.
)" R"(
    * Program flow feedback *

      To support Repeat Until Success type experiments, a special field
      were added to the gate definition for the CC, as shown in the previous
      section:

       - the `"readout_mode": "feedback"` clause in the `"_dist_dsm"` gate
         causes the backend to generate code to retrieve the measurement result
         from the DIO interface and distribute it across the CC.

      For convenience, the gate decomposition section can be extended with
      `"measure_fb %0": ["measure %0", "_wait_uhfqa %0", "_dist_dsm %0", "_wait_dsm %0"]`

      This creates a `measure_fb` instruction consisting of four parts:

       - triggering a measurement (on the UHFQA);
       - waiting for the internal processing time of the UHFQA;
       - retrieve the measurement result, and distribute it across the CC; and
       - wait for the data distribution to finish.
)"
R"(
      Caveats:

       - It is not possible to mix `measure_fb` and `measure` in a single
         program. This is a consequence of the way measurements are read from
         the input DIO interface of the CC: every measurement (both from
         `measure_fb` and `measure`) is pushed onto an input FIFO. This FIFO is
         only popped by a `measure_fb` instruction. If the two types are mixed,
         misalignment occurs between what is written and read. No check is
         currently performed by the backend.

      These limitations will vanish when integration with cQASM 2.0 is
      completed.

    * Code generation pass *

      Most of the magic for all of the above happens in the code generation
      pass, `arch.cc.VQ1Asm`. This generates the following file types:

       - `.vq1asm`: 'Vectored Q1 assembly' file for the Central Controller.
       - `.vcd`: timing file, can be viewed using GTKWave
         (http://gtkwave.sourceforge.net).

      For configuration options, please refer to the documentation of this
      pass.

    )");
}

/**
 * Returns a user-friendly type name for this architecture. Used for
 * documentation generation.
 */
utils::Str Info::get_friendly_name() const {
    return "QuTech Central Controller";
}

/**
 * Returns the name of the namespace for this architecture.
 */
utils::Str Info::get_namespace_name() const {
    return "cc";
}

/**
 * Returns a list of strings accepted for the "eqasm_compiler" key in the
 * platform configuration file. This can be more than one, to support both
 * legacy (inconsistent) names and the new namespace names. The returned
 * set must include at least the name of the namespace.
 */
utils::List<utils::Str> Info::get_eqasm_compiler_names() const {
    return {"cc", "eqasm_backend_cc"};
}

/**
 * Should generate a sane default platform JSON file for the given variant
 * of this architecture. This JSON data will still be preprocessed by
 * preprocess_platform().
 */
utils::Str Info::get_default_platform(const utils::Str &variant) const {

    // NOTE: based on tests/cc/cc_s5_direct_iq.json at the time of writing.
    return HWCONF_DEFAULT_DATA;

}

/**
 * Preprocessing logic for the platform JSON configuration file. May be used
 * to generate/expand certain things that are always the same for that
 * platform, to save typing in the configuration file (and reduce the amount
 * of mistakes made).
 */
void Info::preprocess_platform(utils::Json &data, const utils::Str &variant) const {

    QL_IOUT("desugaring CC instructions");

    // load CC settings
    pass::gen::vq1asm::detail::Settings settings;
    settings.loadBackendSettings(data);

    // get instruction section, since we don't have Platform yet at this stage
    QL_JSON_ASSERT(data, "instructions", "/");
    utils::Json &instructions = data["instructions"];

    // add predicates to instructions (for instrument resources created in post_process_platform())
    for (auto &it : instructions.items()) {
        if (settings.isMeasure(it.value(), it.key())) {
            QL_IOUT("desugaring measure instruction: '" << it.key() << "'");
            instructions[it.key()][predicateKeyInstructionType] = predicateValueMeas;
        }

        // (note that both isMeasure() and isFlux() can be true)
        if (settings.isFlux(it.value(), it.key())) {
            QL_IOUT("desugaring flux instruction: '" << it.key() << "'");
            instructions[it.key()][predicateKeyInstructionType] = predicateValueFlux;
        }
    }

    QL_IOUT("finished desugaring CC instructions");
}


// local type shorthands
using Qubits = utils::Vec<utils::UInt>;         // array of qubits
using InstrVsQubits = utils::Vec<Qubits>;       // instrument versus qubits

/**
 *
 */
static utils::Json buildInstrumentResource(
    const utils::Str &name,
    const InstrVsQubits &instrVsQubits,
    const utils::Str &predicateValue
) {
    utils::Json ext_resource;      // extended resource definition, see https://openql.readthedocs.io/en/latest/gen/reference_resources.html

    utils::Json instrConfig = R"(
    {
        "instruments": []
    }
    )"_json;

    instrConfig["predicate"] = {{ predicateKeyInstructionType, predicateValue }};

    for(utils::UInt i=0; i<instrVsQubits.size(); i++) {
        auto &qubits = instrVsQubits[i];
        instrConfig["instruments"].push_back(
            {
                {"name", QL_SS2S(name<<i) },
                {"qubit", qubits }
            }
        );
    }

    ext_resource["type"] = "Instrument";
    ext_resource["config"] = instrConfig;
    return ext_resource;
}

/*
 * FIXME: add doc
 */
static Qubits ccInstrument2qubits(const utils::Json &instrument) {
    Qubits ret;

    auto qubits = utils::json_get<const utils::Json &>(instrument, "qubits");
    utils::UInt qubitGroupCnt = qubits.size();  // NB: JSON key qubits is a 'matrix' of [groups*qubits]
    for (utils::UInt group=0; group<qubitGroupCnt; group++) {
        const utils::Json &qubitsOfGroup = qubits[group];
        for (utils::UInt i=0; i<qubitsOfGroup.size(); i++) {
            ret.push_back(qubitsOfGroup[0].get<utils::UInt>());
        }
    }
    return ret;
}

/**
 * Post-processing logic for the Platform data structure. This may for
 * instance add annotations with architecture-specific configuration data.
 */
void Info::post_process_platform(
    const ir::compat::PlatformRef &platform,
    const utils::Str &variant
) const {

    QL_IOUT("post_process_platform, variant='" << variant << "'");

    // Desugaring similar to ql/resource/instrument.cc, but independent of resource keys being present
    QL_IOUT("desugaring CC instrument resource");

    // load CC settings
    pass::gen::vq1asm::detail::Settings settings;
    settings.loadBackendSettings(platform);

#if 0   // FIXME: untested, counterpart in Codegen::init does not seem to work
    // TODO Wouter: something with vq1asm::detail::Settings, I guess. Note: to
    //  keep track of the structure alongside the platform, you can add it as
    //  an annotation like so:
    //  platform->set_annotation(pass::gen::vq1asm::detail::Settings());
    //  platform->get_annotation<pass::gen::vq1asm::detail::Settings>().loadBackendSettings(platform);

    platform->set_annotation(pass::gen::vq1asm::detail::Settings());
    platform->get_annotation<pass::gen::vq1asm::detail::Settings>().loadBackendSettings(platform);
#endif

    // Gather qubit information from CC instrument definitions.
    const utils::Str instrumentSignalTypeMeasure = settings.getInstrumentSignalTypeMeasure();
    const utils::Str instrumentSignalTypeFlux = settings.getInstrumentSignalTypeFlux();
    InstrVsQubits measQubits;
    InstrVsQubits fluxQubits{Qubits{}};     // one empty vector
    for (utils::UInt i=0; i<settings.getInstrumentsSize(); i++) {
        const utils::Json &instrument = settings.getInstrumentAtIdx(i);
        utils::Str instrumentSignalType = utils::json_get<utils::Str>(instrument, "signal_type");    // FIXME: nodePath
        if (instrumentSignalType == instrumentSignalTypeMeasure) {
            Qubits qubits = ccInstrument2qubits(instrument);
            measQubits.push_back(qubits);
        } else if (instrumentSignalType == instrumentSignalTypeFlux) {
            /*  we map all fluxing on a single instrument resource: the actual resource we'd like to manage is a *signal* that connects
                to a flux line of a qubit. On a single instrument (e.g. ZI HDAWG) these signals cannot be triggered
                during playback of other signals.
                Note however, that a single "flux" gate may trigger signals on different instruments. During scheduling,
                we don't know about these signals yet though, and we also don't particularly care about keeping the
                instruments independent where possible.
                Also note that all of this breaks down if/when we want to support stuff like "flux assisted measurement",
                where the flux line is manipulated during a measurement, and we can no longer tie types like "flux" and
                "measurement" to a gate, but must really work with the signals.
            */
            Qubits qubits = ccInstrument2qubits(instrument);
            for(auto q : qubits) {
                fluxQubits[0].push_back(q);
            }
        }
    }

    // Create Instrument resources from gathered information
    platform->resources["resources"]["meas"] = buildInstrumentResource("meas", measQubits, predicateValueMeas);
    platform->resources["resources"]["flux"] = buildInstrumentResource("flux", fluxQubits, predicateValueFlux);
    QL_DOUT("created resources:\n" << std::setw(4) << platform->resources);

    // NB: we get the Qubit resource for free, independent of JSON contents (see ql/rmgr/factory.cc and QubitResource::on_initialize)
}

/**
 * Adds the default "backend passes" for this platform. Called by
 * pmgr::Manager::from_defaults() when no compiler configuration file is
 * specified. This typically includes at least the architecture-specific
 * code generation pass, but anything after prescheduling and optimization
 * is considered a backend pass.
 */
void Info::populate_backend_passes(pmgr::Manager &manager, const utils::Str &variant) const {
    // Remove prescheduler if enabled implicitly (pointless since we add our own scheduling).
    // FIXME: bit of a hack, and invalidates https://openql.readthedocs.io/en/latest/gen/reference_architectures.html#default-pass-list
    utils::Str ps_name = "prescheduler";
    const auto &prescheduler = com::options::global[ps_name];
    if (!prescheduler.is_set()) {               // prescheduler enabled implicitly
        if (manager.does_pass_exist(ps_name)) {
            manager.remove_pass(ps_name);
        }
    }

    // Add our passes, and amend scheduledqasmwriter to show our scheduling, not that of prescheduler.
    manager.append_pass(
        "sch.ListSchedule",
        "scheduler",
        {
#if OPT_CC_SCHEDULE_RC
            {"resource_constraints", "yes"}
#else
            {"resource_constraints", "no"}
#endif
        }
    );
    if (manager.does_pass_exist("scheduledqasmwriter")) {
        auto options = manager.get_pass("scheduledqasmwriter")->get_options();
        manager.remove_pass("scheduledqasmwriter");     // confusing, since it doesn't use our scheduling
        manager.append_pass("io.cqasm.Report", "scheduledqasmwriter")->get_options().update_from(options);
    }
    manager.append_pass(
        "arch.cc.gen.VQ1Asm",
        "VQ1Asm",
        {
            {"output_prefix", com::options::global["output_dir"].as_str() + "/%N"}
        }
    );

}

} // namespace cc
} // namespace arch
} // namespace ql
