#ifndef VERIFICARLO_LIBVFCINSTRUMENTPRISM_OPTIONS_HPP
#define VERIFICARLO_LIBVFCINSTRUMENTPRISM_OPTIONS_HPP

#include <llvm/Support/CommandLine.h>

using namespace llvm;

namespace options {

class RoundingMode {
private:
  enum class PrismRoundingMode { StochasticRounding, UpDown };
  PrismRoundingMode rounding_mode;
  std::string up_down_mode = "up-down";
  std::string up_down_ns = "ud";
  std::string stochastic_rounding_mode = "stochastic-rounding";
  std::string stochastic_rounding_ns = "sr";

public:
  explicit RoundingMode(const std::string &mode) {
    if (mode == up_down_mode) {
      rounding_mode = PrismRoundingMode::UpDown;
    } else if (mode == stochastic_rounding_mode) {
      rounding_mode = PrismRoundingMode::StochasticRounding;
    } else {
      errs() << "Invalid mode: " << mode << "\n";
      report_fatal_error("libVFCInstrumentPRISM fatal error");
    }
  }

  [[nodiscard]] auto get_namespace() const {
    if (rounding_mode == PrismRoundingMode::UpDown) {
      return up_down_ns;
    }
    if (rounding_mode == PrismRoundingMode::StochasticRounding) {
      return stochastic_rounding_ns;
    }
    errs() << "Invalid rounding mode\n";
    report_fatal_error("libVFCInstrumentPRISM fatal error");
  }
};

class DispatchMode {
private:
  enum class PrismDispatchMode { Static, Dynamic };
  PrismDispatchMode dispatch_mode;
  std::string static_mode = "static";
  std::string dynamic_mode = "dynamic";

public:
  explicit DispatchMode(const std::string &mode) {
    if (mode == static_mode) {
      dispatch_mode = PrismDispatchMode::Static;
    } else if (mode == dynamic_mode) {
      dispatch_mode = PrismDispatchMode::Dynamic;
    } else {
      errs() << "Invalid dispatch: " << mode << "\n";
      report_fatal_error("libVFCInstrumentPRISM fatal error");
    }
  }

  [[nodiscard]] auto get_namespace() const {
    if (dispatch_mode == PrismDispatchMode::Static) {
      return static_mode + "_dispatch";
    }
    if (dispatch_mode == PrismDispatchMode::Dynamic) {
      return dynamic_mode + "_dispatch";
    }
    errs() << "Invalid dispatch mode\n";
    report_fatal_error("libVFCInstrumentPRISM fatal error");
  }

  [[nodiscard]] auto is_static() const -> bool {
    return dispatch_mode == PrismDispatchMode::Static;
  }

  [[nodiscard]] auto is_dynamic() const -> bool {
    return dispatch_mode == PrismDispatchMode::Dynamic;
  }
};

} // namespace options

enum class debugOptions {
  GetOperands = 1U << 0U,
  TargetFeatures = 1U << 1U,
  ABI = 1U << 2U
};

static cl::opt<std::string>
    VfclibInstFunction("vfclibinst-function",
                       cl::desc("Only instrument given FunctionName"),
                       cl::value_desc("FunctionName"), cl::init(""));

static cl::opt<std::string> VfclibInstIncludeFile(
    "vfclibinst-include-file",
    cl::desc("Only instrument modules / functions in file IncludeNameFile "),
    cl::value_desc("IncludeNameFile"), cl::init(""));

static cl::opt<std::string> VfclibInstExcludeFile(
    "vfclibinst-exclude-file",
    cl::desc("Do not instrument modules / functions in file ExcludeNameFile "),
    cl::value_desc("ExcludeNameFile"), cl::init(""));

static cl::opt<std::string> VfclibInstDynamicIRFile(
    "vfclibinst-prism-dynamic-ir-file",
    cl::desc("Name of the IR file that contains the dynamic operators"),
    cl::value_desc("SRIRFile"), cl::init(""));

static cl::opt<std::string> VfclibInstStaticIRFile(
    "vfclibinst-prism-static-ir-file",
    cl::desc("Name of the IR file that contains the static operators"),
    cl::value_desc("SRIRFile"), cl::init(""));

static cl::opt<bool> VfclibInstVerbose("vfclibinst-verbose",
                                       cl::desc("Activate verbose mode"),
                                       cl::value_desc("Verbose"),
                                       cl::init(false));

static cl::opt<std::string> VfclibInstMode(
    "vfclibinst-mode",
    cl::desc("Instrumentation mode: up-down or stochastic-rounding"),
    cl::value_desc("Mode"));

static cl::opt<std::string>
    VfclibInstDispatch("vfclibinst-dispatch",
                       cl::desc("Instrumentation dispatch: static or dynamic"),
                       cl::value_desc("Dispatch"));

static cl::opt<bool>
    VfclibInstInstrumentFMA("vfclibinst-inst-fma",
                            cl::desc("Instrument floating point fma"),
                            cl::value_desc("InstrumentFMA"), cl::init(false));

static cl::opt<bool> VfclibInstDebug("vfclibinst-debug",
                                     cl::desc("Activate debug mode"),
                                     cl::value_desc("Debug"), cl::init(false));

static cl::bits<debugOptions> VfclibInstDebugOptions(
    cl::desc("Debug options"),
    cl::values(
        clEnumValN(debugOptions::GetOperands, "vfclibinst-debug-getoperands",
                   "Debug GetOperands"),
        clEnumValN(debugOptions::TargetFeatures,
                   "vfclibinst-debug-targetfeatures", "Debug TargetFeatures"),
        clEnumValN(debugOptions::ABI, "vfclibinst-debug-abi", "Debug ABI")),
    cl::CommaSeparated);

#endif // VERIFICARLO_LIBVFCINSTRUMENTPRISM_OPTIONS_HPP
