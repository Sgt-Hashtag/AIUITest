#include <QApplication>
#include "ShapeWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    ShapeWindow window;
    window.show();
    return app.exec();
}