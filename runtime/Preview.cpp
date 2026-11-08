
#include "Preview.h"

#include <stdexcept>
#include <utility>

namespace gash {

namespace {

PreviewProvider& previewProviderStorage() {
    static PreviewProvider provider;
    return provider;
}

} // anonymous namespace

void setPreviewProvider(PreviewProvider provider) {
    previewProviderStorage() = std::move(provider);
}

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
