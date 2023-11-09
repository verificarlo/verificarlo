
#ifndef VERIFICARLO_LIBVFCINSTRUMENTPRISM_TARGET_FEATURES_HPP
#define VERIFICARLO_LIBVFCINSTRUMENTPRISM_TARGET_FEATURES_HPP

#include <set>
#include <sstream>

#include <llvm/IR/Attributes.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>

#include "libVFCInstrumentPRISMOptions.hpp"

using namespace llvm;

const auto target_features_debug =
    VfclibInstDebugOptions.isSet(debugOptions::TargetFeatures);

const auto target_features_debug_abi =
    VfclibInstDebug and VfclibInstDebugOptions.isSet(debugOptions::ABI);

// Taken from
// https://www.fluentcpp.com/2017/04/21/how-to-split-a-string-in-c/
static auto split(const std::string &s,
                  char delimiter) -> std::vector<std::string> {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(s);
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

// template <typename S>
// static std::vector<std::string> split(const S s, char delimiter) {
//   return split(s.str(), delimiter);
// }

using X86_64_ISA = enum {
  x86_64_NONE = 0,
  x86_64_SSE = 1,
  x86_64_SSE2 = 2,
  x86_64_SSE3 = 3,
  x86_64_SSE4 = 4,
  x86_64_AVX = 5,
  x86_64_AVX2 = 6,
  x86_64_AVX512F = 7
};

auto getIsaAsStringX86_64(X86_64_ISA isa) -> std::string {
  switch (isa) {
  case x86_64_NONE:
    return "";
  case x86_64_SSE:
    return "sse";
  case x86_64_SSE2:
    return "sse2";
  case x86_64_SSE3:
    return "sse3";
  case x86_64_SSE4:
    return "sse4";
  case x86_64_AVX:
    return "avx";
  case x86_64_AVX2:
    return "avx2";
  case x86_64_AVX512F:
    return "avx512f";
  default:
    return "";
  }
}

class TargetFeature {
public:
  TargetFeature() = default;
  explicit TargetFeature(const std::string &feature) {
    if (feature[0] == '-') {
      enabled = false;
      name = feature.substr(1);
    } else if (feature[0] == '+') {
      enabled = true;
      name = feature.substr(1);
    } else {
      enabled = true;
      name = feature;
    }
  }

  [[nodiscard]] auto getAsString() const -> std::string {
    if (enabled) {
      return "+" + name;
    }
    return "-" + name;
  }

  auto operator<(const TargetFeature &rhs) const -> bool {
    const auto repr = getAsString();
    const auto rhs_repr = rhs.getAsString();
    return repr < rhs_repr;
  }

  auto operator==(const TargetFeature &rhs) const -> bool {
    return (name == rhs.name) and (enabled == rhs.enabled);
  }

  friend auto operator<<(llvm::raw_ostream &OS,
                         const TargetFeature &TF) -> llvm::raw_ostream & {
    OS << TF.getAsString();
    return OS;
  }

private:
  bool enabled{};
  std::string name;
};

class TargetFeatures {

public:
  TargetFeatures() = default;
  explicit TargetFeatures(const std::string &features) {
    std::vector<std::string> features_list = split(features, ',');
    if (target_features_debug) {
      errs() << "Build TargetFeatures from string: " << features << "\n";
      errs() << "Features list: ";
      for (const auto &feature : features_list) {
        errs() << feature << " ";
      }
      errs() << "\n";
    }
    for (const auto &feature : features_list) {
      addAttribute(TargetFeature(feature));
    }
    if (target_features_debug) {
      errs() << "TargetFeatures built: " << getAsString() << "\n";
    }
  }

  explicit TargetFeatures(const Attribute &features) {
    if (features.isStringAttribute()) {
      const auto features_str = features.getValueAsString();
      std::vector<std::string> features_list = split(features_str.str(), ',');
      if (target_features_debug) {
        errs() << "Build TargetFeatures from Attribute: " << features_str
               << "\n";
        errs() << "Features list: ";
        for (const auto &feature : features_list) {
          errs() << feature << " ";
        }
        errs() << "\n";
      }
      for (const auto &feature : features_list) {
        addAttribute(TargetFeature(feature));
      }
    }
    if (target_features_debug) {
      errs() << "TargetFeatures built: " << getAsString() << "\n";
    }
  }

  void addAttribute(const std::string &feature) {
    if (target_features_debug) {
      errs() << "addAttribute string: " << feature << "\n";
    }
    if (this->hasAttribute(feature)) {
      if (target_features_debug) {
        errs() << "Attribute already present\n";
      }
      return;
    }
    addAttribute(TargetFeature(feature));
  }

  void addAttribute(const TargetFeature &feature) {
    if (target_features_debug) {
      errs() << "addAttribute TargetFeature: " << feature << "\n";
      if (features.find(feature) != features.end()) {
        errs() << "Attribute already present\n";
      }
    }
    features.insert(feature);
  }

  [[nodiscard]] auto hasAttribute(const TargetFeature &feature) const -> bool {
    return features.find(feature) != features.end();
  }

  [[nodiscard]] auto hasAttribute(const std::string &feature) const -> bool {
    return hasAttribute(TargetFeature(feature));
  }

  [[nodiscard]] auto getAsString() const -> std::string {
    std::string str;
    for (const auto &feature : features) {
      str += feature.getAsString() + ",";
    }
    if (not str.empty()) {
      str.pop_back();
    }
    return str;
  }

  // Attribute getAttributes() {
  //   std::string str = getAsString();
  //   return Attribute::get(M.getContext(), str);
  // }

  auto isIncludeIn(const TargetFeatures &rhs) -> bool {
    return std::all_of(features.begin(), features.end(),
                       [&](const TargetFeature &feature) {
                         return rhs.hasAttribute(feature);
                       });
  }

  [[nodiscard]] auto empty() const -> bool { return features.empty(); }

  auto operator+(const TargetFeatures &rhs) -> TargetFeatures {
    TargetFeatures result;
    for (const auto &feature : features) {
      result.addAttribute(feature);
    }
    for (const auto &feature : rhs.features) {
      result.addAttribute(feature);
    }
    return result;
  }

  auto operator-(const TargetFeatures &rhs) -> TargetFeatures {
    TargetFeatures result;
    for (const auto &feature : features) {
      result.addAttribute(feature);
    }
    for (const auto &feature : rhs.features) {
      result.features.erase(feature);
    }
    return result;
  }

  auto operator|(const TargetFeatures &rhs) -> TargetFeatures {
    TargetFeatures result;
    for (const auto &feature : features) {
      result.addAttribute(feature);
    }
    for (const auto &feature : rhs.features) {
      result.addAttribute(feature);
    }
    return result;
  }

  auto operator&(const TargetFeatures &rhs) -> TargetFeatures {
    TargetFeatures result;
    for (const auto &feature : features) {
      if (rhs.hasAttribute(feature)) {
        result.addAttribute(feature);
      }
    }
    return result;
  }

  friend auto operator<<(llvm::raw_ostream &OS,
                         const TargetFeatures &TF) -> llvm::raw_ostream & {
    OS << TF.getAsString();
    return OS;
  }

private:
  std::set<TargetFeature> features;
};

inline auto
getHighestSupportedISA_X86_64(const TargetFeatures &features) -> X86_64_ISA {
  if (features.hasAttribute("avx512f")) {
    return x86_64_AVX512F;
  }
  if (features.hasAttribute("avx2")) {
    return x86_64_AVX2;
  }
  if (features.hasAttribute("avx")) {
    return x86_64_AVX;
  }
  if (features.hasAttribute("sse4.2")) {
    return x86_64_SSE4;
  }
  if (features.hasAttribute("sse3")) {
    return x86_64_SSE3;
  }
  if (features.hasAttribute("sse2")) {
    return x86_64_SSE2;
  }
  if (features.hasAttribute("sse")) {
    return x86_64_SSE;
  }
  return x86_64_NONE;
}

inline auto hasFeatures_X86_64(const Attribute &src,
                               const Attribute &target) -> bool {
  std::vector<std::string> features = {"avx512f", "avx2", "avx", "sse4.2",
                                       "sse3",    "sse2", "sse"};
  for (const auto &feature : features) {
    if (src.hasAttribute(feature) and not target.hasAttribute(feature)) {
      if (VfclibInstDebug) {
        errs() << "Missing feature: " << feature << "\n";
      }
      return false;
    }
  }
  return true;
}

inline auto areABICompatible(Instruction *CallInst, Function *Callee) -> bool {
  auto *caller = CallInst->getParent()->getParent();
  auto calleeTargetFeatures =
      TargetFeatures(Callee->getFnAttribute("target-features"));
  auto callerTargetFeatures =
      TargetFeatures(caller->getFnAttribute("target-features"));

  if (callerTargetFeatures.empty()) {
    // The case when using flang-7 as frontend
    // TODO(yohan): add a flag to enable/disable this
    return true;
  }

  // if caller does have the callee features, we're good
  if (calleeTargetFeatures.isIncludeIn(callerTargetFeatures)) {
    if (target_features_debug_abi) {
      errs() << "Caller has the callee target features\n";
      errs() << "caller features: " << callerTargetFeatures.getAsString()
             << "\n";
      errs() << "callee features: " << calleeTargetFeatures.getAsString()
             << "\n";
    }
    return true;
  }

  // if caller does not have the callee features, we need to check if there
  // is compatibility
  auto caller_highest_isa_supported =
      getHighestSupportedISA_X86_64(callerTargetFeatures);
  auto callee_highest_isa_supported =
      getHighestSupportedISA_X86_64(calleeTargetFeatures);
  if (caller_highest_isa_supported >= callee_highest_isa_supported) {
    if (target_features_debug_abi) {
      errs() << "Caller is compatible with callee target features\n";
      errs() << "caller features: " << callerTargetFeatures.getAsString()
             << "\n";
      errs() << "callee features: " << calleeTargetFeatures.getAsString()
             << "\n";
    }
    return true;
  }

  if (target_features_debug_abi) {
    errs() << "Caller is not compatible with callee target features\n";
    errs() << "caller features: " << callerTargetFeatures.getAsString() << "\n";
    errs() << "callee features: " << calleeTargetFeatures.getAsString() << "\n";
  }
  return false;
}

#endif // VERIFICARLO_LIBVFCINSTRUMENTPRISM_TARGET_FEATURES_HPP