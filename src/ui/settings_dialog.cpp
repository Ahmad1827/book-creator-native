#include "settings_dialog.h"
#include "sketch.h"
#include "app_settings.h"
#include "story_helper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QFileDialog>
#include <QApplication>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Settings");
    setMinimumWidth(700);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(10);

    QLabel *title = new QLabel("Settings", this);
    title->setFont(Sketch::titleFont(32));
    layout->addWidget(title);

    QGroupBox *helperBox = new QGroupBox("Story Helper", this);
    QVBoxLayout *helperLayout = new QVBoxLayout(helperBox);
    helperLayout->setSpacing(8);

    m_radioClaudeCode = new QRadioButton("Use Claude Code on this computer", helperBox);
    m_radioOff = new QRadioButton("Off (hide the Story Helper)", helperBox);
    helperLayout->addWidget(m_radioClaudeCode);
    helperLayout->addWidget(m_radioOff);

    QLabel *privacy = new QLabel(
        "When you press a helper button, the text you're working on and the words already in your book "
        "are sent to Claude through Claude Code, using your Claude Code sign-in. Nothing is sent otherwise.", helperBox);
    privacy->setWordWrap(true);
    Sketch::styleHint(privacy);
    helperLayout->addWidget(privacy);

    QFormLayout *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->setHorizontalSpacing(10);

    QHBoxLayout *pathRow = new QHBoxLayout();
    m_editClaudePath = new QLineEdit(helperBox);
    QPushButton *btnBrowse = new QPushButton("Browse…", helperBox);
    QPushButton *btnTest = new QPushButton("Test", helperBox);
    pathRow->addWidget(m_editClaudePath, 1);
    pathRow->addWidget(btnBrowse);
    pathRow->addWidget(btnTest);
    form->addRow("Claude Code:", pathRow);

    m_comboModel = new QComboBox(helperBox);
    m_comboModel->addItem("Claude Code's default", "");
    m_comboModel->addItem("Haiku (quickest)", "haiku");
    m_comboModel->addItem("Sonnet (balanced)", "sonnet");
    m_comboModel->addItem("Opus (most creative)", "opus");
    m_comboModel->addItem("Fable", "fable");
    form->addRow("Model:", m_comboModel);

    m_comboAge = new QComboBox(helperBox);
    m_comboAge->addItem("Little ones (3–5)", "3-5");
    m_comboAge->addItem("Early readers (5–8)", "5-8");
    m_comboAge->addItem("Confident readers (8–11)", "8-11");
    form->addRow("Readers:", m_comboAge);
    helperLayout->addLayout(form);

    m_lblStatus = new QLabel(helperBox);
    m_lblStatus->setWordWrap(true);
    Sketch::styleHint(m_lblStatus);
    Sketch::setTextColor(m_lblStatus, Sketch::kInkSoft);
    helperLayout->addWidget(m_lblStatus);

    layout->addWidget(helperBox);
    layout->addStretch();

    QHBoxLayout *footer = new QHBoxLayout();
    footer->addStretch();
    QPushButton *btnCancel = new QPushButton("Cancel", this);
    QPushButton *btnSave = new QPushButton("Save", this);
    btnSave->setDefault(true);
    btnSave->setProperty("role", "primary");
    footer->addWidget(btnCancel);
    footer->addWidget(btnSave);
    layout->addLayout(footer);

    // Load current values.
    const bool on = AppSettings::helperBackend() == AppSettings::HelperBackend::ClaudeCode;
    m_radioClaudeCode->setChecked(on);
    m_radioOff->setChecked(!on);
    m_editClaudePath->setText(AppSettings::claudePath());
    const int modelIndex = m_comboModel->findData(AppSettings::claudeModel());
    m_comboModel->setCurrentIndex(modelIndex >= 0 ? modelIndex : 0);
    const int ageIndex = m_comboAge->findData(AppSettings::readerAge());
    m_comboAge->setCurrentIndex(ageIndex >= 0 ? ageIndex : 1);
    refreshDetectedPath();

    auto updateEnabled = [this, btnBrowse, btnTest]() {
        const bool enabled = m_radioClaudeCode->isChecked();
        for (QWidget *w : {static_cast<QWidget*>(m_editClaudePath), static_cast<QWidget*>(btnBrowse), static_cast<QWidget*>(btnTest),
                           static_cast<QWidget*>(m_comboModel), static_cast<QWidget*>(m_comboAge)}) {
            w->setEnabled(enabled);
        }
    };
    updateEnabled();
    connect(m_radioClaudeCode, &QRadioButton::toggled, this, updateEnabled);
    connect(m_editClaudePath, &QLineEdit::textChanged, this, &SettingsDialog::refreshDetectedPath);

    connect(btnBrowse, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this, "Find Claude Code", QDir::homePath());
        if (!path.isEmpty()) m_editClaudePath->setText(path);
    });
    connect(btnTest, &QPushButton::clicked, this, &SettingsDialog::testClaude);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnSave, &QPushButton::clicked, this, [this]() {
        save();
        accept();
    });
}

void SettingsDialog::refreshDetectedPath() {
    if (!m_editClaudePath->text().trimmed().isEmpty()) {
        m_editClaudePath->setPlaceholderText(QString());
        return;
    }
    const QString found = StoryHelper::resolveClaudePath(QString());
    m_editClaudePath->setPlaceholderText(found.isEmpty() ? "Not found. Install Claude Code or browse to it" : "Found: " + found);
}

void SettingsDialog::testClaude() {
    const QString path = StoryHelper::resolveClaudePath(m_editClaudePath->text().trimmed());
    Sketch::styleHint(m_lblStatus);
    Sketch::setTextColor(m_lblStatus, Sketch::kInkSoft);
    m_lblStatus->setText("Checking…");
    QApplication::setOverrideCursor(Qt::WaitCursor);
    QApplication::processEvents();
    QString message;
    const bool ok = StoryHelper::checkInstall(path, &message);
    QApplication::restoreOverrideCursor();

    if (ok) {
        Sketch::setTextColor(m_lblStatus, QColor("#2f7a3a"));
        m_lblStatus->setText(QString("✓ Ready: %1\nIf the helper says you're not signed in, run \"claude\" once in a terminal to sign in.").arg(message));
    } else {
        Sketch::setTextColor(m_lblStatus, Sketch::kTomato.darker(140));
        m_lblStatus->setText(message);
    }
}

void SettingsDialog::save() {
    AppSettings::setHelperBackend(m_radioClaudeCode->isChecked() ? AppSettings::HelperBackend::ClaudeCode : AppSettings::HelperBackend::Off);
    AppSettings::setClaudePath(m_editClaudePath->text());
    AppSettings::setClaudeModel(m_comboModel->currentData().toString());
    AppSettings::setReaderAge(m_comboAge->currentData().toString());
}
