#pragma once
#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

class AiController;

class ShapeWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit ShapeWindow(QWidget *parent = nullptr);
    ~ShapeWindow();

private slots:
    void handlePrompt();
    void addRect();
    void addCircle();

private:
    QGraphicsScene *m_scene;
    QGraphicsView *m_view;
    QLineEdit *m_promptInput;
    QLabel *m_statusLabel;
    AiController *m_aiController;
};