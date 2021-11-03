#include "providers/mem_provider.hpp"

#include <ctime>
#include <cstring>

#include <hex/helpers/utils.hpp>
#include <hex/helpers/file.hpp>
#include "helpers/project_file_handler.hpp"

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

namespace hex::prv {

    MemProvider::MemProvider(std::string path) : Provider(), m_path(std::move(path)) {
        this->open();
    }

    MemProvider::~MemProvider() {
        this->close();
    }

    bool MemProvider::isAvailable() const {
        return true;
    }

    bool MemProvider::isReadable() const {
        return isAvailable();
    }

    bool MemProvider::isWritable() const {
        return false;
    }

    bool MemProvider::isResizable() const {
        return false;
    }

    bool MemProvider::isSavable() const {
        return false;
    }

    void MemProvider::read(u64 offset, void *buffer, size_t size, bool overlays) {

        if ((offset - this->getBaseAddress()) > (this->getSize() - size) || buffer == nullptr || size == 0)
            return;

        auto const address = PageSize * this->m_currPage + offset - this->getBaseAddress();
        auto const pointer = (void*)(uintptr_t)(uint32_t)address;

        ReadProcessMemory(m_handle, pointer, buffer, size, nullptr);

        for (u64 i = 0; i < size; i++)
            if (getPatches().contains(offset + i))
                reinterpret_cast<u8*>(buffer)[i] = getPatches()[offset + PageSize * this->m_currPage + i];

        if (overlays)
            this->applyOverlays(offset, buffer, size);
    }

    void MemProvider::write(u64 offset, const void *buffer, size_t size) {
        if (((offset - this->getBaseAddress()) + size) > this->getSize() || buffer == nullptr || size == 0)
            return;

        addPatch(offset, buffer, size);
    }

    void MemProvider::readRaw(u64 offset, void *buffer, size_t size) {
        offset -= this->getBaseAddress();

        if ((offset + size) > this->getSize() || buffer == nullptr || size == 0)
            return;

        auto const address = PageSize * this->m_currPage + offset;
        auto const pointer = (void*)(uintptr_t)(uint32_t)address;

        ReadProcessMemory(m_handle, pointer, buffer, size, nullptr);
    }

    void MemProvider::writeRaw(u64 offset, const void *buffer, size_t size) {}

    void MemProvider::save() {
        this->applyPatches();
    }

    void MemProvider::saveAs(const std::string &path) {
        // Does nothing
    }

    void MemProvider::resize(ssize_t newSize) {
        // Does nothing
    }

    size_t MemProvider::getActualSize() const {
        return 0x1'0000'0000;
    }

    std::string MemProvider::getName() const {
        return std::filesystem::path(this->m_path).filename().string();
    }

    std::vector<std::pair<std::string, std::string>> MemProvider::getDataInformation() const {
        std::vector<std::pair<std::string, std::string>> result;

        result.emplace_back("hex.builtin.provider.file.path"_lang, this->m_path);
        result.emplace_back("hex.builtin.provider.file.size"_lang, hex::toByteString(this->getActualSize()));

        return result;
    }

    void MemProvider::open() {
        close();

        std::filesystem::path const path = std::filesystem::path{m_path}.filename();
        HANDLE const snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return;
        }

        PROCESSENTRY32 entry = { .dwSize = sizeof(PROCESSENTRY32) };
        for (bool i = Process32First(snapshot, &entry); i; i = Process32Next(snapshot, &entry)) {
            if (path != entry.szExeFile) {
                continue;
            }

            constexpr DWORD NEEDED_ACCESS =  PROCESS_VM_READ | PROCESS_QUERY_INFORMATION | SYNCHRONIZE;
            m_handle = OpenProcess(NEEDED_ACCESS, false, entry.th32ProcessID);

            HMODULE module = {};
            DWORD size = {};
            EnumProcessModules(m_handle, &module, sizeof(module), &size);
            // setBaseAddress((uint32_t)(uintptr_t)module);

            break;
        }

        CloseHandle(snapshot);
    }

    void MemProvider::close() {
        if (m_handle != INVALID_HANDLE_VALUE) {
            CloseHandle(m_handle);
            m_handle = INVALID_HANDLE_VALUE;
        }
    }
}
