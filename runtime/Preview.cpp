
#include "Preview.h"

#include <stdexcept>
#include <tuple>
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
