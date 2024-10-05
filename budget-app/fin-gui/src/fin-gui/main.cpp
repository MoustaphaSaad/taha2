#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlExtensionPlugin>

Q_IMPORT_QML_PLUGIN(EInkPlugin)

int main(int argc, char* argv[])
{
	QGuiApplication app(argc, argv);

	QQmlApplicationEngine engine;
	engine.addImportPath("qrc:/");
	QObject::connect(
		&engine,
		&QQmlApplicationEngine::objectCreationFailed,
		&app,
		[]() { QCoreApplication::exit(-1); },
		Qt::QueuedConnection);
	engine.loadFromModule("Fin.GUI", "Main");

	return app.exec();
}
