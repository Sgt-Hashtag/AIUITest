#include "ShapeWindow.h"
#include "AiController.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QBrush>
#include <QPen>

ShapeWindow::ShapeWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 1. Setup Scene & View
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, 800, 600);
    
    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing);

    // 2. Add Initial Shapes (So AI has something to work with)
    auto* rect = m_scene->addRect(100, 100, 100, 100, QPen(Qt::black), QBrush(Qt::gray));
    rect->setFlag(QGraphicsItem::ItemIsMovable);
    
    auto* circle = m_scene->addEllipse(300, 100, 100, 100, QPen(Qt::black), QBrush(Qt::blue));
    circle->setFlag(QGraphicsItem::ItemIsMovable);

    // 3. Setup AI Controller
    m_aiController = new AiController(m_scene, this);

    // 4. Setup UI Layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    mainLayout->addWidget(m_view);

    // Input Bar
    QHBoxLayout *inputLayout = new QHBoxLayout();
    m_promptInput = new QLineEdit(this);
    m_promptInput->setPlaceholderText("Try: 'make rects red', 'move right', 'add circle'");
    
    QPushButton *sendBtn = new QPushButton("Send", this);
    m_statusLabel = new QLabel("Ready", this);

    inputLayout->addWidget(m_promptInput);
    inputLayout->addWidget(sendBtn);
    inputLayout->addWidget(m_statusLabel);
    
    mainLayout->addLayout(inputLayout);

    setCentralWidget(centralWidget);
    setWindowTitle("AI Shape Demo");
    resize(900, 700);

    // 5. Connections
    connect(sendBtn, &QPushButton::clicked, this, &ShapeWindow::handlePrompt);
    connect(m_promptInput, &QLineEdit::returnPressed, this, &ShapeWindow::handlePrompt);
}

ShapeWindow::~ShapeWindow() {}

void ShapeWindow::handlePrompt() {
    QString text = m_promptInput->text();
    if (!text.isEmpty()) {
        m_statusLabel->setText("Processing...");
        m_aiController->processPrompt(text);
        m_statusLabel->setText("Done");
        m_promptInput->clear();
    }
}

void ShapeWindow::addRect() {
    m_scene->addRect(50, 50, 50, 50, QPen(Qt::black), QBrush(Qt::green));
}

void ShapeWindow::addCircle() {
    m_scene->addEllipse(50, 50, 50, 50, QPen(Qt::black), QBrush(Qt::yellow));
}