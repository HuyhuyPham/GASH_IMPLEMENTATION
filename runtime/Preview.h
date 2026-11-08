
#ifndef GASH_PREVIEW_H
#define GASH_PREVIEW_H

#include <functional>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

namespace gash {

// Snapshot of one runtime variable.
struct VariableSnapshot {
    std::string name;
    std::string type;
    std::string value;
    bool locked = false;
    std::string scope;
};

// Function that retrieves active runtime variables.
using PreviewProvider =
    std::function<std::vector<VariableSnapshot>()>;

// Register or clear the snapshot provider.
void setPreviewProvider(PreviewProvider provider);

// Display an existing snapshot.
void previewSnapshot(
    const std::vector<VariableSnapshot>& variables,
    std::ostream& out = std::cout
);

// Get the latest snapshot and display it.
void preview(std::ostream& out = std::cout);

} // namespace gash

#endif // GASH_PREVIEW_H
