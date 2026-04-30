#include "AiController.h"
#include "LlamaEngine.h"
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QBrush>
#include <QDebug>
#include <QPen>
#include <QRandomGenerator>
#include <numeric>
#include <cmath>

namespace {
enum class ShapeTarget {
    All,
    Rectangles,
    Circles
};

ShapeTarget targetFromPrompt(const QString& prompt) {
    const QString lower = prompt.toLower();
    if (lower.contains("rect") || lower.contains("rectangle") ||
        lower.contains("square") || lower.contains("box")) {
        return ShapeTarget::Rectangles;
    }
    if (lower.contains("circle") || lower.contains("ellipse") || lower.contains("oval")) {
        return ShapeTarget::Circles;
    }
    return ShapeTarget::All;
}

bool itemMatchesTarget(QGraphicsItem* item, ShapeTarget target) {
    if (target == ShapeTarget::All) {
        return qgraphicsitem_cast<QGraphicsRectItem*>(item) ||
               qgraphicsitem_cast<QGraphicsEllipseItem*>(item);
    }
    if (target == ShapeTarget::Rectangles) {
        return qgraphicsitem_cast<QGraphicsRectItem*>(item) != nullptr;
    }
    return qgraphicsitem_cast<QGraphicsEllipseItem*>(item) != nullptr;
}

void moveItems(QGraphicsScene* scene, ShapeTarget target, qreal dx, qreal dy) {
    int changed = 0;
    for (QGraphicsItem* item : scene->items()) {
        if (itemMatchesTarget(item, target)) {
            item->moveBy(dx, dy);
            ++changed;
        }
    }
    qDebug() << "[EXEC] Moved items:" << changed;
}

void colorItems(QGraphicsScene* scene, ShapeTarget target, const QColor& color) {
    int changed = 0;
    for (QGraphicsItem* item : scene->items()) {
        if (target != ShapeTarget::Circles) {
            if (auto* rect = qgraphicsitem_cast<QGraphicsRectItem*>(item)) {
                QBrush b = rect->brush();
                b.setColor(color);
                rect->setBrush(b);
                ++changed;
                continue;
            }
        }

        if (target != ShapeTarget::Rectangles) {
            if (auto* ell = qgraphicsitem_cast<QGraphicsEllipseItem*>(item)) {
                QBrush b = ell->brush();
                b.setColor(color);
                ell->setBrush(b);
                ++changed;
            }
        }
    }
    qDebug() << "[EXEC] Recolored items:" << changed;
}
}

AiController::AiController(QGraphicsScene* scene, QObject* parent) : QObject(parent), m_scene(scene) {
    // Load the EMBEDDING model
    if (!LlamaEngine::instance().loadEmbeddingModel("models/msmarco-MiniLM-L6-v3.i1-Q6_K.gguf")) {
        qCritical() << "Failed to load Embedding Model!";
    } else {
        qDebug() << "Embedding Model Ready. Dimension:" << LlamaEngine::instance().getDimension();
    }

     m_intents = {
        {"add_circle",   "Add a new blue circle shape to the canvas"},
        {"add_rect",     "Add a new green rectangle shape to the canvas"},
        {"move_right",   "Move all shapes on the canvas to the right by 50 pixels"},
        {"move_left",    "Move all shapes on the canvas to the left by 50 pixels"},
        {"move_up",      "Move all shapes on the canvas up by 50 pixels"},
        {"move_down",    "Move all shapes on the canvas down by 50 pixels"},
        {"color_red",    "Change the color of all shapes to red"},
        {"color_blue",   "Change the color of all shapes to blue"},
        {"color_green",  "Change the color of all shapes to green"},
        {"clear_all",    "Delete all shapes from the canvas"}
    };
    
    // Pre-compute Intent Vectors (Optimization)
    precomputeIntentVectors();
}

void AiController::precomputeIntentVectors() {
    qDebug() << "[AI] Pre-computing intent vectors...";
    for (auto& intent : m_intents) {
        intent.vector = LlamaEngine::instance().getEmbedding(intent.description);
        if (intent.vector.empty()) {
            qWarning() << "[AI] Failed to generate vector for intent:" << intent.name.c_str();
        }
    }
    qDebug() << "[AI] Intents ready.";
}

float AiController::cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size()) return 0.0f;
    // Since vectors are already L2-normalized in LlamaEngine, dot product == cosine similarity
    return std::inner_product(a.begin(), a.end(), b.begin(), 0.0f);
}

void AiController::processPrompt(const QString& prompt) {
    qDebug() << "AI Received Prompt:" << prompt;
    
    std::vector<float> userVec = LlamaEngine::instance().getEmbedding(prompt.toStdString());
    
    if (userVec.empty()){
         qWarning() << "[AI] Failed to generate user embedding.";
        return;
    }

    const QString lowerPrompt = prompt.toLower();
    const bool wantsAdd = lowerPrompt.contains("add") ||
                          lowerPrompt.contains("create") ||
                          lowerPrompt.contains("draw");
    const bool wantsMove = lowerPrompt.contains("move") ||
                           lowerPrompt.contains("shift") ||
                           lowerPrompt.contains("drag");
    const bool wantsColor = lowerPrompt.contains("color") ||
                            lowerPrompt.contains("paint") ||
                            lowerPrompt.contains("make") ||
                            lowerPrompt.contains("change");

    // Find Best Match
    float bestScore = -1.0f;
    float secondBestScore = -1.0f;
    std::string bestAction = "";
    
    for (const auto& intent : m_intents) {
        float score = cosineSimilarity(userVec, intent.vector);

        if (intent.name == "add_circle" && wantsAdd &&
            (lowerPrompt.contains("circle") || lowerPrompt.contains("ellipse") || lowerPrompt.contains("oval"))) {
            score += 0.45f;
        } else if (intent.name == "add_rect" && wantsAdd &&
                   (lowerPrompt.contains("rect") || lowerPrompt.contains("rectangle") ||
                    lowerPrompt.contains("square") || lowerPrompt.contains("box"))) {
            score += 0.45f;
        } else if (intent.name == "move_right" && wantsMove && lowerPrompt.contains("right")) {
            score += 0.45f;
        } else if (intent.name == "move_left" && wantsMove && lowerPrompt.contains("left")) {
            score += 0.45f;
        } else if (intent.name == "move_up" && wantsMove && lowerPrompt.contains("up")) {
            score += 0.45f;
        } else if (intent.name == "move_down" && wantsMove && lowerPrompt.contains("down")) {
            score += 0.45f;
        } else if (intent.name == "color_red" && wantsColor && lowerPrompt.contains("red")) {
            score += 0.45f;
        } else if (intent.name == "color_blue" && wantsColor && lowerPrompt.contains("blue")) {
            score += 0.45f;
        } else if (intent.name == "color_green" && wantsColor && lowerPrompt.contains("green")) {
            score += 0.45f;
        } else if (intent.name == "clear_all" &&
                   (lowerPrompt.contains("clear") || lowerPrompt.contains("delete") || lowerPrompt.contains("remove"))) {
            score += 0.45f;
        }

        qDebug() << "[Match]" << intent.name.c_str() << ":" << score;
        
        if (score > bestScore) {
            secondBestScore = bestScore;
            bestScore = score;
            bestAction = intent.name;
        } else if (score > secondBestScore) {
            secondBestScore = score;
        }
    }
    const float THRESHOLD = 0.40f;
    const float MIN_MARGIN = 0.02f;
    
    // Threshold & Execute
    if (bestScore >= THRESHOLD && (bestScore - secondBestScore) >= MIN_MARGIN) {
        qDebug() << "[AI] Best Match:" << bestAction.c_str() << "(Score:" << bestScore << ")";
        executeAction(bestAction, prompt);
    } else {
        qDebug() << "[AI] No confident match found. Best Score:" << bestScore
                 << "Second Best:" << secondBestScore;
    }
}


void AiController::executeAction(const std::string& intentName, const QString& prompt) {
    qDebug() << "[EXEC] Triggering action:" << intentName.c_str();
    const ShapeTarget target = targetFromPrompt(prompt);

    if (intentName == "add_circle") {
        qreal x = 100 + QRandomGenerator::global()->bounded(400);
        qreal y = 100 + QRandomGenerator::global()->bounded(300);
        auto* circle = m_scene->addEllipse(x, y, 80, 80, QPen(Qt::black), QBrush(Qt::blue));
        circle->setFlag(QGraphicsItem::ItemIsMovable);
    }
    else if (intentName == "add_rect") {
        qreal x = 100 + QRandomGenerator::global()->bounded(400);
        qreal y = 100 + QRandomGenerator::global()->bounded(300);
        auto* rect = m_scene->addRect(x, y, 100, 60, QPen(Qt::black), QBrush(Qt::green));
        rect->setFlag(QGraphicsItem::ItemIsMovable);
    }
    else if (intentName == "move_right") {
        moveItems(m_scene, target, 50, 0);
    }
    else if (intentName == "move_left") {
        moveItems(m_scene, target, -50, 0);
    }
    else if (intentName == "move_up") {
        moveItems(m_scene, target, 0, -50);
    }
    else if (intentName == "move_down") {
        moveItems(m_scene, target, 0, 50);
    }
    else if (intentName == "color_red") {
        colorItems(m_scene, target, QColor("#FF0000"));
    }
    else if (intentName == "color_blue") {
        colorItems(m_scene, target, QColor("#0000FF"));
    }
    else if (intentName == "color_green") {
        colorItems(m_scene, target, QColor("#00FF00"));
    }
    else if (intentName == "clear_all") {
        m_scene->clear();
    }
}
