// src/main.cpp
#include "app/Application.h"
#include "ui/MainWindow.h"

int main(int argc, char** argv)
{
    Application app(argc, argv);

    MainWindow w;
    w.resize(1000, 700);
    w.show();

    return app.exec();
}
