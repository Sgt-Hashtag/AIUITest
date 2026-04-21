#include "AiController.h"
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QBrush>
#include <QDebug>
#include <QRandomGenerator>
#include <QJsonDocument> // Required for JSON parsing
#include <QJsonObject>   // Required for QJsonObject class definition

AiController::AiController(QGraphicsScene* scene, QObject* parent)
    : QObject(parent), m_scene(scene) {}

void AiController::processPrompt(const QString& prompt) {
    qDebug() << "AI Received Prompt:" << prompt;
    
    // Simulate LLM response
    QJsonArray commands = mockLlmResponse(prompt);
    
    // Execute changes on the UI
    executeCommands(commands);
}

// Simulates what an LLM would return for specific prompts
QJsonArray AiController::mockLlmResponse(const QString& prompt) {
    QJsonArray cmds;
    qDebug() << "[AI] Analyzing prompt:" << prompt;

    QString lowerPrompt = prompt.toLower();

    // CHECK FOR TARGETS ---
    bool targetRect = lowerPrompt.contains("rect") || lowerPrompt.contains("square") || lowerPrompt.contains("box");
    bool targetEllipse = lowerPrompt.contains("circle") || lowerPrompt.contains("ellipse") || lowerPrompt.contains("oval");
    bool targetAll = lowerPrompt.contains("all") || (!targetRect && !targetEllipse); // Default to all if no specific shape mentioned? Let's be safe: Default to NONE unless specified.
    
    
    // COLOR CHANGE
    if (lowerPrompt.contains("red") || lowerPrompt.contains("blue") || lowerPrompt.contains("green")) {
        QColor color;
        if (lowerPrompt.contains("red")) color = QColor("#FF0000");
        else if (lowerPrompt.contains("blue")) color = QColor("#0000FF");
        else if (lowerPrompt.contains("green")) color = QColor("#00FF00");
        
        if (!color.isValid()) return cmds;

        // If user said "rects", only affect rects
        if (targetRect) {
            QJsonObject cmd;
            cmd["action"] = "setColor";
            cmd["target_type"] = "rect";
            cmd["color"] = color.name();
            cmds.append(cmd);
            qDebug() << "[AI] Command: Color Rects" << color.name();
        }
        
        // If user said "circles", only affect ellipses
        if (targetEllipse) {
            QJsonObject cmd;
            cmd["action"] = "setColor";
            cmd["target_type"] = "ellipse";
            cmd["color"] = color.name();
            cmds.append(cmd);
            qDebug() << "[AI] Command: Color Ellipses" << color.name();
        }
        
        // If user said "all" or just "make everything red"
        if (lowerPrompt.contains("all") || lowerPrompt.contains("everything")) {
             QJsonObject cmd;
            cmd["action"] = "setColor";
            cmd["target_type"] = "all";
            cmd["color"] = color.name();
            cmds.append(cmd);
            qDebug() << "[AI] Command: Color All" << color.name();
        }
        
        // Fallback: If they just said "red" with no shape, let's default to ALL for ease of use in demo
        if (!targetRect && !targetEllipse && !lowerPrompt.contains("all")) {
             QJsonObject cmd;
            cmd["action"] = "setColor";
            cmd["target_type"] = "all";
            cmd["color"] = color.name();
            cmds.append(cmd);
            qDebug() << "[AI] Command: Color All (Default)" << color.name();
        }
    }

    // MOVE
    if (lowerPrompt.contains("move")) {
        int dx = 0, dy = 0;
        if (lowerPrompt.contains("right")) dx = 50;
        if (lowerPrompt.contains("left")) dx = -50;
        if (lowerPrompt.contains("up")) dy = -50;
        if (lowerPrompt.contains("down")) dy = 50;

        if (dx != 0 || dy != 0) {
            QJsonObject cmd;
            cmd["action"] = "move";
            cmd["target_type"] = "all"; // Moving usually applies to selection or all
            cmd["dx"] = dx;
            cmd["dy"] = dy;
            cmds.append(cmd);
            qDebug() << "[AI] Command: Move (" << dx << "," << dy << ")";
        }
    }

    // ADD
    if (lowerPrompt.contains("add")) {
        if (targetEllipse || lowerPrompt.contains("circle")) {
            QJsonObject cmd;
            cmd["action"] = "add";
            cmd["shape"] = "ellipse";
            cmd["x"] = 100 + QRandomGenerator::global()->bounded(400);
            cmd["y"] = 100 + QRandomGenerator::global()->bounded(300);
            cmd["w"] = 80;
            cmd["h"] = 80;
            cmds.append(cmd);
            qDebug() << "[AI] Command: Add Circle";
        }
        else if (targetRect || lowerPrompt.contains("rect") || lowerPrompt.contains("square")) {
            QJsonObject cmd;
            cmd["action"] = "add";
            cmd["shape"] = "rect";
            cmd["x"] = 100 + QRandomGenerator::global()->bounded(400);
            cmd["y"] = 100 + QRandomGenerator::global()->bounded(300);
            cmd["w"] = 100;
            cmd["h"] = 60;
            cmds.append(cmd);
            qDebug() << "[AI] Command: Add Rect";
        }
    }

    return cmds;
}

void AiController::executeCommands(const QJsonArray& commands) {
    qDebug() << "Executing" << commands.size() << "commands:";
    
    for (const QJsonValue& val : commands) {
        QJsonObject cmd = val.toObject();
        QString action = cmd["action"].toString();
        qDebug() << "  Command:" << action;

        if (action == "setColor") {
            QString targetType = cmd["target_type"].toString();
            QString colorStr = cmd["color"].toString();
            QColor newColor(colorStr);
            qDebug() << "    Target:" << targetType << "Color:" << colorStr;

            int changedCount = 0;
            for (QGraphicsItem* item : m_scene->items()) {
                bool isTarget = false;
                
                if (targetType == "all") {
                    isTarget = true;
                } else if (targetType == "rect") {
                    QGraphicsRectItem* rectItem = qgraphicsitem_cast<QGraphicsRectItem*>(item);
                    if (rectItem) isTarget = true;
                } else if (targetType == "ellipse") {
                    QGraphicsEllipseItem* ellipseItem = qgraphicsitem_cast<QGraphicsEllipseItem*>(item);
                    if (ellipseItem) isTarget = true;
                }
                
                if (isTarget) {
                    if (auto* rectItem = qgraphicsitem_cast<QGraphicsRectItem*>(item)) {
                        QBrush b = rectItem->brush();
                        b.setColor(newColor);
                        rectItem->setBrush(b);
                        changedCount++;
                    } else if (auto* ellipseItem = qgraphicsitem_cast<QGraphicsEllipseItem*>(item)) {
                        QBrush b = ellipseItem->brush();
                        b.setColor(newColor);
                        ellipseItem->setBrush(b);
                        changedCount++;
                    }
                }
            }
            qDebug() << "    Changed" << changedCount << "items.";
        }
        else if (action == "move") {
            int dx = cmd["dx"].toInt();
            int dy = cmd["dy"].toInt();
            qDebug() << "    Moving all items by (" << dx << "," << dy << ")";
            for (QGraphicsItem* item : m_scene->items()) {
                item->moveBy(dx, dy);
            }
        }
        else if (action == "add") {
            QString shape = cmd["shape"].toString();
            qreal x = cmd["x"].toDouble();
            qreal y = cmd["y"].toDouble();
            qreal w = cmd["w"].toDouble();
            qreal h = cmd["h"].toDouble();
            qDebug() << "    Adding" << shape << "at (" << x << "," << y << ") size (" << w << "x" << h << ")";

            if (shape == "ellipse") {
                m_scene->addEllipse(x, y, w, h, QPen(Qt::black), QBrush(Qt::blue));
            } else {
                m_scene->addRect(x, y, w, h, QPen(Qt::black), QBrush(Qt::green));
            }
        }
    }
}