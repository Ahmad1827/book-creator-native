#include <QApplication>
#include <QDirIterator>
#include <QFontDatabase>
#include "main_window.h"
#include "sketch.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // The story fonts ship inside the app so books look the same on any computer.
    QDirIterator fonts(":/fonts", {"*.ttf"});
    while (fonts.hasNext()) {
        QFontDatabase::addApplicationFont(fonts.next());
    }

    Sketch::install();

    MainWindow window;
    window.show();

    return app.exec();
}
