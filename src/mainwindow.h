#pragma once

#include "matrix.h"

#include <QMainWindow>

#include <functional>
#include <map>
#include <optional>
#include <string>

class QLabel;
class QTextEdit;

namespace la {

class MainWindow final : public QMainWindow {
public:
    MainWindow();

private:
    void execute(const std::function<void()> &action);
    std::optional<Matrix> inputMatrix(const QString &name,
                                      std::optional<int> fixedRows = {},
                                      std::optional<int> fixedCols = {});
    std::optional<Matrix> promptMatrix(const QString &name);
    void offerSave(const Matrix &matrix, const QString &suggestedName);
    void setResult(const QString &title, const std::string &body,
                   const std::optional<Matrix> &matrix = {});
    QString nextMatrixName() const;

    void expressionAction();
    void solveSystemAction();
    void consistencyAction();
    void solveAxBAction();
    void transposeAction();
    void inverseAction();
    void multiplicationAction();
    void luAction();
    void reductionAction();
    void determinantAction();
    void spacesAction();
    void eigenAction();
    void manageMatricesAction();
    void exportAction();

    std::map<std::string, Matrix> matrices_;
    QLabel *savedCount_ = nullptr;
    QLabel *resultTitle_ = nullptr;
    QTextEdit *resultText_ = nullptr;
    QString lastTitle_;
    QString lastOutput_;
};

} // namespace la
