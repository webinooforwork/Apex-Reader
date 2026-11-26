#include "../core/capability_registry.h"
#include "../engines/text_document.h"
#include <QFile>
#include <QTemporaryDir>
#include <cassert>

int main() {
    const auto* text = apex::CapabilityRegistry::find(".TXT");
    assert(text != nullptr);
    assert(text->support == apex::Support::Viewing);
    assert(apex::CapabilityRegistry::find("unknown") == nullptr);

    QTemporaryDir directory;
    assert(directory.isValid());
    const QString path = directory.filePath("sample.txt");
    QFile file(path);
    assert(file.open(QIODevice::WriteOnly));
    file.write("APEX reads local text.\nNo upload happens.");
    file.close();

    apex::TextDocument document;
    assert(document.load(path));
    assert(document.text.startsWith("APEX reads local text."));
    assert(document.words() == 7);

    apex::TextDocument missing;
    assert(!missing.load(directory.filePath("missing.txt")));
    assert(!missing.error.isEmpty());
    return 0;
}
