#ifndef LLD_ELF_ARCH_RISCVGP_H
#define LLD_ELF_ARCH_RISCVGP_H

#include <cstdint>

namespace lld {
	namespace elf {

		class Ctx;
		class InputSectionBase;

		void collectRISCVGPSectionBenefits(Ctx &ctx);
		uint64_t getRISCVGPSectionBenefit(const InputSectionBase *sec);

		void optimizeRISCVGP(Ctx &ctx);

} // namespace elf
} // namespace lld

#endif
