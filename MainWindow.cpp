#include <QSettings>
#include <QMessageBox>
#include <QTextCodec>
#include <QDebug>
#include <QSqlError>
#include <QFileDialog>

#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tabWidget->removeTab(2);
    dbSrc = QSqlDatabase::addDatabase("QPSQL", "ConnectSrc");
    dbDsc = QSqlDatabase::addDatabase("QPSQL", "ConnectDsc");
    dbScript = QSqlDatabase::addDatabase("QPSQL", "ConnectScript");
    dbFunc = QSqlDatabase::addDatabase("QPSQL", "ConnectFunc");
    initCodec();
    loadSettings();
    connectFunc();
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}


void MainWindow::on_btConnectSrc_clicked()
{
    bool isManualLocal = false;
    if (!isManual) {
        defaultDbName = ui->comboDbSrc->currentText();
        defaultSchemaName = ui->comboSchemaSrc->currentText();
        defaultTableName = ui->comboTableSrc->currentText();
        isManual = true;
        isManualLocal = true;
    }
    dbSrc.setHostName(ui->editHostSrc->text());
    dbSrc.setDatabaseName("postgres");
    dbSrc.setUserName(ui->editUserSrc->text());
    dbSrc.setPassword(ui->editPasswordSrc->text());
    if (connect(dbSrc)) {
        querySrc = QSqlQuery(dbSrc);
        loadDb(querySrc, ui->comboDbSrc);
    }
    if (isManualLocal) {
        isManual = false;
    }
}

void MainWindow::on_btConnectDsc_clicked()
{
    bool isManualLocal = false;
    if (!isManual) {
        defaultDbName = ui->comboDbDsc->currentText();
        defaultSchemaName = ui->comboSchemaDsc->currentText();
        defaultTableName = ui->comboTableDsc->currentText();
        isManual = true;
        isManualLocal = true;
    }
    dbDsc.setHostName(ui->editHostDsc->text());
    dbDsc.setDatabaseName("postgres");
    dbDsc.setUserName(ui->editUserDsc->text());
    dbDsc.setPassword(ui->editPasswordDsc->text());
    if (connect(dbDsc)) {
        queryDsc = QSqlQuery(dbDsc);
        loadDb(queryDsc, ui->comboDbDsc);
    }
    if (isManualLocal) {
        isManual = false;
    }
}

void MainWindow::on_comboDbSrc_currentIndexChanged(const QString &arg1)
{
    bool isManualLocal = false;
    if (!isManual) {
        defaultDbName = QString();
        defaultSchemaName = ui->comboSchemaSrc->currentText();
        defaultTableName = ui->comboTableSrc->currentText();
        isManual = true;
        isManualLocal = true;
    }
    dbSrc.close();
    dbSrc.setDatabaseName(arg1);
    if (connect(dbSrc)) {
        querySrc = QSqlQuery(dbSrc);
        loadShemas(querySrc, ui->comboSchemaSrc);
    }
    if (isManualLocal) {
        isManual = false;
    }
}

void MainWindow::on_comboDbDsc_currentIndexChanged(const QString &arg1)
{
    bool isManualLocal = false;
    if (!isManual) {
        defaultDbName = QString();
        defaultSchemaName = ui->comboSchemaDsc->currentText();
        defaultTableName = ui->comboTableDsc->currentText();
        isManual = true;
        isManualLocal = true;
    }
    dbDsc.close();
    dbDsc.setDatabaseName(arg1);
    if (connect(dbDsc)) {
        queryDsc = QSqlQuery(dbDsc);
        loadShemas(queryDsc, ui->comboSchemaDsc);
    }
    if (isManualLocal) {
        isManual = false;
    }
}

void MainWindow::on_comboSchemaSrc_currentIndexChanged(const QString &arg1)
{
    bool isManualLocal = false;
    if (!isManual) {
        defaultDbName = QString();
        defaultSchemaName = QString();
        defaultTableName = ui->comboTableSrc->currentText();
        isManual = true;
        isManualLocal = true;
    }
    loadTables(querySrc, ui->comboTableSrc, arg1);
    if (isManualLocal) {
        isManual = false;
    }
}

void MainWindow::on_comboSchemaDsc_currentIndexChanged(const QString &arg1)
{
    bool isManualLocal = false;
    if (!isManual) {
        defaultDbName = QString();
        defaultSchemaName = QString();
        defaultTableName = ui->comboTableDsc->currentText();
        isManual = true;
        isManualLocal = true;
    }
    loadTables(queryDsc, ui->comboTableDsc, arg1);
    if (isManualLocal) {
        isManual = false;
    }
}

void MainWindow::on_comboTableSrc_currentIndexChanged(const QString &arg1)
{
    loadFields(querySrc, ui->listFieldsSrc, ui->comboSchemaSrc->currentText(), arg1);
}

void MainWindow::on_comboTableDsc_currentIndexChanged(const QString &arg1)
{
    loadFields(queryDsc, ui->listFieldsDsc, ui->comboSchemaDsc->currentText(), arg1);
}

void MainWindow::on_btPrepare_clicked()
{
    QTableWidget *table = ui->tableData;
    table->clear();
    table->setRowCount(0);
    table->setColumnCount(0);
    QStringList fields = getFields(ui->listFieldsSrc);
    if (!fields.isEmpty() && querySrc.exec(QString("SELECT \"%1\" FROM \"%2\".\"%3\" %4;")
                                           .arg(fields.join("\",\""))
                                           .arg(ui->comboSchemaSrc->currentText())
                                           .arg(ui->comboTableSrc->currentText())
                                           .arg(ui->editWhere->text()))) {
        table->setColumnCount(fields.count());
        table->setHorizontalHeaderLabels(fields);
        int row = 0;
        while (querySrc.next()) {
            table->setRowCount(++row);
            for (int col = 0; col < fields.count(); ++col) {
                QVariant value = querySrc.value(col);
                if (value.isNull()) {
                    table->setItem(row-1, col, new QTableWidgetItem("{NULL}"));
                } else {
                    table->setItem(row-1, col, new QTableWidgetItem(value.toString()));
                }
            }
            table->item(row-1, 0)->setCheckState(Qt::Checked);
        }
    }
    table->resizeColumnsToContents();
}

void MainWindow::on_btExport_clicked()
{
    QStringList fieldsSrc = getFields(ui->listFieldsSrc);
    QStringList fieldsDsc = getFields(ui->listFieldsDsc);
    if (!fieldsSrc.isEmpty() && fieldsSrc.count() == fieldsDsc.count()) {
        QTableWidget *table = ui->tableData;
        if (ui->checkBox->isChecked()) {
            queryDsc.exec(QString("DELETE FROM \"%1\".\"%2\";")
                          .arg(ui->comboSchemaDsc->currentText())
                          .arg(ui->comboTableDsc->currentText()));
        } else {
            QString keySrc = QString();
            QString keyDsc = QString();
            if (~fieldsSrc.indexOf("id")) {
                keySrc = "id";
            }
            if (~fieldsDsc.indexOf("id")) {
                keyDsc = "id";
            }
            if (keySrc.isEmpty() && querySrc.exec(QString("SELECT column_name FROM information_schema.key_column_usage "
                                                          "WHERE table_schema = '%1' AND table_name = '%2';")
                                                  .arg(ui->comboSchemaSrc->currentText())
                                                  .arg(ui->comboTableSrc->currentText()))) {
                if (querySrc.first()) {
                    keySrc = querySrc.value(0).toString();
                }
            }
            if (!keySrc.isEmpty()) {
                QList<QListWidgetItem*> items = ui->listFieldsSrc->findItems(keySrc, Qt::MatchFixedString);
                if (!items.isEmpty()) {
                    if (items.first()->checkState() == Qt::Unchecked) {
                        keySrc = QString();
                    }
                }
            }
            if (keyDsc.isEmpty() && !keySrc.isEmpty() &&
                    queryDsc.exec(QString("SELECT column_name FROM information_schema.key_column_usage "
                                          "WHERE table_schema = '%1' AND table_name = '%2';")
                                  .arg(ui->comboSchemaDsc->currentText())
                                  .arg(ui->comboTableDsc->currentText()))) {
                if (queryDsc.first()) {
                    keyDsc = queryDsc.value(0).toString();
                }
            }
            if (!keyDsc.isEmpty()) {
                QList<QListWidgetItem*> items = ui->listFieldsDsc->findItems(keyDsc, Qt::MatchFixedString);
                if (!items.isEmpty()) {
                    if (items.first()->checkState() == Qt::Unchecked) {
                        keyDsc = QString();
                    }
                }
            }
            if (!keySrc.isEmpty() && !keyDsc.isEmpty()) {
                int colKey = -1;
                for (int col = 0; col < table->columnCount(); ++col) {
                    if (table->horizontalHeaderItem(col)->text() == keySrc) {
                        colKey = col;
                        break;
                    }
                }
                if (~colKey) {
                    QStringList keys;
                    for (int row = 0; row < table->rowCount(); ++row) {
                        if (table->item(row, 0)->checkState() == Qt::Checked) {
                            keys.append(table->item(row, colKey)->text());
                        }
                    }
                    if (!queryDsc.exec(QString("DELETE FROM \"%1\".\"%2\" WHERE \"%3\" IN ('%4');")
                                       .arg(ui->comboSchemaDsc->currentText())
                                       .arg(ui->comboTableDsc->currentText())
                                       .arg(keyDsc)
                                       .arg(keys.join("','")))) {
                        qDebug() << queryDsc.lastError().text();
                    }
                }
            }
        }
        bool notError = true;
        for (int row = 0; row < table->rowCount(); ++row) {
            if (table->item(row, 0)->checkState() == Qt::Checked) {
                QStringList values;
                for (int col = 0; col < table->columnCount(); ++col) {
                    values << table->item(row, col)->text().replace("'", "''");
                }
                if (!queryDsc.exec(QString("INSERT INTO \"%1\".\"%2\" (\"%3\") VALUES ('%4');")
                                   .arg(ui->comboSchemaDsc->currentText())
                                   .arg(ui->comboTableDsc->currentText())
                                   .arg(fieldsDsc.join("\",\""))
                                   .arg(values.join("','"))
                                   .replace("'TRUE'", "TRUE")
                                   .replace("'FALSE'", "FALSE")
                                   .replace("'true'", "TRUE")
                                   .replace("'false'", "FALSE")
                                   .replace("'{NULL}'", "NULL"))) {
                    qDebug() << queryDsc.lastError().text();
                    notError = false;
                }
            }
        }
        if (notError) {
            QMessageBox::about(this, "Message", "Success!");
        } else {
            QMessageBox::critical(this, "Error", "Error!");
        }
    } else {
        QMessageBox::critical(this, "Error", "Error!");
    }
}


void MainWindow::loadSettings()
{
    QSettings ini("setting.ini", QSettings::IniFormat);
    ini.beginGroup("WINDOW");
    this->setGeometry(ini.value("X", 200).toInt(),
                      ini.value("Y", 200).toInt(),
                      ini.value("WIDTH", 200).toInt(),
                      ini.value("HEIGHT", 200).toInt());
    QString sizes_t = ini.value("SPLITTER").toString();
    if (!sizes_t.isEmpty()) {
        QList<int> sizes;
        QStringList sizes_l = sizes_t.split(";");
        for (int i = 0; i < sizes_l.count(); ++i) {
            sizes << sizes_l[i].toInt();
        }
        ui->splitter->setSizes(sizes);
    }
    sizes_t = ini.value("SPLITTER2").toString();
    if (!sizes_t.isEmpty()) {
        QList<int> sizes;
        QStringList sizes_l = sizes_t.split(";");
        for (int i = 0; i < sizes_l.count(); ++i) {
            sizes << sizes_l[i].toInt();
        }
        ui->splitter_2->setSizes(sizes);
    }
    ui->tabWidget->setCurrentIndex(ini.value("TAB", 0).toInt());
    ini.endGroup();
    ini.beginGroup("SRC");
    ui->editHostSrc->setText(ini.value("HOST").toString());
    ui->editUserSrc->setText(ini.value("USER").toString());
    ui->editPasswordSrc->setText(ini.value("PASSWORD").toString());
    QString dbNameSrc = ini.value("DATABASE").toString();
    QString schemaSrc = ini.value("SCHEMA").toString();
    QString tableSrc = ini.value("TABLE").toString();
    ini.endGroup();
    ini.beginGroup("DSC");
    ui->editHostDsc->setText(ini.value("HOST").toString());
    ui->editUserDsc->setText(ini.value("USER").toString());
    ui->editPasswordDsc->setText(ini.value("PASSWORD").toString());
    QString dbNameDsc = ini.value("DATABASE").toString();
    QString schemaDsc = ini.value("SCHEMA").toString();
    QString tableDsc = ini.value("TABLE").toString();
    ini.endGroup();
    ini.beginGroup("SCRIPT");
    ui->editScriptHost->setText(ini.value("HOST").toString());
    ui->editScriptUser->setText(ini.value("USER").toString());
    ui->editScriptPassword->setText(ini.value("PASSWORD").toString());
    listScriptDb = ini.value("LISTDB").toString().split(";");

    ini.endGroup();
    if (!ui->editHostSrc->text().isEmpty() && !ui->editUserSrc->text().isEmpty() &&
            !ui->editPasswordSrc->text().isEmpty()) {
        on_btConnectSrc_clicked();
    }
    if (!ui->editHostDsc->text().isEmpty() && !ui->editUserDsc->text().isEmpty() &&
            !ui->editPasswordDsc->text().isEmpty()) {
        on_btConnectDsc_clicked();
    }
    if (!dbNameSrc.isEmpty()) {
        ui->comboDbSrc->setCurrentIndex(ui->comboDbSrc->findText(dbNameSrc));
    }
    if (!schemaSrc.isEmpty()) {
        ui->comboSchemaSrc->setCurrentIndex(ui->comboSchemaSrc->findText(schemaSrc));
    }
    if (!tableSrc.isEmpty()) {
        ui->comboTableSrc->setCurrentIndex(ui->comboTableSrc->findText(tableSrc));
    }
    if (!dbNameDsc.isEmpty()) {
        ui->comboDbDsc->setCurrentIndex(ui->comboDbDsc->findText(dbNameDsc));
    }
    if (!schemaDsc.isEmpty()) {
        ui->comboSchemaDsc->setCurrentIndex(ui->comboSchemaDsc->findText(schemaDsc));
    }
    if (!tableDsc.isEmpty()) {
        ui->comboTableDsc->setCurrentIndex(ui->comboTableDsc->findText(tableDsc));
    }

    ini.beginGroup("FUNCTION");
    ui->editFuncHost->setText(ini.value("HOST", "127.0.0.1").toString());
    ui->editFuncUser->setText(ini.value("USER", "postgres").toString());
    ui->editFuncPassword->setText(ini.value("PASSWORD", "").toString());
    ui->comboFuncDb->setCurrentIndex(ini.value("DB", 0).toInt());
    ui->comboFunc->setCurrentIndex(ini.value("FUNCTION", 0).toInt());
    ini.endGroup();
}

void MainWindow::saveSettings()
{
    QSettings ini("setting.ini", QSettings::IniFormat);
    ini.beginGroup("WINDOW");
    ini.setValue("X", this->geometry().x());
    ini.setValue("Y", this->geometry().y());
    ini.setValue("WIDTH", this->geometry().width());
    ini.setValue("HEIGHT", this->geometry().height());
    QStringList sizes;
    for (int index = 0; index < ui->splitter->sizes().count(); ++index) {
        sizes.append(QString::number(ui->splitter->sizes()[index]));
    }
    ini.setValue("SPLITTER", sizes.join(";"));
    sizes.clear();
    for (int index = 0; index < ui->splitter_2->sizes().count(); ++index) {
        sizes.append(QString::number(ui->splitter_2->sizes()[index]));
    }
    ini.setValue("SPLITTER2", sizes.join(";"));
    ini.setValue("TAB", ui->tabWidget->currentIndex());
    ini.endGroup();
    ini.beginGroup("SRC");
    ini.setValue("HOST", ui->editHostSrc->text());
    ini.setValue("USER", ui->editUserSrc->text());
    ini.setValue("PASSWORD", ui->editPasswordSrc->text());
    ini.setValue("DATABASE", ui->comboDbSrc->currentText());
    ini.setValue("SCHEMA", ui->comboSchemaSrc->currentText());
    ini.setValue("TABLE", ui->comboTableSrc->currentText());
    ini.endGroup();
    ini.beginGroup("DSC");
    ini.setValue("HOST", ui->editHostDsc->text());
    ini.setValue("USER", ui->editUserDsc->text());
    ini.setValue("PASSWORD", ui->editPasswordDsc->text());
    ini.setValue("DATABASE", ui->comboDbDsc->currentText());
    ini.setValue("SCHEMA", ui->comboSchemaDsc->currentText());
    ini.setValue("TABLE", ui->comboTableDsc->currentText());
    ini.endGroup();

    ini.beginGroup("SCRIPT");
    ini.setValue("HOST", ui->editScriptHost->text());
    ini.setValue("USER", ui->editScriptUser->text());
    ini.setValue("PASSWORD", ui->editScriptPassword->text());
    ini.setValue("LISTDB", listScriptDb.join(";"));
    ini.endGroup();

    ini.beginGroup("FUNCTION");
    ini.setValue("HOST", ui->editFuncHost->text());
    ini.setValue("USER", ui->editFuncUser->text());
    ini.setValue("PASSWORD", ui->editFuncPassword->text());
    ini.setValue("DB", ui->comboFuncDb->currentIndex());
    ini.setValue("FUNCTION", ui->comboFunc->currentIndex());
    ini.endGroup();
}

void MainWindow::loadDb(QSqlQuery query, QComboBox *combo)
{
    combo->clear();
    if (query.exec("SELECT datname FROM pg_database WHERE datname NOT IN "
                   "('postgres','template0','template1') ORDER BY datname ASC;")) {
        while (query.next()) {
            combo->addItem(query.value(0).toString());
        }
    }
    if (!defaultDbName.isEmpty()) {
        int index = combo->findText(defaultDbName);
        if (~index) {
            combo->setCurrentIndex(index);
        }
    }
}

void MainWindow::loadDb(QSqlQuery query, QListWidget *list)
{
    list->clear();
    if (query.exec("SELECT datname FROM pg_database WHERE datname NOT IN "
                   "('postgres','template0','template1') ORDER BY datname ASC;")) {
        while (query.next()) {
            QString dbName = query.value(0).toString();
            QListWidgetItem *item = new QListWidgetItem(dbName);
            item->setCheckState(~listScriptDb.indexOf(dbName) ? Qt::Checked : Qt::Unchecked);
            list->addItem(item);
        }
    }
}

void MainWindow::loadShemas(QSqlQuery query, QComboBox *combo)
{
    combo->clear();
    if (query.exec("SELECT DISTINCT schemaname FROM pg_tables WHERE schemaname NOT IN "
                   "('information_schema','pg_catalog') ORDER BY schemaname ASC;")) {
        while (query.next()) {
            combo->addItem(query.value(0).toString());
        }
    }
    if (!defaultSchemaName.isEmpty()) {
        int index = combo->findText(defaultSchemaName);
        if (~index) {
            combo->setCurrentIndex(index);
        }
    }
}

void MainWindow::loadTables(QSqlQuery query, QComboBox *combo, QString schema)
{
    combo->clear();
    if (query.exec(QString("SELECT tablename FROM pg_tables WHERE schemaname = '%1' "
                           "ORDER BY tablename ASC;")
                   .arg(schema))) {
        while (query.next()) {
            combo->addItem(query.value(0).toString());
        }
    }
    if (!defaultTableName.isEmpty()) {
        int index = combo->findText(defaultTableName);
        if (~index) {
            combo->setCurrentIndex(index);
        }
    }
}

void MainWindow::loadFields(QSqlQuery query, QListWidget *list, QString schema, QString tableName)
{
    list->clear();
    if (query.exec(QString("SELECT column_name FROM information_schema.columns "
                           "WHERE table_schema = '%1' AND table_name = '%2' "
                           "ORDER BY ordinal_position ASC;")
                   .arg(schema)
                   .arg(tableName))) {
        while (query.next()) {
            QListWidgetItem *item = new QListWidgetItem(query.value(0).toString());
            item->setCheckState(Qt::Checked);
            list->addItem(item);
        }
    }
}

QStringList MainWindow::getFields(QListWidget *list)
{
    QStringList fields;
    for (int index = 0; index < list->count(); ++index) {
        QListWidgetItem *item = list->item(index);
        if (item->checkState() == Qt::Checked) {
            fields << item->text();
        }
    }
    return fields;
}

void MainWindow::disconnectFunc()
{
    QObject::disconnect(ui->editFuncHost, SIGNAL(editingFinished()), this, SLOT(onFuncHostChange()));
    QObject::disconnect(ui->editFuncUser, SIGNAL(editingFinished()), this, SLOT(onFuncHostChange()));
    QObject::disconnect(ui->editFuncPassword, SIGNAL(editingFinished()), this, SLOT(onFuncHostChange()));
    QObject::disconnect(ui->comboFuncDb, SIGNAL(currentIndexChanged(int)), this, SLOT(onFuncDbChange(int)));
    QObject::disconnect(ui->comboFunc, SIGNAL(currentIndexChanged(int)), this, SLOT(onFuncChange(int)));
}

void MainWindow::connectFunc()
{
    QObject::connect(ui->editFuncHost, SIGNAL(editingFinished()), this, SLOT(onFuncHostChange()));
    QObject::connect(ui->editFuncUser, SIGNAL(editingFinished()), this, SLOT(onFuncHostChange()));
    QObject::connect(ui->editFuncPassword, SIGNAL(editingFinished()), this, SLOT(onFuncHostChange()));
    QObject::connect(ui->comboFuncDb, SIGNAL(currentIndexChanged(int)), this, SLOT(onFuncDbChange(int)));
    QObject::connect(ui->comboFunc, SIGNAL(currentIndexChanged(int)), this, SLOT(onFuncChange(int)));
}

void MainWindow::loadListFuncDb(const QString &dbName)
{
    if (ui->editFuncHost->text().isEmpty() ||
            ui->editFuncUser->text().isEmpty() ||
            ui->editFuncPassword->text().isEmpty()) {
        return;
    }
    dbFunc.setHostName(ui->editFuncHost->text());
    dbFunc.setDatabaseName("postgres");
    dbFunc.setUserName(ui->editFuncUser->text());
    dbFunc.setPassword(ui->editFuncPassword->text());
    if (connect(dbFunc)) {
        queryFunc = QSqlQuery(dbFunc);
        loadDb(queryFunc, ui->comboFuncDb);
        int index = ui->comboFuncDb->findText(dbName);
        if (index > 0) {
            ui->comboFuncDb->setCurrentIndex(index);
        }
    }
}

void MainWindow::loadListFunc(const QString &funcName)
{
    QComboBox *combo = ui->comboFunc;
    combo->clear();

    if (queryFunc.exec("SELECT 1;")) {
        while (queryFunc.next()) {
            combo->addItem(queryFunc.value(0).toString());
        }
    }

    int index = combo->findText(funcName);
    if (index > 0) {
        combo->setCurrentIndex(index);
    }
}

void MainWindow::loadFunc()
{
}

void MainWindow::initCodec()
{
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(codec);
#if QT_VERSION_MAJOR < 5
    QTextCodec::setCodecForCStrings(codec);
    QTextCodec::setCodecForTr(codec);
#endif
}

bool MainWindow::connect(QSqlDatabase &db)
{
    if (!db.open()) {
        qDebug() << db.lastError().text();
        QMessageBox::critical(this, "Error", "Not connection");
        return false;
    }
    return true;
}


void MainWindow::on_actionSaveIntoFile_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, "File", qApp->applicationDirPath(), "*.table");
    if (!fileName.isEmpty()) {
        QTableWidget *table = ui->tableData;
        QFile file(fileName);
        file.open(QFile::WriteOnly);
        for (int col = 0; col < table->columnCount(); ++col) {
        }
        for (int row = 0; row < table->rowCount(); ++row) {
        }
        file.close();
    }
}

void MainWindow::on_actionLoadFromFile_triggered()
{
}


void MainWindow::on_btScriptConnect_clicked()
{
    dbScript.setHostName(ui->editScriptHost->text());
    dbScript.setDatabaseName("postgres");
    dbScript.setUserName(ui->editScriptUser->text());
    dbScript.setPassword(ui->editScriptPassword->text());
    if (connect(dbScript)) {
        queryScript = QSqlQuery(dbScript);
        loadDb(queryScript, ui->listScriptDb);
    }
}

void MainWindow::on_btScriptExecute_clicked()
{
    if (ui->editScript->toPlainText().isEmpty()) {
        QMessageBox::critical(this, "Error", "Empty script");
        return;
    }
    listScriptDb.clear();
    for (int i = 0; i < ui->listScriptDb->count(); ++i) {
        QListWidgetItem *item = ui->listScriptDb->item(i);
        if (item->checkState() == Qt::Checked) {
            QString dbName = item->text();
            listScriptDb.append(dbName);
            dbScript.close();
            dbScript.setDatabaseName(dbName);
            dbScript.open();
            queryScript = QSqlQuery(dbScript);
            if (!queryScript.exec(ui->editScript->toPlainText())) {
                if (QMessageBox::question(this, "Error", QString("Error script in db %1.\n"
                                                                 "To continue on others db?").arg(dbName),
                                          QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
                    return;
                }
            }
        }
    }
    QMessageBox::about(this, "Message", "Success!");
}


void MainWindow::onFuncHostChange()
{
    disconnectFunc();
    loadListFuncDb(ui->comboFuncDb->currentText());
    loadListFunc(ui->comboFunc->currentText());
    loadFunc();
    connectFunc();
}

void MainWindow::onFuncDbChange(const int &)
{
    disconnectFunc();
    loadListFuncDb(ui->comboFuncDb->currentText());
    loadFunc();
    connectFunc();
}

void MainWindow::onFuncChange(const int &)
{
    disconnectFunc();
    loadFunc();
    connectFunc();
}

void MainWindow::on_btFuncUpdate_clicked()
{
}

void MainWindow::on_btScriptSelectAll_clicked()
{
    for (int i = 0; i < ui->listScriptDb->count(); ++i) {
        ui->listScriptDb->item(i)->setCheckState(Qt::Checked);
    }
}

void MainWindow::on_btScriptUnselectAll_clicked()
{
    for (int i = 0; i < ui->listScriptDb->count(); ++i) {
        ui->listScriptDb->item(i)->setCheckState(Qt::Unchecked);
    }
}
