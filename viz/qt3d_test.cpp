#include <QtWidgets/QApplication>
#include <Qt3DCore/QEntity>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DCore/QTransform>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    Qt3DExtras::Qt3DWindow window;
    window.defaultFrameGraph()->setClearColor(Qt::black);

    auto *sceneRoot = new Qt3DCore::QEntity();
    auto *sphereMesh = new Qt3DExtras::QSphereMesh();
    sphereMesh->setRadius(1.0f);

    auto *material = new Qt3DExtras::QPhongMaterial();
    material->setDiffuse(Qt::red);

    auto *transform = new Qt3DCore::QTransform();
    transform->setScale(1.5f);

    auto *sphereEntity = new Qt3DCore::QEntity(sceneRoot);
    sphereEntity->addComponent(sphereMesh);
    sphereEntity->addComponent(material);
    sphereEntity->addComponent(transform);

    window.setRootEntity(sceneRoot);
    window.show();

    return app.exec();
}