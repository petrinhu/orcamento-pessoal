#pragma once

#include <QIcon>

// Generates the PeuBud application icon at runtime via QPainter.
// Produces a multi-resolution QIcon with sizes 16/32/48/64/128/256.
class IconGenerator {
public:
    static QIcon generate();

private:
    static QPixmap render(int size);
};
