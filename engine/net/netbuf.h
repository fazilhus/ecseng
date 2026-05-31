#pragma once
#include <cstdlib>


namespace Core {

    class netbuf {
    public:
        netbuf(const std::size_t max_size = 0x1000) {
            m_buffer = static_cast<char*>(malloc(max_size));
            m_max_size = max_size;
            m_size = 0;
        }

        ~netbuf() {
            free(m_buffer);
        }

        template<typename T>
        [[nodiscard]] bool write(const T& value) {
            if (m_size + sizeof(T) > m_max_size) return false;
            memcpy(m_buffer + m_size, &value, sizeof(T));
            m_size += sizeof(T);
            return true;
        }

        void reset() {
            m_size = 0;
        }

        char* m_buffer;
        std::size_t m_max_size;
        std::size_t m_size;
    };

    class netrdbuf {
    public:
        netrdbuf(char* buffer, const std::size_t size) : m_buffer(buffer), m_size(size), m_cursor(0) {
        }

        ~netrdbuf() {
            m_buffer = nullptr;
            m_size = 0;
            m_cursor = 0;
        }

        template<typename T>
        [[nodiscard]] bool read(T& out) {
            if (m_cursor + sizeof(T) > m_size) return false;
            out = *reinterpret_cast<T*>(m_buffer + m_cursor);
            m_cursor += sizeof(T);
            return true;
        }

        char* m_buffer;
        std::size_t m_size;
        std::size_t m_cursor;
    };

} // namespace core