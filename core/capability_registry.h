#pragma once
#include <string>
#include <vector>
namespace apex {
enum class Support { Unsupported, Metadata, TextExtraction, Viewing, Editing, FullFidelity };
struct Capability { std::string extension; std::string format; Support support; std::string note; };
class CapabilityRegistry {
public:
 static const std::vector<Capability>& all();
 static const Capability* find(std::string extension);
 static std::string label(Support support);
};
}
