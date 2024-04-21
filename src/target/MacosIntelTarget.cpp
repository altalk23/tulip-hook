#include "MacosIntelTarget.hpp"

#include <Platform.hpp>

using namespace tulip::hook;

#if defined(TULIP_HOOK_MACOS) && defined(TULIP_HOOK_X64)

Target& Target::get() {
	static MacosIntelTarget ret;
	return ret;
}

Result<csh> MacosIntelTarget::openCapstone() {
	cs_err status;

	status = cs_open(CS_ARCH_X86, CS_MODE_64, &m_capstone);
	if (status != CS_ERR_OK) {
		return Err("Couldn't open capstone");
	}

	return Ok(m_capstone);
}

std::unique_ptr<HandlerGenerator> MacosIntelTarget::getHandlerGenerator(
	void* address, void* trampoline, void* handler, void* content, void* wrapped, HandlerMetadata const& metadata
) {
	return std::make_unique<X64HandlerGenerator>(address, trampoline, handler, content, wrapped, metadata);
}

std::unique_ptr<WrapperGenerator> MacosIntelTarget::getWrapperGenerator(void* address, WrapperMetadata const& metadata) {
	return std::make_unique<X64WrapperGenerator>(address, metadata);
}

#endif