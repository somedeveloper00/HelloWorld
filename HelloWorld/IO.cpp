#include "IO.hpp"

std::string readFile(const std::string path)
{
    std::ifstream stream(path, std::ios::in);
    try
    {
        stream.exceptions(std::ios::failbit | std::ios::badbit);
        std::ostringstream ss;
        ss << stream.rdbuf();
        stream.close();
        return ss.str();
    }
    catch (const std::exception&)
    {
        std::cerr << "could not read file \"" << path << "\": " << std::endl;
        return "";
    }
}