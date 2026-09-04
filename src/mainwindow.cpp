#include "mainwindow.h"

#include "algorithms.h"
#include "expression.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFormLayout>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

#include <sstream>

namespace la {

namespace {

QString qtext(const std::string &value) { return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size())); }

QString joinedSteps(const std::vector<std::string> &steps)
{
    QStringList values;
    for (const auto &step : steps) values << qtext(step);
    return values.join("\n\n");
}

bool validMatrixName(const QString &name)
{
    static const QRegularExpression pattern(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*$"));
    return pattern.match(name).hasMatch();
}

} // namespace

MainWindow::MainWindow()
{
    setWindowTitle("Linear Algebra Helper");
    resize(1180, 760);
    setMinimumSize(900, 620);

    auto *central = new QWidget(this);
    auto *root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto *sidebar = new QWidget;
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(350);
    auto *sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(24, 24, 24, 24);
    sideLayout->setSpacing(9);

    auto *brand = new QLabel("LINEAR ALGEBRA HELPER");
    brand->setObjectName("brand");
    auto *subtitle = new QLabel("Symbolic matrix workshop");
    subtitle->setObjectName("subtitle");
    savedCount_ = new QLabel;
    savedCount_->setObjectName("savedCount");
    sideLayout->addWidget(brand);
    sideLayout->addWidget(subtitle);
    sideLayout->addSpacing(8);
    sideLayout->addWidget(savedCount_);

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *buttonHost = new QWidget;
    auto *buttons = new QVBoxLayout(buttonHost);
    buttons->setContentsMargins(0, 8, 4, 8);
    buttons->setSpacing(7);
    struct Operation { const char *label; void (MainWindow::*action)(); };
    const Operation operations[] = {
        {"01  Matrix expression", &MainWindow::expressionAction},
        {"02  Solve augmented system", &MainWindow::solveSystemAction},
        {"03  Parameter consistency", &MainWindow::consistencyAction},
        {"04  Solve Ax = b", &MainWindow::solveAxBAction},
        {"05  Transpose", &MainWindow::transposeAction},
        {"06  Inverse", &MainWindow::inverseAction},
        {"07  Multiply with steps", &MainWindow::multiplicationAction},
        {"08  LU factorization", &MainWindow::luAction},
        {"09  Row reduction", &MainWindow::reductionAction},
        {"10  Determinant", &MainWindow::determinantAction},
        {"11  Rank and spaces", &MainWindow::spacesAction},
        {"12  Eigen analysis", &MainWindow::eigenAction},
        {"13  Saved matrices", &MainWindow::manageMatricesAction},
        {"14  Export result", &MainWindow::exportAction},
    };
    for (const auto &operation : operations) {
        auto *button = new QPushButton(operation.label);
        button->setCursor(Qt::PointingHandCursor);
        connect(button, &QPushButton::clicked, this, [this, action = operation.action] {
            execute([this, action] { (this->*action)(); });
        });
        buttons->addWidget(button);
    }
    buttons->addStretch();
    scroll->setWidget(buttonHost);
    sideLayout->addWidget(scroll, 1);

    auto *workspace = new QWidget;
    workspace->setObjectName("workspace");
    auto *workspaceLayout = new QVBoxLayout(workspace);
    workspaceLayout->setContentsMargins(38, 32, 38, 32);
    resultTitle_ = new QLabel("Ready for a matrix");
    resultTitle_->setObjectName("resultTitle");
    resultText_ = new QTextEdit;
    resultText_->setReadOnly(true);
    resultText_->setObjectName("resultText");
    resultText_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    resultText_->setPlainText("Choose an operation from the left.\n\nEntries accept exact expressions such as 1/3, sqrt(2), pi, and x + 1.");
    workspaceLayout->addWidget(resultTitle_);
    workspaceLayout->addWidget(resultText_, 1);

    root->addWidget(sidebar);
    root->addWidget(workspace, 1);
    setCentralWidget(central);

    setStyleSheet(R"(
        QMainWindow { background: #ece9df; }
        #sidebar { background: #102d35; }
        #brand { color: #f3c969; font: 700 19px 'Segoe UI'; letter-spacing: 1px; }
        #subtitle { color: #a9c2c3; font: 13px 'Segoe UI'; }
        #savedCount { color: #f6efe0; background: #1b4149; border-radius: 5px; padding: 8px; }
        #sidebar QPushButton { color: #e9f0ed; background: transparent; border: 1px solid #416269;
            border-radius: 5px; text-align: left; padding: 9px 11px; font: 13px 'Segoe UI'; }
        #sidebar QPushButton:hover { background: #d36b43; border-color: #d36b43; color: white; }
        #sidebar QScrollArea, #sidebar QScrollArea > QWidget > QWidget { background: transparent; }
        #workspace { background: #ece9df; }
        #resultTitle { color: #173b43; font: 700 24px 'Georgia'; padding-bottom: 8px; }
        #resultText { color: #172a2f; background: #fffdf7; border: 1px solid #c9c4b7;
            border-radius: 6px; padding: 16px; selection-background-color: #d36b43; }
        QDialog { color: #172a2f; background: #f3f0e7; }
        QDialog QLabel { color: #172a2f; }
        QDialog QLineEdit, QDialog QSpinBox, QDialog QComboBox, QDialog QTableWidget,
        QDialog QListWidget, QDialog QTextEdit, QDialog QAbstractItemView {
            color: #172a2f; background: #fffdf7; border: 1px solid #aaa597;
            selection-color: white; selection-background-color: #d36b43;
        }
        QDialog QPushButton { color: #173b43; background: #fffdf7; border: 1px solid #aaa597;
            border-radius: 4px; padding: 6px 14px; }
        QDialog QPushButton:hover { background: #e5dfd1; }
        QDialog QPushButton:default { color: white; background: #d36b43; border-color: #b95532; }
    )");
    savedCount_->setText("0 matrices saved this session");
}

void MainWindow::execute(const std::function<void()> &action)
{
    try {
        action();
    } catch (const std::exception &error) {
        QMessageBox::critical(this, "Operation failed", qtext(error.what()));
    }
}

std::optional<Matrix> MainWindow::inputMatrix(const QString &name, std::optional<int> fixedRows, std::optional<int> fixedCols)
{
    bool ok = true;
    int rows = fixedRows.value_or(0);
    if (!fixedRows) {
        rows = QInputDialog::getInt(this, "Matrix dimensions", "Rows for " + name + ':', 3, 1, 20, 1, &ok);
        if (!ok) return {};
    }
    int cols = fixedCols.value_or(0);
    if (!fixedCols) {
        cols = QInputDialog::getInt(this, "Matrix dimensions", "Columns for " + name + ':', 3, 1, 20, 1, &ok);
        if (!ok) return {};
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Enter " + name);
    dialog.resize(std::max(460, cols * 115), std::max(300, rows * 55));
    auto *layout = new QVBoxLayout(&dialog);
    auto *instruction = new QLabel(QString("Enter each element of %1. Symbolic expressions and exact fractions are supported.").arg(name));
    instruction->setWordWrap(true);
    auto *table = new QTableWidget(rows, cols);
    table->horizontalHeader()->setDefaultSectionSize(105);
    table->verticalHeader()->setDefaultSectionSize(34);
    for (int row = 0; row < rows; ++row) {
        table->setVerticalHeaderItem(row, new QTableWidgetItem(QString::number(row + 1)));
        for (int col = 0; col < cols; ++col) table->setItem(row, col, new QTableWidgetItem("0"));
    }
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(instruction);
    layout->addWidget(table, 1);
    layout->addWidget(buttons);

    while (dialog.exec() == QDialog::Accepted) {
        try {
            Matrix matrix(static_cast<std::size_t>(rows), static_cast<std::size_t>(cols));
            for (int row = 0; row < rows; ++row) {
                for (int col = 0; col < cols; ++col) {
                    const QString entry = table->item(row, col) ? table->item(row, col)->text() : QString();
                    matrix.at(static_cast<std::size_t>(row), static_cast<std::size_t>(col)) = parseScalar(entry.toStdString());
                }
            }
            return matrix;
        } catch (const std::exception &error) {
            QMessageBox::warning(&dialog, "Invalid matrix entry", qtext(error.what()));
        }
    }
    return {};
}

std::optional<Matrix> MainWindow::promptMatrix(const QString &name)
{
    if (!matrices_.empty()) {
        QStringList choices{"<Enter a new matrix>"};
        for (const auto &[savedName, matrix] : matrices_) {
            Q_UNUSED(matrix);
            choices << qtext(savedName);
        }
        bool ok = false;
        const QString selected = QInputDialog::getItem(this, "Choose " + name,
            "Use a saved matrix or enter a new one:", choices, 0, false, &ok);
        if (!ok) return {};
        if (selected != choices.front()) return matrices_.at(selected.toStdString());
    }
    return inputMatrix(name);
}

void MainWindow::offerSave(const Matrix &matrix, const QString &suggestedName)
{
    if (QMessageBox::question(this, "Save matrix", "Save this matrix for later operations?") != QMessageBox::Yes) return;
    bool ok = false;
    QString name = QInputDialog::getText(this, "Save matrix", "Matrix name:", QLineEdit::Normal,
                                         matrices_.count(suggestedName.toStdString()) ? nextMatrixName() : suggestedName, &ok).trimmed();
    if (!ok) return;
    if (!validMatrixName(name)) {
        QMessageBox::warning(this, "Invalid name", "Use a letter or underscore first, followed by letters, digits, or underscores.");
        return;
    }
    matrices_[name.toStdString()] = matrix;
    savedCount_->setText(QString("%1 matrices saved this session").arg(matrices_.size()));
}

void MainWindow::setResult(const QString &title, const std::string &body, const std::optional<Matrix> &matrix)
{
    lastTitle_ = title;
    lastOutput_ = qtext(body);
    resultTitle_->setText(title);
    resultText_->setPlainText(lastOutput_);
    if (matrix) offerSave(*matrix, "R");
}

QString MainWindow::nextMatrixName() const
{
    for (char name = 'A'; name <= 'Z'; ++name) {
        const std::string candidate(1, name);
        if (!matrices_.count(candidate)) return qtext(candidate);
    }
    return QString("M%1").arg(matrices_.size() + 1);
}

void MainWindow::expressionAction()
{
    if (matrices_.empty()) {
        if (QMessageBox::question(this, "No saved matrices", "Add a matrix before evaluating an expression?") != QMessageBox::Yes) return;
        auto matrix = inputMatrix("A");
        if (!matrix) return;
        matrices_["A"] = *matrix;
        savedCount_->setText("1 matrix saved this session");
    }
    QStringList names;
    for (const auto &[name, matrix] : matrices_) { Q_UNUSED(matrix); names << qtext(name); }
    bool ok = false;
    const QString expression = QInputDialog::getMultiLineText(this, "Matrix expression",
        "Available: " + names.join(", ") + "\nExamples: A + B*3, A**2, det(A), inv(A), rref(A), A.T, A[0,1]",
        names.front(), &ok).trimmed();
    if (!ok || expression.isEmpty()) return;
    const auto value = evaluateExpression(expression.toStdString(), matrices_);
    const std::string body = "Expression:\n" + expression.toStdString() + "\n\nResult:\n" + expressionValueText(value);
    if (std::holds_alternative<Matrix>(value)) setResult("Matrix Expression", body, std::get<Matrix>(value));
    else setResult("Matrix Expression", body);
}

void MainWindow::solveSystemAction()
{
    const auto matrix = promptMatrix("augmented matrix");
    if (!matrix) return;
    const auto solution = solveAugmentedSystem(*matrix);
    QString body = "Row-reduction steps:\n\n" + joinedSteps(solution.steps) + "\n\nSolution summary:\n";
    for (const auto &line : solution.solutionLines) body += qtext(line) + '\n';
    setResult("System Solution", body.toStdString());
}

void MainWindow::consistencyAction()
{
    const auto matrix = promptMatrix("augmented matrix with parameters");
    if (matrix) setResult("Consistency Conditions", consistencyReport(*matrix));
}

void MainWindow::solveAxBAction()
{
    const auto a = promptMatrix("A");
    if (!a) return;
    const auto b = inputMatrix("b", static_cast<int>(a->rows()), 1);
    if (b) setResult("Solution Ax = b", solveAxBReport(*a, *b));
}

void MainWindow::transposeAction()
{
    const auto matrix = promptMatrix("A");
    if (!matrix) return;
    const Matrix result = transpose(*matrix);
    setResult("Matrix Transpose", "Original Matrix A:\n" + matrixText(*matrix) + "\n\nTranspose:\n" + matrixText(result), result);
}

void MainWindow::inverseAction()
{
    const auto matrix = promptMatrix("A");
    if (!matrix) return;
    const Matrix result = inverse(*matrix);
    setResult("Matrix Inverse", "Original Matrix A:\n" + matrixText(*matrix) + "\n\nInverse:\n" + matrixText(result), result);
}

void MainWindow::multiplicationAction()
{
    const auto a = promptMatrix("A");
    if (!a) return;
    const auto b = promptMatrix("B");
    if (!b) return;
    Matrix result;
    const auto report = multiplicationReport(*a, *b, &result);
    setResult("Matrix Multiplication", report, result);
}

void MainWindow::luAction()
{
    const auto matrix = promptMatrix("A");
    if (matrix) setResult("LU Factorization", luReport(*matrix));
}

void MainWindow::reductionAction()
{
    const auto matrix = promptMatrix("A");
    if (!matrix) return;
    QMessageBox choice(this);
    choice.setWindowTitle("Row reduction");
    choice.setText("Choose the target form.");
    auto *echelon = choice.addButton("Echelon form", QMessageBox::ActionRole);
    auto *rref = choice.addButton("RREF", QMessageBox::ActionRole);
    choice.addButton(QMessageBox::Cancel);
    choice.exec();
    if (choice.clickedButton() != echelon && choice.clickedButton() != rref) return;
    const bool toRref = choice.clickedButton() == rref;
    const auto reduction = rowReduce(*matrix, toRref);
    setResult(toRref ? "Reduced Row Echelon Form" : "Echelon Form",
              joinedSteps(reduction.steps).toStdString(), reduction.reduced);
}

void MainWindow::determinantAction()
{
    const auto matrix = promptMatrix("A");
    if (matrix) setResult("Determinant", "Matrix A:\n" + matrixText(*matrix) + "\n\ndet(A) = " + scalarText(determinant(*matrix)));
}

void MainWindow::spacesAction()
{
    const auto matrix = promptMatrix("A");
    if (matrix) setResult("Rank, Nullity, and Spaces", spacesReport(*matrix));
}

void MainWindow::eigenAction()
{
    const auto matrix = promptMatrix("A");
    if (matrix) setResult("Eigenvalues and Eigenvectors", eigenReport(*matrix));
}

void MainWindow::manageMatricesAction()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Saved matrices");
    dialog.resize(680, 480);
    auto *layout = new QVBoxLayout(&dialog);
    auto *list = new QListWidget;
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    auto refresh = [&] {
        list->clear();
        for (const auto &[name, matrix] : matrices_)
            list->addItem(qtext(name + "  (" + std::to_string(matrix.rows()) + " x " + std::to_string(matrix.cols()) + ")\n" + matrixText(matrix)));
        savedCount_->setText(QString("%1 matrices saved this session").arg(matrices_.size()));
    };
    auto *row = new QHBoxLayout;
    auto *addButton = new QPushButton("Add");
    auto *deleteButton = new QPushButton("Delete");
    auto *clearButton = new QPushButton("Clear all");
    auto *closeButton = new QPushButton("Close");
    row->addWidget(addButton); row->addWidget(deleteButton); row->addWidget(clearButton); row->addStretch(); row->addWidget(closeButton);
    layout->addWidget(list, 1); layout->addLayout(row);
    connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(addButton, &QPushButton::clicked, &dialog, [&] {
        bool ok = false;
        const QString name = QInputDialog::getText(&dialog, "Matrix name", "Name:", QLineEdit::Normal, nextMatrixName(), &ok).trimmed();
        if (!ok) return;
        if (!validMatrixName(name)) { QMessageBox::warning(&dialog, "Invalid name", "Enter a valid identifier."); return; }
        auto matrix = inputMatrix(name);
        if (matrix) { matrices_[name.toStdString()] = *matrix; refresh(); }
    });
    connect(deleteButton, &QPushButton::clicked, &dialog, [&] {
        const int rowIndex = list->currentRow();
        if (rowIndex < 0) return;
        auto iterator = matrices_.begin();
        std::advance(iterator, rowIndex);
        matrices_.erase(iterator);
        refresh();
    });
    connect(clearButton, &QPushButton::clicked, &dialog, [&] {
        if (QMessageBox::question(&dialog, "Clear matrices", "Delete every saved matrix?") == QMessageBox::Yes) {
            matrices_.clear(); refresh();
        }
    });
    refresh();
    dialog.exec();
}

void MainWindow::exportAction()
{
    if (lastOutput_.isEmpty()) { QMessageBox::information(this, "Nothing to export", "Run an operation first."); return; }
    const QString path = QFileDialog::getSaveFileName(this, "Export result", "Linear-Algebra-Helper-result.txt", "Text files (*.txt);;All files (*.*)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) throw std::runtime_error("Could not open the selected file for writing.");
    const QByteArray title = lastTitle_.toUtf8();
    file.write(title + "\n" + QByteArray(title.size(), '=') + "\n\n" + lastOutput_.toUtf8() + "\n");
}

} // namespace la
