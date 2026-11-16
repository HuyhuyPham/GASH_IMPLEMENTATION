
// Implements preview() snapshot provider storage and formatted active-variable display.

#include "Preview.h"

#include <stdexcept>
#include <tuple>
#include <utility>

namespace gash {

namespace {

// Returns the singleton storage slot holding the registered PreviewProvider callback.
PreviewProvider& previewProviderStorage() {
    static PreviewProvider provider;
    return provider;
}

} // anonymous namespace

// Registers or clears the callback supplying live variable snapshots to preview().
void setPreviewProvider(PreviewProvider provider) {
    previewProviderStorage() = std::move(provider);
}

// Adapts a tuple-based snapshot provider callback into PreviewVariableSnapshot entries.
void setTuplePreviewProvider(
    std::function<std::vector<std::tuple<
        std::string, std::string, std::string, bool, std::string>>()> rawProvider
) {
    if (!rawProvider) {
        previewProviderStorage() = {};
        return;
    }
    previewProviderStorage() = [raw = std::move(rawProvider)]() {
        auto items = raw();
        std::vector<VariableSnapshot> out;
        out.reserve(items.size());
        for (auto& t : items) {
            out.push_back(VariableSnapshot{
                std::move(std::get<0>(t)),
                std::move(std::get<1>(t)),
                std::move(std::get<2>(t)),
                std::get<3>(t),
                std::move(std::get<4>(t))
            });
        }
        return out;
    };
}

// Prints a formatted summary of variable snapshots (name, type, value, lock, scope).
void previewSnapshot(
    const std::vector<VariableSnapshot>& variables,
    std::ostream& out
) {
    out << "Variables (" << variables.size() << "):\n";

    for (const auto& var : variables) {
        out << "  "
            << var.name
            << " : " << var.type
            << " = " << var.value
            << " ["
            << (var.locked ? "locked" : "unlocked")
            << "]";

        if (!var.scope.empty()) {
            out << " scope=" << var.scope;
        }

        out << '\n';
    }
}

// Queries the registered provider and renders active variable state to `out`.
void preview(std::ostream& out) {
    const auto& provider = previewProviderStorage();

    if (!provider) {
        throw std::logic_error(
            "preview() requires an Environment snapshot provider"
        );
    }

    previewSnapshot(provider(), out);
}

} // namespace gash
