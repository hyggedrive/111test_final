//=- AArch64Processors.td - Describe AArch64 Processors ------*- tablegen -*-=//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// AArch64 Processor subtarget features.
//===----------------------------------------------------------------------===//


def TuneA35     : SubtargetFeature<"a35", "ARMProcFamily", "CortexA35",
                                "Cortex-A35 ARM processors">;

def TuneA53     : SubtargetFeature<"a53", "ARMProcFamily", "CortexA53",
                                   "Cortex-A53 ARM processors", [
                                   FeatureFuseAES,
                                   FeatureFuseAdrpAdd,
                                   FeatureBalanceFPOps,
                                   FeatureCustomCheapAsMoveHandling,
                                   FeaturePostRAScheduler]>;

def TuneA55     : SubtargetFeature<"a55", "ARMProcFamily", "CortexA55",
                                   "Cortex-A55 ARM processors", [
                                   FeatureFuseAES,
                                   FeatureFuseAdrpAdd,
                                   FeaturePostRAScheduler,
                                   FeatureFuseAddress]>;

def TuneA510    : SubtargetFeature<"a510", "ARMProcFamily", "CortexA510",
                                   "Cortex-A510 ARM processors", [
                                   FeatureFuseAES,
                                   FeatureFuseAdrpAdd,
                                   FeaturePostRAScheduler
                                   ]>;

def TuneA57     : SubtargetFeature<"a57", "ARMProcFamily", "CortexA57",
                                   "Cortex-A57 ARM processors", [
                                   FeatureFuseAES,
                                   FeatureBalanceFPOps,
                                   FeatureCustomCheapAsMoveHandling,
                                   FeatureFuseAdrpAdd,
                                   FeatureFuseLiterals,
                                   FeaturePostRAScheduler,
                                   FeatureEnableSelectOptimize,
                                   FeaturePredictableSelectIsExpensive]>;

def TuneA65     : SubtargetFeature<"a65", "ARMProcFamily", "CortexA65",
                                   "Cortex-A65 ARM processors", [
                                   FeatureFuseAES,
                                   FeatureFuseAddress,
                                   FeatureFuseAdrpAdd,
                                   FeatureFuseLiterals,
                                   FeatureEnableSelectOptimize,
                                   FeaturePredictableSelectIsExpensive]>;

def TuneA72     : SubtargetFeature<"a72", "ARMProcFamily", "CortexA72",
                                   "Cortex-A72 ARM processors", [
                                   FeatureFuseAES,
                                   FeatureFuseAdrpAdd,
                                   FeatureFuseLiterals,
                                   FeatureEnableSelectOptimize,
                                   FeaturePredictableSelectIsExpensive]>;

def TuneA73     : SubtargetFeature<"a73", "ARMProcFamily", "CortexA73",
                                   "Cortex-A73 ARM processors", [
                                   FeatureFuseAES,
                                   FeatureFuseAdrpAdd,
                                   FeatureEnableSelectOptimize,
                                   FeaturePredictableSelectIsExpensive]>;

def TuneA75     : SubtargetFeature<"a75", "ARMProcFamily", "CortexA75",
                                   "Cortex-A75 ARM processors", [
                                   FeatureFuseAES,
                                   FeatureFuseAdrpAdd,
                                   FeatureEnableSelectOptimize,
                                   FeaturePredictableSelectIsExpensive]>;

def TuneA76     : SubtargetFeature<"a76", "ARMProcFamily", "CortexA76",
                                   "Cortex-A76 ARM processors", [
                                   FeatureFuseAES,
                                   FeatureFuseAdrpAdd,
                                   FeatureLSLFast,
                                   FeatureEnableSelectOptimize,
                                   FeaturePredictableSelectIsExpensive]>;

def TuneA77     : SubtargetFeature<"a77", "ARMProcFamily", "CortexA77",
                                   "Cortex-A77 ARM processors", [
                                   FeatureCmpBccFusion,
                                   FeatureFuseAES,
                                   FeatureFuseAdrpAdd,
                                   FeatureLSLFast,
                                   FeatureEnableSelectOptimize,
                                   FeaturePredictableSelectIsExpensive]>;

def TuneA78 : SubtargetFeature<"a78", "ARMProcFamily", "CortexA78",
                               "Cortex-A78 ARM processors", [
                               FeatureCmpBccFusion,
                               FeatureFuseAES,
                               FeatureFuseAdrpAdd,
                               FeatureLSLFast,
                               FeaturePostRAScheduler,
                               FeatureEnableSelectOptimize,
                               FeaturePredictableSelectIsExpensive]>;

def TuneA78C : SubtargetFeature<"a78c", "ARMProcFamily",
                                "CortexA78C",
                                "Cortex-A78C ARM processors", [
                                FeatureCmpBccFusion,
                                FeatureFuseAES,
                                FeatureFuseAdrpAdd,
                                FeatureLSLFast,
                                FeaturePostRAScheduler,
                                FeatureEnableSelectOptimize,
                                FeaturePredictableSelectIsExpensive]>;

def TuneA710    : SubtargetFeature<"a710", "ARMProcFamily", "CortexA710",
                                   "Cortex-A710 ARM processors", [
                                   FeatureCmpBccFusion,
                                   FeatureFuseAES,
                                   FeatureFuseAdrpAdd,
                                   FeatureLSLFast,
                                   FeaturePostRAScheduler,
                                   FeatureEnableSelectOptimize,
                                   FeaturePredictableSelectIsExpensive]>;

def TuneA715 : SubtargetFeature<"a715", "ARMProcFamily", "CortexA715",
                                 "Cortex-A715 ARM processors", [
                                 FeatureFuseAES,
                                 FeaturePostRAScheduler,
                                 FeatureCmpBccFusion,
                                 FeatureLSLFast,
                                 FeatureFuseAdrpAdd,
                                 FeatureEnableSelectOptimize,
                                 FeaturePredictableSelectIsExpensive]>;

def TuneR82 : SubtargetFeature<"cortex-r82", "ARMProcFamily",
                               "CortexR82",
                               "Cortex-R82 ARM processors", [
                               FeaturePostRAScheduler]>;

def TuneX1 : SubtargetFeature<"cortex-x1", "ARMProcFamily", "CortexX1",
                                  "Cortex-X1 ARM processors", [
                                  FeatureCmpBccFusion,
                                  FeatureFuseAES,
                                  FeatureFuseAdrpAdd,
                                  FeatureLSLFast,
                                  FeaturePostRAScheduler,
                                  FeatureEnableSelectOptimize,
                                  FeaturePredictableSelectIsExpensive]>;

def TuneX2 : SubtargetFeature<"cortex-x2", "ARMProcFamily", "CortexX2",
                                  "Cortex-X2 ARM processors", [
                                  FeatureCmpBccFusion,
                                  FeatureFuseAES,
                                  FeatureFuseAdrpAdd,
                                  FeatureLSLFast,
                                  FeaturePostRAScheduler,
                                  FeatureEnableSelectOptimize,
                                  FeaturePredictableSelectIsExpensive]>;

def TuneX3 : SubtargetFeature<"cortex-x3", "ARMProcFamily", "CortexX3",
                              "Cortex-X3 ARM processors", [
                               FeatureLSLFast,
                               FeatureFuseAdrpAdd,
                               FeatureFuseAES,
                               FeaturePostRAScheduler,
                               FeatureEnableSelectOptimize,
                               FeaturePredictableSelectIsExpensive]>;

def TuneA64FX : SubtargetFeature<"a64fx", "ARMProcFamily", "A64FX",
                                 "Fujitsu A64FX processors", [
                                 FeaturePostRAScheduler,
                                 FeatureAggressiveFMA,
                                 FeatureArithmeticBccFusion,
                                 FeaturePredictableSelectIsExpensive
                                 ]>;

def TuneCarmel : SubtargetFeature<"carmel", "ARMProcFamily", "Carmel",
                                  "Nvidia Carmel processors">;

// Note that cyclone does not fuse AES instructions, but newer apple chips do
// perform the fusion and cyclone is used by default when targetting apple OSes.
def TuneAppleA7  : SubtargetFeature<"apple-a7", "ARMProcFamily", "AppleA7",
                                    "Apple A7 (the CPU formerly known as Cyclone)", [
                                    FeatureAlternateSExtLoadCVTF32Pattern,
                                    FeatureArithmeticBccFusion,
                                    FeatureArithmeticCbzFusion,
                                    FeatureDisableLatencySchedHeuristic,
                                    FeatureFuseAES, FeatureFuseCryptoEOR,
                                    FeatureZCRegMove,
                                    FeatureZCZeroing,
                                    FeatureZCZeroingFPWorkaround]
                                    >;

def TuneAppleA10 : SubtargetFeature<"apple-a10", "ARMProcFamily", "AppleA10",
                                    "Apple A10", [
                                    FeatureAlternateSExtLoadCVTF32Pattern,
                                    FeatureArithmeticBccFusion,
                                    FeatureArithmeticCbzFusion,
                                    FeatureDisableLatencySchedHeuristic,
                                    FeatureFuseAES,
                                    FeatureFuseCryptoEOR,
                                    FeatureZCRegMove,
                                    FeatureZCZeroing]
                                    >;

def TuneAppleA11 : SubtargetFeature<"apple-a11", "ARMProcFamily", "AppleA11",
                                    "Apple A11", [
                                    FeatureAlternateSExtLoadCVTF32Pattern,
                                    FeatureArithmeticBccFusion,
                                    FeatureArithmeticCbzFusion,
                                    FeatureDisableLatencySchedHeuristic,
                                    FeatureFuseAES,
                                    FeatureFuseCryptoEOR,
                                    FeatureZCRegMove,
                                    FeatureZCZeroing]
                                    >;

def TuneAppleA12 : SubtargetFeature<"apple-a12", "ARMProcFamily", "AppleA12",
                                    "Apple A12", [
                                    FeatureAlternateSExtLoadCVTF32Pattern,
                                    FeatureArithmeticBccFusion,
                                    FeatureArithmeticCbzFusion,
                                    FeatureDisableLatencySchedHeuristic,
                                    FeatureFuseAES,
                                    FeatureFuseCryptoEOR,
                                    FeatureZCRegMove,
                                    FeatureZCZeroing]
                                    >;

def TuneAppleA13 : SubtargetFeature<"apple-a13", "ARMProcFamily", "AppleA13",
                                    "Apple A13", [
                                    FeatureAlternateSExtLoadCVTF32Pattern,
                                    FeatureArithmeticBccFusion,
                                    FeatureArithmeticCbzFusion,
                                    FeatureDisableLatencySchedHeuristic,
                                    FeatureFuseAES,
                                    FeatureFuseCryptoEOR,
                                    FeatureZCRegMove,
                                    FeatureZCZeroing]
                                    >;

def TuneAppleA14 : SubtargetFeature<"apple-a14", "ARMProcFamily", "AppleA14",
                                    "Apple A14", [
                                    FeatureAggressiveFMA,
                                    FeatureAlternateSExtLoadCVTF32Pattern,
                                    FeatureArithmeticBccFusion,
                                    FeatureArithmeticCbzFusion,
                                    FeatureDisableLatencySchedHeuristic,
                                    FeatureFuseAddress,
                                    FeatureFuseAES,
                                    FeatureFuseArithmeticLogic,
                                    FeatureFuseCCSelect,
                                    FeatureFuseCryptoEOR,
                                    FeatureFuseAdrpAdd,
                                    FeatureFuseLiterals,
                                    FeatureZCRegMove,
                                    FeatureZCZeroing]>;

def TuneAppleA15 : SubtargetFeature<"apple-a15", "ARMProcFamily", "AppleA15",
                                    "Apple A15", [
                                    FeatureAlternateSExtLoadCVTF32Pattern,
                                    FeatureArithmeticBccFusion,
                                    FeatureArithmeticCbzFusion,
                                    FeatureDisableLatencySchedHeuristic,
                                    FeatureFuseAddress,
                                    FeatureFuseAES,
                                    FeatureFuseArithmeticLogic,
                                    FeatureFuseCCSelect,
                                    FeatureFuseCryptoEOR,
                                    FeatureFuseLiterals,
                                    FeatureZCRegMove,
                                    FeatureZCZeroing
                                    ]>;

def TuneAppleA16 : SubtargetFeature<"apple-a16", "ARMProcFamily", "AppleA16",
                                    "Apple A16", [
                                    FeatureAlternateSExtLoadCVTF32Pattern,
                                    FeatureArithmeticBccFusion,
                                    FeatureArithmeticCbzFusion,
                                    FeatureDisableLatencySchedHeuristic,
                                    FeatureFuseAddress,
                                    FeatureFuseAES,
                                    FeatureFuseArithmeticLogic,
                                    FeatureFuseCCSelect,
                                    FeatureFuseCryptoEOR,
                                    FeatureFuseLiterals,
                                    FeatureZCRegMove,
                                    FeatureZCZeroing
                                    ]>;

def TuneExynosM3 : SubtargetFeature<"exynosm3", "ARMProcFamily", "ExynosM3",
                                    "Samsung Exynos-M3 processors",
                                    [FeatureExynosCheapAsMoveHandling,
                                     FeatureForce32BitJumpTables,
                                     FeatureFuseAddress,
                                     FeatureFuseAES,
                                     FeatureFuseCCSelect,
                                     FeatureFuseAdrpAdd,
                                     FeatureFuseLiterals,
                                     FeatureLSLFast,
                                     FeaturePostRAScheduler,
                                     FeaturePredictableSelectIsExpensive]>;

// Re-uses some scheduling and tunings from the ExynosM3 proc family.
def TuneExynosM4 : SubtargetFeature<"exynosm4", "ARMProcFamily", "ExynosM3",
                                    "Samsung Exynos-M4 processors",
                                    [FeatureArithmeticBccFusion,
                                     FeatureArithmeticCbzFusion,
                                     FeatureExynosCheapAsMoveHandling,
                                     FeatureForce32BitJumpTables,
                                     FeatureFuseAddress,
                                     FeatureFuseAES,
                                     FeatureFuseArithmeticLogic,
                                     FeatureFuseCCSelect,
                                     FeatureFuseAdrpAdd,
                                     FeatureFuseLiterals,
                                     FeatureLSLFast,
                                     FeaturePostRAScheduler,
                                     FeatureZCZeroing]>;

def TuneKryo    : SubtargetFeature<"kryo", "ARMProcFamily", "Kryo",
                                   "Qualcomm Kryo processors", [
                                   FeatureCustomCheapAsMoveHandling,
                                   FeaturePostRAScheduler,
                                   FeaturePredictableSelectIsExpensive,
                                   FeatureZCZeroing,
                                   FeatureLSLFast]
                                   >;

def TuneFalkor  : SubtargetFeature<"falkor", "ARMProcFamily", "Falkor",
                                   "Qualcomm Falkor processors", [
                                   FeatureCustomCheapAsMoveHandling,
                                   FeaturePostRAScheduler,
                                   FeaturePredictableSelectIsExpensive,
                                   FeatureZCZeroing,
                                   FeatureLSLFast,
                                   FeatureSlowSTRQro
                                   ]>;

def TuneNeoverseE1 : SubtargetFeature<"neoversee1", "ARMProcFamily", "NeoverseE1",
                                      "Neoverse E1 ARM processors", [
                                      FeatureFuseAES,
                                      FeatureFuseAdrpAdd,
                                      FeaturePostRAScheduler]>;

def TuneNeoverseN1 : SubtargetFeature<"neoversen1", "ARMProcFamily", "NeoverseN1",
                                      "Neoverse N1 ARM processors", [
                                      FeatureFuseAES,
                                      FeatureFuseAdrpAdd,
                                      FeatureLSLFast,
                                      FeaturePostRAScheduler,
                                      FeatureEnableSelectOptimize,
                                      FeaturePredictableSelectIsExpensive]>;

def TuneNeoverseN2 : SubtargetFeature<"neoversen2", "ARMProcFamily", "NeoverseN2",
                                      "Neoverse N2 ARM processors", [
                                      FeatureFuseAES,
                                      FeatureFuseAdrpAdd,
                                      FeatureLSLFast,
                                      FeaturePostRAScheduler,
                                      FeatureEnableSelectOptimize,
                                      FeaturePredictableSelectIsExpensive]>;

def TuneNeoverse512TVB : SubtargetFeature<"neoverse512tvb", "ARMProcFamily", "Neoverse512TVB",
                                      "Neoverse 512-TVB ARM processors", [
                                      FeatureFuseAES,
                                      FeatureFuseAdrpAdd,
                                      FeatureLSLFast,
                                      FeaturePostRAScheduler,
                                      FeatureEnableSelectOptimize,
                                      FeaturePredictableSelectIsExpensive]>;

def TuneNeoverseV1 : SubtargetFeature<"neoversev1", "ARMProcFamily", "NeoverseV1",
                                      "Neoverse V1 ARM processors", [
                                      FeatureFuseAES,
                                      FeatureFuseAdrpAdd,
                                      FeatureLSLFast,
                                      FeaturePostRAScheduler,
                                      FeatureEnableSelectOptimize,
                                      FeaturePredictableSelectIsExpensive,
                                      FeatureNoSVEFPLD1R]>;

def TuneNeoverseV2 : SubtargetFeature<"neoversev2", "ARMProcFamily", "NeoverseV2",
                                      "Neoverse V2 ARM processors", [
                                      FeatureFuseAES,
                                      FeatureLSLFast,
                                      FeaturePostRAScheduler,
                                      FeatureEnableSelectOptimize,
                                      FeaturePredictableSelectIsExpensive]>;

def TuneSaphira  : SubtargetFeature<"saphira", "ARMProcFamily", "Saphira",
                                   "Qualcomm Saphira processors", [
                                   FeatureCustomCheapAsMoveHandling,
                                   FeaturePostRAScheduler,
                                   FeaturePredictableSelectIsExpensive,
                                   FeatureZCZeroing,
                                   FeatureLSLFast]>;

def TuneThunderX2T99  : SubtargetFeature<"thunderx2t99", "ARMProcFamily", "ThunderX2T99",
                                         "Cavium ThunderX2 processors", [
                                          FeatureAggressiveFMA,
                                          FeatureArithmeticBccFusion,
                                          FeaturePostRAScheduler,
                                          FeaturePredictableSelectIsExpensive]>;

def TuneThunderX3T110  : SubtargetFeature<"thunderx3t110", "ARMProcFamily",
                                          "ThunderX3T110",
                                          "Marvell ThunderX3 processors", [
                                           FeatureAggressiveFMA,
                                           FeatureArithmeticBccFusion,
                                           FeaturePostRAScheduler,
                                           FeaturePredictableSelectIsExpensive,
                                           FeatureBalanceFPOps,
                                           FeatureStrictAlign]>;

def TuneThunderX : SubtargetFeature<"thunderx", "ARMProcFamily", "ThunderX",
                                    "Cavium ThunderX processors", [
                                    FeaturePostRAScheduler,
                                    FeaturePredictableSelectIsExpensive]>;

def TuneThunderXT88 : SubtargetFeature<"thunderxt88", "ARMProcFamily",
                                       "ThunderXT88",
                                       "Cavium ThunderX processors", [
                                       FeaturePostRAScheduler,
                                       FeaturePredictableSelectIsExpensive]>;

def TuneThunderXT81 : SubtargetFeature<"thunderxt81", "ARMProcFamily",
                                       "ThunderXT81",
                                       "Cavium ThunderX processors", [
                                       FeaturePostRAScheduler,
                                       FeaturePredictableSelectIsExpensive]>;

def TuneThunderXT83 : SubtargetFeature<"thunderxt83", "ARMProcFamily",
                                       "ThunderXT83",
                                       "Cavium ThunderX processors", [
                                       FeaturePostRAScheduler,
                                       FeaturePredictableSelectIsExpensive]>;

def TuneTSV110 : SubtargetFeature<"tsv110", "ARMProcFamily", "TSV110",
                                  "HiSilicon TS-V110 processors", [
                                  FeatureCustomCheapAsMoveHandling,
                                  FeatureFuseAES,
                                  FeaturePostRAScheduler]>;

def TuneHIP09 : SubtargetFeature<"hip09", "ARMProcFamily", "HIP09",
                                   "HiSilicon HIP-09 processors", [
                                   FeatureCustomCheapAsMoveHandling,
                                   FeatureExperimentalZeroingPseudos,
                                   FeatureFuseAES,
                                   FeatureLSLFast,
                                   FeatureAscendStoreAddress,
                                   FeatureCmpBccFusion,
                                   FeatureArithmeticBccFusion,
                                   FeatureFuseLiterals,
                                   FeatureFuseMvnClz,
                                   FeaturePostRAScheduler]>;

def TuneHIP10C : SubtargetFeature<"hip10c", "ARMProcFamily", "HIP10C",
                                   "HiSilicon HIP10C processors", [
                                   FeatureCustomCheapAsMoveHandling,
                                   FeatureCmpBccFusion,
                                   FeatureExperimentalZeroingPseudos,
                                   FeatureFuseAES,
                                   FeatureLSLFast,
                                   FeatureAscendStoreAddress,
                                   FeaturePostRAScheduler]>;

def TuneHIP11 : SubtargetFeature<"hip11", "ARMProcFamily", "HIP11",
                                   "HiSilicon HIP11 processors", [
                                    FeatureCustomCheapAsMoveHandling,
                                    FeatureExperimentalZeroingPseudos,
                                    FeatureFuseAES,
                                    FeatureLSLFast,
                                    FeatureAscendStoreAddress,
                                    FeatureCmpBccFusion,
                                    FeatureArithmeticBccFusion,
                                    FeaturePostRAScheduler]>;

def TuneHIP12 : SubtargetFeature<"hip12", "ARMProcFamily", "HIP12",
                                   "HiSilicon HIP12 processors", [
                                    FeatureCustomCheapAsMoveHandling,
                                    FeatureExperimentalZeroingPseudos,
                                    FeatureCmpBccFusion,
                                    FeatureFuseAES,
                                    FeatureLSLFast,
                                    FeatureAscendStoreAddress,
                                    FeaturePostRAScheduler]>;

def TuneAmpere1 : SubtargetFeature<"ampere1", "ARMProcFamily", "Ampere1",
                                   "Ampere Computing Ampere-1 processors", [
                                   FeaturePostRAScheduler,
                                   FeatureFuseAES,
                                   FeatureLSLFast,
                                   FeatureAggressiveFMA,
                                   FeatureArithmeticBccFusion,
                                   FeatureCmpBccFusion,
                                   FeatureFuseAddress,
                                   FeatureFuseLiterals]>;

def TuneAmpere1A : SubtargetFeature<"ampere1a", "ARMProcFamily", "Ampere1A",
                                    "Ampere Computing Ampere-1A processors", [
                                    FeaturePostRAScheduler,
                                    FeatureFuseAES,
                                    FeatureLSLFast,
                                    FeatureAggressiveFMA,
                                    FeatureArithmeticBccFusion,
                                    FeatureCmpBccFusion,
                                    FeatureFuseAddress,
                                    FeatureFuseLiterals,
                                    FeatureFuseLiterals]>;

def ProcessorFeatures {
  list<SubtargetFeature> A53  = [HasV8_0aOps, FeatureCRC, FeatureCrypto,
                                 FeatureFPARMv8, FeatureNEON, FeaturePerfMon];
  list<SubtargetFeature> A55  = [HasV8_2aOps, FeatureCrypto, FeatureFPARMv8,
                                 FeatureNEON, FeatureFullFP16, FeatureDotProd,
                                 FeatureRCPC, FeaturePerfMon, FeatureLSE];
  list<SubtargetFeature> A510 = [HasV9_0aOps, FeatureNEON, FeaturePerfMon,
                                 FeatureMatMulInt8, FeatureBF16, FeatureAM,
                                 FeatureMTE, FeatureETE, FeatureSVE2BitPerm,
                                 FeatureFP16FML, FeatureLSE];
  list<SubtargetFeature> A65  = [HasV8_2aOps, FeatureCrypto, FeatureFPARMv8,
                                 FeatureNEON, FeatureFullFP16, FeatureDotProd,
                                 FeatureRCPC, FeatureSSBS, FeatureRAS,
                                 FeaturePerfMon, FeatureLSE];
  list<SubtargetFeature> A76  = [HasV8_2aOps, FeatureCrypto, FeatureFPARMv8,
                                 FeatureNEON, FeatureFullFP16, FeatureDotProd,
                                 FeatureRCPC, FeatureSSBS, FeaturePerfMon,
                                 FeatureLSE];
  list<SubtargetFeature> A77  = [HasV8_2aOps, FeatureCrypto, FeatureFPARMv8,
                                 FeatureNEON, FeatureFullFP16, FeatureDotProd,
                                 FeatureRCPC, FeaturePerfMon, FeatureSSBS,
                                 FeatureLSE];
  list<SubtargetFeature> A78  = [HasV8_2aOps, FeatureCrypto, FeatureFPARMv8,
                                 FeatureNEON, FeatureFullFP16, FeatureDotProd,
                                 FeatureRCPC, FeaturePerfMon, FeatureSPE,
                                 FeatureSSBS, FeatureLSE];
  list<SubtargetFeature> A78C = [HasV8_2aOps, FeatureCrypto, FeatureFPARMv8,
                                 FeatureNEON, FeatureFullFP16, FeatureDotProd,
                                 FeatureFlagM, FeatureFP16FML, FeaturePAuth,
                                 FeaturePerfMon, FeatureRCPC, FeatureSPE,
                                 FeatureSSBS, FeatureLSE];
  list<SubtargetFeature> A710 = [HasV9_0aOps, FeatureNEON, FeaturePerfMon,
                                 FeatureETE, FeatureMTE, FeatureFP16FML,
                                 FeatureSVE2BitPerm, FeatureBF16, FeatureMatMulInt8,
                                 FeatureLSE];
  list<SubtargetFeature> A715 = [HasV9_0aOps, FeatureNEON, FeatureMTE,
                                 FeatureFP16FML, FeatureSVE, FeatureTRBE,
                                 FeatureSVE2BitPerm, FeatureBF16, FeatureETE,
                                 FeaturePerfMon, FeatureMatMulInt8, FeatureSPE,
                                 FeatureLSE];
  list<SubtargetFeature> R82  = [HasV8_0rOps, FeaturePerfMon, FeatureFullFP16,
                                 FeatureFP16FML, FeatureSSBS, FeaturePredRes,
                                 FeatureSB];
  list<SubtargetFeature> X1   = [HasV8_2aOps, FeatureCrypto, FeatureFPARMv8,
                                 FeatureNEON, FeatureRCPC, FeaturePerfMon,
                                 FeatureSPE, FeatureFullFP16, FeatureDotProd,
                                 FeatureSSBS, FeatureLSE];
  list<SubtargetFeature> X1C  = [HasV8_2aOps, FeatureCrypto, FeatureFPARMv8,
                                 FeatureNEON, FeatureRCPC_IMMO, FeaturePerfMon,
                                 FeatureSPE, FeatureFullFP16, FeatureDotProd,
                                 FeaturePAuth, FeatureSSBS, FeatureFlagM,
                                 FeatureLSE2, FeatureLSE];
  list<SubtargetFeature> X2   = [HasV9_0aOps, FeatureNEON, FeaturePerfMon,
                                 FeatureMatMulInt8, FeatureBF16, FeatureAM,
                                 FeatureMTE, FeatureETE, FeatureSVE2BitPerm,
                                 FeatureFP16FML, FeatureLSE];
  list<SubtargetFeature> X3 =   [HasV9_0aOps, FeatureSVE, FeatureNEON,
                                 FeaturePerfMon, FeatureETE, FeatureTRBE,
                                 FeatureSPE, FeatureBF16, FeatureMatMulInt8,
                                 FeatureMTE, FeatureSVE2BitPerm, FeatureFullFP16,
                                 FeatureFP16FML, FeatureLSE];
  list<SubtargetFeature> A64FX    = [HasV8_2aOps, FeatureFPARMv8, FeatureNEON,
                                     FeatureSHA2, FeaturePerfMon, FeatureFullFP16,
                                     FeatureSVE, FeatureComplxNum, FeatureLSE];
  list<SubtargetFeature> Carmel   = [HasV8_2aOps, FeatureNEON, FeatureCrypto,
                                     FeatureFullFP16, FeatureLSE];
  list<SubtargetFeature> AppleA7  = [HasV8_0aOps, FeatureCrypto, FeatureFPARMv8,
                                     FeatureNEON,FeaturePerfMon, FeatureAppleA7SysReg];
  list<SubtargetFeature> AppleA10 = [HasV8_0aOps, FeatureCrypto, FeatureFPARMv8,
                                     FeatureNEON, FeaturePerfMon, FeatureCRC,
                                     FeatureRDM, FeaturePAN, FeatureLOR, FeatureVH];
  list<SubtargetFeature> AppleA11 = [HasV8_2aOps, FeatureCrypto, FeatureFPARMv8,
                                     FeatureNEON, FeaturePerfMon, FeatureFullFP16,
                                     FeatureLSE];
  list<SubtargetFeature> AppleA12 = [HasV8_3aOps, FeatureCrypto, FeatureFPARMv8,
                                     FeatureNEON, FeaturePerfMon, FeatureFullFP16,
                                     FeatureLSE];
  list<SubtargetFeature> AppleA13 = [HasV8_4aOps, FeatureCrypto, FeatureFPARMv8,
                                     FeatureNEON, FeaturePerfMon, FeatureFullFP16,
                                     FeatureFP16FML, FeatureSHA3, FeatureLSE];
  list<SubtargetFeature> AppleA14 = [HasV8_4aOps, FeatureCrypto, FeatureFPARMv8,
                                     FeatureNEON, FeaturePerfMon, FeatureFRInt3264,
                                     FeatureSpecRestrict, FeatureSSBS, FeatureSB,
                                     FeaturePredRes, FeatureCacheDeepPersist,
                                     FeatureFullFP16, FeatureFP16FML, FeatureSHA3,
                                     FeatureAltFPCmp, FeatureLSE];
  list<SubtargetFeature> AppleA15 = [HasV8_6aOps, FeatureCrypto, FeatureFPARMv8,
                                     FeatureNEON, FeaturePerfMon, FeatureSHA3,
                                     FeatureFullFP16, FeatureFP16FML, FeatureLSE];
  list<SubtargetFeature> AppleA16 = [HasV8_6aOps, FeatureCrypto, FeatureFPARMv8,
                                     FeatureNEON, FeaturePerfMon, FeatureSHA3,
                                     FeatureFullFP16, FeatureFP16FML,
                                     FeatureHCX, FeatureLSE];
  list<SubtargetFeature> ExynosM3 = [HasV8_0aOps, FeatureCRC, FeatureCrypto,
                                     FeaturePerfMon];
  list<SubtargetFeature> ExynosM4 = [HasV8_2aOps, FeatureCrypto, FeatureDotProd,
                                     FeatureFullFP16, FeaturePerfMon, FeatureLSE];
  list<SubtargetFeature> Falkor   = [HasV8_0aOps, FeatureCRC, FeatureCrypto,
                                     FeatureFPARMv8, FeatureNEON, FeaturePerfMon,
                                     FeatureRDM];
  list<SubtargetFeature> NeoverseE1 = [HasV8_2aOps, FeatureCrypto, FeatureDotProd,
                                       FeatureFPARMv8, FeatureFullFP16, FeatureNEON,
                                       FeatureRCPC, FeatureSSBS, FeaturePerfMon,
                                       FeatureLSE];
  list<SubtargetFeature> NeoverseN1 = [HasV8_2aOps, FeatureCrypto, FeatureDotProd,
                                       FeatureFPARMv8, FeatureFullFP16, FeatureNEON,
                                       FeatureRCPC, FeatureSPE, FeatureSSBS,
                                       FeaturePerfMon, FeatureLSE];
  list<SubtargetFeature> NeoverseN2 = [HasV8_5aOps, FeatureBF16, FeatureETE,
                                       FeatureMatMulInt8, FeatureMTE, FeatureSVE2,
                                       FeatureSVE2BitPerm, FeatureTRBE, FeatureCrypto,
                                       FeaturePerfMon, FeatureLSE];
  list<SubtargetFeature> Neoverse512TVB = [HasV8_4aOps, FeatureBF16, FeatureCacheDeepPersist,
                                           FeatureCrypto, FeatureFPARMv8, FeatureFP16FML,
                                           FeatureFullFP16, FeatureMatMulInt8, FeatureNEON,
                                           FeaturePerfMon, FeatureRandGen, FeatureSPE,
                                           FeatureSSBS, FeatureSVE, FeatureLSE];
  list<SubtargetFeature> NeoverseV1 = [HasV8_4aOps, FeatureBF16, FeatureCacheDeepPersist,
                                       FeatureCrypto, FeatureFPARMv8, FeatureFP16FML,
                                       FeatureFullFP16, FeatureMatMulInt8, FeatureNEON,
                                       FeaturePerfMon, FeatureRandGen, FeatureSPE,
                                       FeatureSSBS, FeatureSVE, FeatureLSE];
  list<SubtargetFeature> NeoverseV2 = [HasV9_0aOps, FeatureBF16, FeatureSPE,
                                       FeaturePerfMon, FeatureETE, FeatureMatMulInt8,
                                       FeatureNEON, FeatureSVE2BitPerm, FeatureFP16FML,
                                       FeatureMTE, FeatureRandGen, FeatureLSE];
  list<SubtargetFeature> Saphira    = [HasV8_4aOps, FeatureCrypto, FeatureFPARMv8,
                                       FeatureNEON, FeatureSPE, FeaturePerfMon,
                                       FeatureLSE];
  list<SubtargetFeature> ThunderX   = [HasV8_0aOps, FeatureCRC, FeatureCrypto,
                                       FeatureFPARMv8, FeaturePerfMon, FeatureNEON];
  list<SubtargetFeature> ThunderX2T99  = [HasV8_1aOps, FeatureCRC, FeatureCrypto,
                                          FeatureFPARMv8, FeatureNEON, FeatureLSE];
  list<SubtargetFeature> ThunderX3T110 = [HasV8_3aOps, FeatureCRC, FeatureCrypto,
                                          FeatureFPARMv8, FeatureNEON, FeatureLSE,
                                          FeaturePAuth, FeaturePerfMon];
  list<SubtargetFeature> TSV110 = [HasV8_2aOps, FeatureCrypto, FeatureFPARMv8,
                                   FeatureNEON, FeaturePerfMon, FeatureSPE,
                                   FeatureFullFP16, FeatureFP16FML, FeatureDotProd];
  list<SubtargetFeature> HIP09 = [HasV8_5aOps, FeatureBF16, FeatureCrypto, FeatureFPARMv8,
                                  FeatureMatMulInt8, FeatureMatMulFP32, FeatureMatMulFP64,
                                  FeatureNEON, FeaturePerfMon, FeatureRandGen, FeatureSPE,
                                  FeatureFullFP16, FeatureFP16FML, FeatureDotProd,
                                  FeatureJS, FeatureComplxNum, FeatureSHA3, FeatureSM4,
                                  FeatureSVE];
  list<SubtargetFeature> HIP10C = [HasV8_5aOps, FeatureAES, FeatureFP16FML, FeatureFullFP16,
                                   FeatureMatMulFP32, FeatureMatMulFP64, FeatureMTE, FeaturePerfMon,
                                   FeatureRandGen, FeatureSHA2, FeatureSHA3, FeatureSM4, FeatureWFxT,
                                   FeatureBF16, FeatureMatMulInt8];
  list<SubtargetFeature> HIP11 = [HasV9_0aOps, FeaturePerfMon, FeatureETE, FeatureMTE,
                                  FeatureFP16FML, FeatureSVE2BitPerm, FeatureBF16,
                                  FeatureMatMulInt8, FeatureCrypto, FeatureSHA3, FeatureSM4,
                                  FeatureSME, FeatureSMEF64F64, FeatureSMEFA64];
  list<SubtargetFeature> HIP12 = [HasV9_2aOps, FeatureSVE, FeatureSVE2,
                                  FeatureSVE2BitPerm, FeatureSVE2AES,
                                  FeatureSVE2SM4, FeatureSVE2SHA3,
                                  FeatureFullFP16, FeaturePerfMon,
                                  FeatureETE, FeatureTRBE, FeatureSPE,
                                  FeatureSPE_EEF, FeatureNMI,
                                  FeatureHBC, FeatureRCPC3, FeatureBF16,
                                  FeatureComplxNum, FeatureCRC,
                                  FeatureDotProd, FeatureFPARMv8,
                                  FeatureMatMulInt8, FeatureJS,
                                  FeatureLSE, FeatureNEON, FeaturePAuth,
                                  FeatureRAS, FeatureRCPC, FeatureRDM,
                                  FeatureFPAC, FeatureLS64, FeatureRME,
                                  FeatureBRBE];
  list<SubtargetFeature> Ampere1 = [HasV8_6aOps, FeatureNEON, FeaturePerfMon,
                                    FeatureSSBS, FeatureRandGen, FeatureSB,
                                    FeatureSHA2, FeatureSHA3, FeatureAES,
                                    FeatureLSE];
  list<SubtargetFeature> Ampere1A = [HasV8_6aOps, FeatureNEON, FeaturePerfMon,
                                     FeatureMTE, FeatureSSBS, FeatureRandGen,
                                     FeatureSB, FeatureSM4, FeatureSHA2,
                                     FeatureSHA3, FeatureAES, FeatureLSE];

  // ETE and TRBE are future architecture extensions. We temporarily enable them
  // by default for users targeting generic AArch64. The extensions do not
  // affect code generated by the compiler and can be used only by explicitly
  // mentioning the new system register names in assembly.
  list<SubtargetFeature> Generic = [FeatureFPARMv8, FeatureNEON, FeatureETE];
}

// FeatureFuseAdrpAdd is enabled under Generic to allow linker merging
// optimizations.
def : ProcessorModel<"generic", CortexA55Model, ProcessorFeatures.Generic,
                     [FeatureFuseAES, FeatureFuseAdrpAdd, FeaturePostRAScheduler,
                      FeatureEnableSelectOptimize]>;
def : ProcessorModel<"cortex-a35", CortexA53Model, ProcessorFeatures.A53,
                     [TuneA35]>;
def : ProcessorModel<"cortex-a34", CortexA53Model, ProcessorFeatures.A53,
                     [TuneA35]>;
def : ProcessorModel<"cortex-a53", CortexA53Model, ProcessorFeatures.A53,
                     [TuneA53]>;
def : ProcessorModel<"cortex-a55", CortexA55Model, ProcessorFeatures.A55,
                     [TuneA55]>;
def : ProcessorModel<"cortex-a510", CortexA510Model, ProcessorFeatures.A510,
                     [TuneA510]>;
def : ProcessorModel<"cortex-a57", CortexA57Model, ProcessorFeatures.A53,
                     [TuneA57]>;
def : ProcessorModel<"cortex-a65", CortexA53Model, ProcessorFeatures.A65,
                     [TuneA65]>;
def : ProcessorModel<"cortex-a65ae", CortexA53Model, ProcessorFeatures.A65,
                     [TuneA65]>;
def : ProcessorModel<"cortex-a72", CortexA57Model, ProcessorFeatures.A53,
                     [TuneA72]>;
def : ProcessorModel<"cortex-a73", CortexA57Model, ProcessorFeatures.A53,
                     [TuneA73]>;
def : ProcessorModel<"cortex-a75", CortexA57Model, ProcessorFeatures.A55,
                     [TuneA75]>;
def : ProcessorModel<"cortex-a76", CortexA57Model, ProcessorFeatures.A76,
                     [TuneA76]>;
def : ProcessorModel<"cortex-a76ae", CortexA57Model, ProcessorFeatures.A76,
                     [TuneA76]>;
def : ProcessorModel<"cortex-a77", CortexA57Model, ProcessorFeatures.A77,
                     [TuneA77]>;
def : ProcessorModel<"cortex-a78", CortexA57Model, ProcessorFeatures.A78,
                     [TuneA78]>;
def : ProcessorModel<"cortex-a78c", CortexA57Model, ProcessorFeatures.A78C,
                     [TuneA78C]>;
def : ProcessorModel<"cortex-a710", NeoverseN2Model, ProcessorFeatures.A710,
                     [TuneA710]>;
def : ProcessorModel<"cortex-a715", NeoverseN2Model, ProcessorFeatures.A715,
                     [TuneA715]>;
def : ProcessorModel<"cortex-r82", CortexA55Model, ProcessorFeatures.R82,
                     [TuneR82]>;
def : ProcessorModel<"cortex-x1", CortexA57Model, ProcessorFeatures.X1,
                     [TuneX1]>;
def : ProcessorModel<"cortex-x1c", CortexA57Model, ProcessorFeatures.X1C,
                     [TuneX1]>;
def : ProcessorModel<"cortex-x2", NeoverseN2Model, ProcessorFeatures.X2,
                     [TuneX2]>;
def : ProcessorModel<"cortex-x3", NeoverseN2Model, ProcessorFeatures.X3,
                     [TuneX3]>;
def : ProcessorModel<"neoverse-e1", CortexA53Model,
                     ProcessorFeatures.NeoverseE1, [TuneNeoverseE1]>;
def : ProcessorModel<"neoverse-n1", NeoverseN1Model,
                     ProcessorFeatures.NeoverseN1, [TuneNeoverseN1]>;
def : ProcessorModel<"neoverse-n2", NeoverseN2Model,
                     ProcessorFeatures.NeoverseN2, [TuneNeoverseN2]>;
def : ProcessorModel<"neoverse-512tvb", NeoverseV1Model,
                     ProcessorFeatures.Neoverse512TVB, [TuneNeoverse512TVB]>;
def : ProcessorModel<"neoverse-v1", NeoverseV1Model,
                     ProcessorFeatures.NeoverseV1, [TuneNeoverseV1]>;
def : ProcessorModel<"neoverse-v2", NeoverseV2Model,
                     ProcessorFeatures.NeoverseV2, [TuneNeoverseV2]>;
def : ProcessorModel<"exynos-m3", ExynosM3Model, ProcessorFeatures.ExynosM3,
                     [TuneExynosM3]>;
def : ProcessorModel<"exynos-m4", ExynosM4Model, ProcessorFeatures.ExynosM4,
                     [TuneExynosM4]>;
def : ProcessorModel<"exynos-m5", ExynosM5Model, ProcessorFeatures.ExynosM4,
                     [TuneExynosM4]>;
def : ProcessorModel<"falkor", FalkorModel, ProcessorFeatures.Falkor,
                     [TuneFalkor]>;
def : ProcessorModel<"saphira", FalkorModel, ProcessorFeatures.Saphira,
                     [TuneSaphira]>;
def : ProcessorModel<"kryo", KryoModel, ProcessorFeatures.A53, [TuneKryo]>;

// Cavium ThunderX/ThunderX T8X  Processors
def : ProcessorModel<"thunderx", ThunderXT8XModel,  ProcessorFeatures.ThunderX,
                     [TuneThunderX]>;
def : ProcessorModel<"thunderxt88", ThunderXT8XModel,
                     ProcessorFeatures.ThunderX, [TuneThunderXT88]>;
def : ProcessorModel<"thunderxt81", ThunderXT8XModel,
                     ProcessorFeatures.ThunderX, [TuneThunderXT81]>;
def : ProcessorModel<"thunderxt83", ThunderXT8XModel,
                     ProcessorFeatures.ThunderX, [TuneThunderXT83]>;
// Cavium ThunderX2T9X  Processors. Formerly Broadcom Vulcan.
def : ProcessorModel<"thunderx2t99", ThunderX2T99Model,
                     ProcessorFeatures.ThunderX2T99, [TuneThunderX2T99]>;
// Marvell ThunderX3T110 Processors.
def : ProcessorModel<"thunderx3t110", ThunderX3T110Model,
                     ProcessorFeatures.ThunderX3T110, [TuneThunderX3T110]>;

// HiSilicon Processors.
def : ProcessorModel<"tsv110", TSV110Model, ProcessorFeatures.TSV110,
                     [TuneTSV110]>;
def : ProcessorModel<"hip09", HIP09Model, ProcessorFeatures.HIP09,
                     [TuneHIP09]>;
def : ProcessorModel<"hip10c", HIP10CModel, ProcessorFeatures.HIP10C,
                     [TuneHIP10C]>;
def : ProcessorModel<"hip11", HIP11Model, ProcessorFeatures.HIP11,
                     [TuneHIP11]>;
def : ProcessorModel<"hip12", HIP12Model, ProcessorFeatures.HIP12,
                     [TuneHIP12]>;

// Support cyclone as an alias for apple-a7 so we can still LTO old bitcode.
def : ProcessorModel<"cyclone", CycloneModel, ProcessorFeatures.AppleA7,
                     [TuneAppleA7]>;

// iPhone and iPad CPUs
def : ProcessorModel<"apple-a7", CycloneModel, ProcessorFeatures.AppleA7,
                     [TuneAppleA7]>;
def : ProcessorModel<"apple-a8", CycloneModel, ProcessorFeatures.AppleA7,
                     [TuneAppleA7]>;
def : ProcessorModel<"apple-a9", CycloneModel, ProcessorFeatures.AppleA7,
                     [TuneAppleA7]>;
def : ProcessorModel<"apple-a10", CycloneModel, ProcessorFeatures.AppleA10,
                     [TuneAppleA10]>;
def : ProcessorModel<"apple-a11", CycloneModel, ProcessorFeatures.AppleA11,
                     [TuneAppleA11]>;
def : ProcessorModel<"apple-a12", CycloneModel, ProcessorFeatures.AppleA12,
                     [TuneAppleA12]>;
def : ProcessorModel<"apple-a13", CycloneModel, ProcessorFeatures.AppleA13,
                     [TuneAppleA13]>;
def : ProcessorModel<"apple-a14", CycloneModel, ProcessorFeatures.AppleA14,
                     [TuneAppleA14]>;
def : ProcessorModel<"apple-a15", CycloneModel, ProcessorFeatures.AppleA15,
                     [TuneAppleA15]>;
def : ProcessorModel<"apple-a16", CycloneModel, ProcessorFeatures.AppleA16,
                     [TuneAppleA16]>;

// Mac CPUs
def : ProcessorModel<"apple-m1", CycloneModel, ProcessorFeatures.AppleA14,
                     [TuneAppleA14]>;
def : ProcessorModel<"apple-m2", CycloneModel, ProcessorFeatures.AppleA15,
                     [TuneAppleA15]>;

// watch CPUs.
def : ProcessorModel<"apple-s4", CycloneModel, ProcessorFeatures.AppleA12,
                     [TuneAppleA12]>;
def : ProcessorModel<"apple-s5", CycloneModel, ProcessorFeatures.AppleA12,
                     [TuneAppleA12]>;

// Alias for the latest Apple processor model supported by LLVM.
def : ProcessorModel<"apple-latest", CycloneModel, ProcessorFeatures.AppleA16,
                     [TuneAppleA16]>;

// Fujitsu A64FX
def : ProcessorModel<"a64fx", A64FXModel, ProcessorFeatures.A64FX,
                     [TuneA64FX]>;

// Nvidia Carmel
def : ProcessorModel<"carmel", NoSchedModel, ProcessorFeatures.Carmel,
                     [TuneCarmel]>;

// Ampere Computing
def : ProcessorModel<"ampere1", Ampere1Model, ProcessorFeatures.Ampere1,
                     [TuneAmpere1]>;

def : ProcessorModel<"ampere1a", Ampere1Model, ProcessorFeatures.Ampere1A,
                     [TuneAmpere1A]>;
                     