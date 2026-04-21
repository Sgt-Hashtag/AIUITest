#pragma once
#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QJsonArray>

class AiController : public QObject {
    Q_OBJECT
public:
    explicit AiController(QGraphicsScene* scene, QObject* parent = nullptr);
    
    // Call this when user types a prompt
    void processPrompt(const QString& prompt);

private:
    QGraphicsScene* m_scene;
    
    // Simulates LLM output for demo purposes
    QJsonArray mockLlmResponse(const QString& prompt);
    
    // Executes the JSON commands on the Qt Scene
    void executeCommands(const QJsonArray& commands);
};