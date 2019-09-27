#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QListWidget>
#include <QSqlDatabase>
#include <QSqlQuery>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void showEvent(QShowEvent *);

private slots:
    void on_btConnectSrc_clicked();
    void on_btConnectDsc_clicked();

    void on_comboDbSrc_currentIndexChanged(const QString &arg1);
    void on_comboDbDsc_currentIndexChanged(const QString &arg1);
    void on_comboSchemaSrc_currentIndexChanged(const QString &arg1);
    void on_comboSchemaDsc_currentIndexChanged(const QString &arg1);
    void on_comboTableSrc_currentIndexChanged(const QString &arg1);
    void on_comboTableDsc_currentIndexChanged(const QString &arg1);

    void on_btPrepare_clicked();
    void on_btExport_clicked();

    void on_actionSaveIntoFile_triggered();
    void on_actionLoadFromFile_triggered();

    void on_btScriptConnect_clicked();
    void on_btScriptExecute_clicked();

    void onFuncHostChange();
    void onFuncDbChange(const int &);
    void onFuncChange(const int &);
    void on_btFuncUpdate_clicked();

    void on_btScriptSelectAll_clicked();
    void on_btScriptUnselectAll_clicked();

private:
    Ui::MainWindow *ui;
    QSqlDatabase dbSrc;
    QSqlDatabase dbDsc;
    QSqlDatabase dbScript;
    QSqlDatabase dbFunc;
    QSqlQuery querySrc;
    QSqlQuery queryDsc;
    QSqlQuery queryScript;
    QSqlQuery queryFunc;

    QString defaultDbName;
    QString defaultSchemaName;
    QString defaultTableName;
    QStringList listScriptDb;

    bool isManual;

    void initCodec();
    bool connect(QSqlDatabase &db);

    void loadSettings();
    void saveSettings();

    void loadDb(QSqlQuery query, QComboBox *combo);
    void loadDb(QSqlQuery query, QListWidget *list);
    void loadShemas(QSqlQuery query, QComboBox *combo);
    void loadTables(QSqlQuery query, QComboBox *combo, QString schema);
    void loadFields(QSqlQuery query, QListWidget *list, QString schema, QString tableName);

    QStringList getFields(QListWidget *list);

    void disconnectFunc();
    void connectFunc();
    void loadListFuncDb(const QString &dbName);
    void loadListFunc(const QString &funcName);
    void loadFunc();
};

#endif // MAINWINDOW_H
