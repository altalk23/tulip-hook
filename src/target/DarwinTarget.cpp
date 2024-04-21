#include "DarwinTarget.hpp"

#include <Platform.hpp>

using namespace tulip::hook;

#if defined(TULIP_HOOK_DARWIN)

#include <mach/mach.h>
#include <mach/mach_init.h> /* mach_task_self()     */
#include <mach/mach_port.h>
#include <mach/vm_map.h> /* vm_allocate()        */
#include <mach/task.h>
#include <iostream>

Result<> DarwinTarget::allocatePage() {
	kern_return_t status;
	vm_address_t ret;

	status = vm_allocate(mach_task_self(), &ret, static_cast<vm_size_t>(PAGE_MAX_SIZE), VM_FLAGS_ANYWHERE);

	if (status != KERN_SUCCESS) {
		return Err("Couldn't allocate page");
	}

	m_allocatedPage = reinterpret_cast<void*>(ret);
	m_currentOffset = 0;
	m_remainingOffset = PAGE_MAX_SIZE;

	return this->protectMemory(m_allocatedPage, PAGE_MAX_SIZE, VM_PROT_READ | VM_PROT_EXECUTE);
}

Result<uint32_t> DarwinTarget::getProtection(void* address) {
	auto const pageSize = PAGE_SIZE;
	auto const pageMask = pageSize - 1;

	auto const ptr = reinterpret_cast<uintptr_t>(address);
	auto const alignedPtr = ptr & (~pageMask);

	kern_return_t status;
	vm_size_t vmsize;
	vm_address_t vmaddress = reinterpret_cast<vm_address_t>(alignedPtr);
	vm_region_basic_info_data_t info;
	mach_msg_type_number_t infoCount = VM_REGION_BASIC_INFO_COUNT_64;
	mach_port_t object;

	status = vm_region_64(
		mach_task_self(),
		&vmaddress,
		&vmsize,
		VM_REGION_BASIC_INFO_64,
		reinterpret_cast<vm_region_info_t>(&info),
		&infoCount,
		&object
	);

	if (status != KERN_SUCCESS) {
		return Err("Couldn't get protection");
	}

	return Ok(info.protection);
}

Result<> DarwinTarget::protectMemory(void* address, size_t size, uint32_t protection) {
	auto const pageSize = PAGE_SIZE;
	auto const pageMask = pageSize - 1;

	auto const ptr = reinterpret_cast<uintptr_t>(address);
	auto const alignedPtr = ptr & (~pageMask);
	auto const beginSize = ptr - alignedPtr;
	auto const pageCount = (beginSize + size + pageMask) / pageSize;
	auto const alignedSize = pageCount * pageSize;

	std::cout << "Using " << size << " bytes at " << address << " with " << protection << " max size " << PAGE_SIZE << std::endl;
	std::cout << "Protecting " << alignedSize << " bytes at " << (void*)alignedPtr << " with " << protection << std::endl;

	kern_return_t status;

	status = vm_protect(mach_task_self(), reinterpret_cast<vm_address_t>(alignedPtr), alignedSize, false, protection);

	if (status != KERN_SUCCESS) {
		return Err("Couldn't protect memory");
	}
	return Ok();
}

Result<> DarwinTarget::rawWriteMemory(void* destination, void const* source, size_t size) {
	kern_return_t status;
	std::cout << "Writing " << size << " bytes to " << destination << std::endl;

	TULIP_HOOK_UNWRAP_INTO(auto protection, this->getProtection(destination));

	std::cout << "Protection: " << protection << std::endl;

	TULIP_HOOK_UNWRAP(this->protectMemory(destination, size, this->getWritableProtection()));

	std::cout << "Protected" << std::endl;

	TULIP_HOOK_UNWRAP_INTO(auto newprotection, this->getProtection(destination));

	std::cout << "New Protection: " << newprotection << std::endl;

	status = vm_write(
		mach_task_self(),
		reinterpret_cast<vm_address_t>(destination),
		reinterpret_cast<vm_offset_t>(source),
		static_cast<mach_msg_type_number_t>(size)
	);
	std::cout << "Status: " << status << std::endl;

	if (status != KERN_SUCCESS) {
		return Err("Couldn't write memory");
	}
	TULIP_HOOK_UNWRAP(this->protectMemory(destination, size, protection));
	std::cout << "Wrote " << size << " bytes to " << destination << std::endl;
	return Ok();
}

uint32_t DarwinTarget::getWritableProtection() {
	return VM_PROT_COPY | VM_PROT_READ | VM_PROT_WRITE;
}

#endif