#pragma once
#include <QObject>
#include <QGraphicsScene>
#include <vector>
#include <string>

// Structure to hold an Intent and its pre-computed vector
struct SemanticIntent {
    std::string name;       // e.g., "add_circle"
    std::string description;// e.g., "Add a new blue circle to the canvas"
    std::vector<float> vector; // The 384-dim embedding
};

class AiController : public QObject {
    Q_OBJECT
public:
    explicit AiController(QGraphicsScene* scene, QObject* parent = nullptr);
    
    // Call this when user types a prompt
    void processPrompt(const QString& prompt);

private:
    QGraphicsScene* m_scene;
    std::vector<SemanticIntent> m_intents;

    void precomputeIntentVectors();
    
    // Helper: Execute logic based on the matched intent name
    void executeAction(const std::string& intentName, const QString& prompt);
    
    // Helper: Math utility for Cosine Similarity
    float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b);
};
