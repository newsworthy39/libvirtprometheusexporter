#include <memory>
#include <string>
#include <stdexcept>

namespace custom
{
    template <typename... Args>
    std::string format(const std::string &format, Args... args)
    {
        int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) + 1; // Extra space for '\0'
        if (size_s <= 0)
        {
            throw std::runtime_error("Error during formatting.");
        }
        auto size = static_cast<size_t>(size_s);
        std::unique_ptr<char[]> buf(new char[size]);
        std::snprintf(buf.get(), size, format.c_str(), args...);
        return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
    }

    /**
     * @brief Helper to generate response
     *
     * @param reply
     * @param response
     * @param version
     * @return std::string
     */
    std::string generate_prometheus(std::string reply, int response = 200, std::string version = "HTTP/1.0")
    {
        std::string type = "text/plain; version=0.0.4";
        std::string status = "200 OK";
        return custom::format("%s %s\r\nContent-Length: %d\r\nContent-Type: %s\r\n\r\n%s", version.c_str(),
                              status.c_str(), reply.length(), type.c_str(), reply.c_str());
    }

    std::vector<std::string> split(const std::string &s, std::string delimiter = ".")
    {
        // for string delimiter
        size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;

        while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos)
        {
            token = s.substr(pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back(token);
        }

        res.push_back(s.substr(pos_start));

        return res;
    }
}