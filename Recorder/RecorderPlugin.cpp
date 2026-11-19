#include "Recorder.h"
#include "RecorderPlugin.h"

#include <QtCore/QtPlugin>

RecorderPlugin::RecorderPlugin(QObject *parent)
    : QObject(parent)
{
    initialized = false;
}

void RecorderPlugin::initialize(QDesignerFormEditorInterface * /*core*/)
{
    if (initialized)
        return;

    initialized = true;
}

bool RecorderPlugin::isInitialized() const
{
    return initialized;
}

QWidget *RecorderPlugin::createWidget(QWidget *parent)
{
    return new Recorder(parent);
}

QString RecorderPlugin::name() const
{
    return "Recorder";
}

QString RecorderPlugin::group() const
{
    return "My Plugins";
}

QIcon RecorderPlugin::icon() const
{
    return QIcon();
}

QString RecorderPlugin::toolTip() const
{
    return QString();
}

QString RecorderPlugin::whatsThis() const
{
    return QString();
}

bool RecorderPlugin::isContainer() const
{
    return false;
}

QString RecorderPlugin::domXml() const
{
    return "<widget class=\"Recorder\" name=\"Recorder\">\n"
        " <property name=\"geometry\">\n"
        "  <rect>\n"
        "   <x>0</x>\n"
        "   <y>0</y>\n"
        "   <width>100</width>\n"
        "   <height>100</height>\n"
        "  </rect>\n"
        " </property>\n"
        "</widget>\n";
}

QString RecorderPlugin::includeFile() const
{
    return "Recorder.h";
}
