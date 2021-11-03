#pragma once

#include <hex/providers/provider.hpp>

#include <string_view>


#if defined(OS_WINDOWS)
#include <windows.h>
#else
#pragma error "TODO!"
#endif

namespace hex::prv {

    class MemProvider : public Provider {
    public:
        explicit MemProvider(std::string path);
        ~MemProvider() override;

        bool isAvailable() const override;
        bool isReadable() const override;
        bool isWritable() const override;
        bool isResizable() const override;
        bool isSavable() const override;

        void read(u64 offset, void *buffer, size_t size, bool overlays) override;
        void write(u64 offset, const void *buffer, size_t size) override;
        void resize(ssize_t newSize) override;

        void readRaw(u64 offset, void *buffer, size_t size) override;
        void writeRaw(u64 offset, const void *buffer, size_t size) override;
        size_t getActualSize() const override;

        void save() override;
        void saveAs(const std::string &path) override;

        [[nodiscard]] std::string getName() const override;
        [[nodiscard]] std::vector<std::pair<std::string, std::string>> getDataInformation() const override;

    private:
        HANDLE m_handle = INVALID_HANDLE_VALUE;
        std::string m_path;

        void open();
        void close();
    };

}
