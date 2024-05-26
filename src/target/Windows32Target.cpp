#include "Windows32Target.hpp"

#include <Platform.hpp>
#include <stdexcept>

using namespace tulip::hook;

#if defined(TULIP_HOOK_WINDOWS)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

Result<> Windows32Target::allocatePage() {
	m_allocatedPage = VirtualAlloc(nullptr, 0x4000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READ);

	if (!m_allocatedPage) {
		return Err("Unable to allocate memory: " + std::to_string(GetLastError()));
	}

	m_currentOffset = 0;
	m_remainingOffset = 0x4000;

	return Ok();
}

Result<uint32_t> Windows32Target::getProtection(void* address) {
	MEMORY_BASIC_INFORMATION information;

	if (!VirtualQuery(address, &information, sizeof(MEMORY_BASIC_INFORMATION))) {
		return Err("Unable to query memory protection information");
	}

	return Ok(information.Protect);
}

Result<> Windows32Target::protectMemory(void* address, size_t size, uint32_t protection) {
	DWORD oldProtection;

	if (!VirtualProtect(address, size, protection, &oldProtection)) {
		return Err("Unable to apply memory protection");
	}

	return Ok();
}

Result<> Windows32Target::rawWriteMemory(void* destination, void const* source, size_t size) {
	if (!WriteProcessMemory(GetCurrentProcess(), destination, source, size, nullptr)) {
		return Err("Unable to write to memory");
	}
	return Ok();
}

uint32_t Windows32Target::getWritableProtection() {
	return PAGE_READWRITE;
}

Target& Target::get() {
	static Windows32Target ret;
	return ret;
}

Result<csh> Windows32Target::openCapstone() {
	cs_err status;

	status = cs_open(CS_ARCH_X86, CS_MODE_32, &m_capstone);
	if (status != CS_ERR_OK) {
		return Err("Couldn't open capstone");
	}

	return Ok(m_capstone);
}

std::unique_ptr<HandlerGenerator> Windows32Target::getHandlerGenerator(
	void* address, void* trampoline, void* handler, void* content, void* wrapped, HandlerMetadata const& metadata
) {
	return std::make_unique<X86HandlerGenerator>(address, trampoline, handler, content, wrapped, metadata);
}

std::unique_ptr<WrapperGenerator> Windows32Target::getWrapperGenerator(void* address, WrapperMetadata const& metadata) {
	return std::make_unique<X86WrapperGenerator>(address, metadata);
}

#endif