#include "dao_clvdataaccessor.h"

/////////////////
///  BASIC FUNCTIONS
/////////////////

ClvDataAccessor::ClvDataAccessor(QString filePath)
{
    db_m =  QSqlDatabase::addDatabase("QSQLITE");
    db_m.setDatabaseName(filePath);
}

void ClvDataAccessor::open(bool initialize){
    if(db_m.isOpen()){
        qDebug() << "Database is already opened.";
    }else{
        db_m.open();
        checkTableSyntax(initialize);
    }
}

bool ClvDataAccessor::isOpen(){
    return db_m.isOpen();
}

void ClvDataAccessor::close(){
    if(db_m.isOpen()){
        db_m.close();
    }else{
        qDebug() << "Database is not opened.";
    }
}

QSqlQuery ClvDataAccessor::execSql(QSqlQuery query){
    bool success = query.exec();
    if(AppInfo::DEBUG_CLV_DATA_ACCESSOR){
        qDebug() << "EXEC SQL: " << query.lastQuery();
    }
    if(!success){
        qDebug();
        qDebug() << QString("SQL Error:'%1'").arg(query.executedQuery());
        qDebug() << query.lastError().text();
        qDebug();
    }
    return query;
}

void ClvDataAccessor::checkTableSyntax(bool initialize){
   checkTableSyntax2(PROJECT_TABLE, PROJECT_CHECK_TABLE, PROJECT_DROP_TABLE, PROJECT_CREATE_TABLE, initialize);
   checkTableSyntax2(COMMAND_TABLE, COMMAND_CHECK_TABLE, COMMAND_DROP_TABLE, COMMAND_CREATE_TABLE, initialize);
   checkTableSyntax2(GROUP_TABLE, GROUP_CHECK_TABLE, GROUP_DROP_TABLE, GROUP_CREATE_TABLE, initialize);
   checkTableSyntax2(HIGHLIGHT_TABLE, HIGHLIGHT_CHECK_TABLE, HIGHLIGHT_DROP_TABLE, HIGHLIGHT_CREATE_TABLE, initialize);
   checkTableSyntax2(LAYER_TABLE, LAYER_CHECK_TABLE, LAYER_DROP_TABLE, LAYER_CREATE_TABLE, initialize);
   checkTableSyntax2(SEARCH1_TABLE, SEARCH1_CHECK_TABLE, SEARCH1_DROP_TABLE, SEARCH1_CREATE_TABLE, initialize);
   checkTableSyntax2(SEARCH2_TABLE, SEARCH2_CHECK_TABLE, SEARCH2_DROP_TABLE, SEARCH2_CREATE_TABLE, initialize);
   checkTableSyntax2(BOOKMARK_TABLE, BOOKMARK_CHECK_TABLE, BOOKMARK_DROP_TABLE, BOOKMARK_CREATE_TABLE, initialize);
   checkTableSyntax2(MEMO_TABLE, MEMO_CHECK_TABLE, MEMO_DROP_TABLE, MEMO_CREATE_TABLE, initialize);
}

void ClvDataAccessor::checkTableSyntax2(QString tableString, QString checkQuery, QString dropQuery, QString createQuery, bool initialize){
    QSqlQuery query;
    query.prepare(checkQuery);
    execSql(query);
    if(query.next()){
        qDebug() << QString("Table '%1' exists.").arg(tableString);
        QString pastCreateQuery = query.value(0).toString();
        if(initialize){
            qDebug() << "    initialize 'true'. drop and recreate";
            query.prepare(dropQuery);
            execSql(query);
            query.prepare(createQuery);
            execSql(query);
        }else if(createQuery != pastCreateQuery){
            qDebug() << "    syntax wrong. drop and recreate";
            query.prepare(dropQuery);
            execSql(query);
            query.prepare(createQuery);
            execSql(query);
        }else{
            qDebug() << "    syntax ok.";
        }
    }else{
        qDebug() << QString("Table '%1' doesn't exists.").arg(tableString);
        qDebug() << "    create table.";
        query.prepare(createQuery);
        execSql(query);
    }
}

QString ClvDataAccessor::getCurrentDateString(){
    QDateTime time = QDateTime::currentDateTime();
    return time.toString("yyyy-MM-dd hh:mm:ss");
}

/////////////////
///  PROJECT INFORMATION HANDLING
/////////////////

void ClvDataAccessor::writeProjectInfo(QString product){
    QSqlQuery query;
    query.prepare(PROJECT_INSERT_TO_TABLE);
    query.bindValue(BIND_MAJOR_VERSION, AppInfo::MAJOR_VERSION);
    query.bindValue(BIND_MINOR_VERSION, AppInfo::MINOR_VERSION);
    query.bindValue(BIND_PRODUCT, product);
    query.bindValue(BIND_CREATE_DATE, getCurrentDateString());
    query.bindValue(BIND_UPDATE_DATE, getCurrentDateString());
    execSql(query);
}


/////////////////
///  COMMAND TABLE
/////////////////

void ClvDataAccessor::writeCommand(QString name, QString decoratedName, QString scope, QString tag, QString file, QByteArray data){
    QSqlQuery query;
    query.prepare(COMMAND_INSERT_TO_TABLE);
    query.bindValue(BIND_NAME, name);
    query.bindValue(BIND_DECORATED_NAME, decoratedName);
    query.bindValue(BIND_SCOPE, scope);
    query.bindValue(BIND_TAG, tag);
    query.bindValue(BIND_FILE, file);
    query.bindValue(BIND_DATA, data);
    execSql(query);
}

void ClvDataAccessor::updateCommand(QString layer, QString group, QList<int> idList){
    foreach(int id, idList){
        QString sql = QString(COMMAND_UPDATE_GROUP_RAYER).arg(layer, group, QString::number(id));
        QSqlQuery query;
        query.prepare(sql);
        execSql(query);
    }
}

/////////////////
///  COMMAND GROUP
/////////////////

QList<int> ClvDataAccessor::getIdListFromGroup(QString group){
    QList<int> idList;
    QString sql = QString(GROUP_SELECT_COMMAND_ID_BY_GROUP_NAME).arg(group);
    QSqlQuery query;
    query.prepare(sql);
    execSql(query);
    while(query.next()){
        idList.append(query.value(0).toInt());
    }
    return idList;
}

void ClvDataAccessor::writeGroupDefinition(QString groupName, QList<QString> regexList, QList<QString> highlightList){
    QSet<int> hitIdSet;
    QList<int> hitIdList;

    // make id list
    foreach(QString regex, regexList){
        QSqlQuery query;
        QString sql = QString(COMMAND_SELECT_ID_BY_REGEX).arg(regex);
        query.prepare(sql);
        query = execSql(query);
        while(query.next()){
            int id = query.value(0).toInt();
            if(!hitIdSet.contains(id)){
                hitIdSet.insert(id);
                hitIdList.append(id);
            }
        }
    }

    // register
    foreach(int id, hitIdList){
        QSqlQuery query;
        query.prepare(GROUP_INSERT_TO_TABLE);
        query.bindValue(BIND_NAME, groupName);
        query.bindValue(BIND_COMMAND_ID, id);
        execSql(query);
    }

    // highlight
    foreach(QString regex, highlightList){
        QSqlQuery query;
        query.prepare(HIGHLIGHT_INSERT_TO_TABLE);
        query.bindValue(BIND_GROUP_NAME, groupName);
        query.bindValue(BIND_REGEX, regex);
        query.bindValue(BIND_COLOR, "red");
        execSql(query);
    }
}

/////////////////
///  LAYER
/////////////////

void ClvDataAccessor::writeLayerDefinition(QString layerName, QList<QString> groupList){
    // register
    foreach(QString group, groupList){
        QSqlQuery query;
        query.prepare(LAYER_INSERT_TO_TABLE);
        query.bindValue(BIND_NAME, layerName);
        query.bindValue(BIND_GROUP_NAME, group);
        execSql(query);
    }
}

/////////////////
///  VAL
/////////////////

// table project
const QString ClvDataAccessor::PROJECT_TABLE = "project";
const QString ClvDataAccessor::PROJECT_CHECK_TABLE = "SELECT sql FROM sqlite_master WHERE type='table' AND name='project'";
const QString ClvDataAccessor::PROJECT_DROP_TABLE = "DROP TABLE project";
const QString ClvDataAccessor::PROJECT_CREATE_TABLE = "CREATE TABLE project (major_version INT, minor_version INT, product TEXT, create_date TEXT, update_date TEXT)";
const QString ClvDataAccessor::PROJECT_INSERT_TO_TABLE = "INSERT INTO project values (:major_version, :minor_version, :product, :create_date, :update_date )";

// table command
const QString ClvDataAccessor::COMMAND_TABLE = "command";
const QString ClvDataAccessor::COMMAND_CHECK_TABLE = "SELECT sql FROM sqlite_master WHERE type='table' AND name='command'";
const QString ClvDataAccessor::COMMAND_CREATE_TABLE = "CREATE TABLE command (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, decorated_name TEXT, layer_name TEXT, group_name TEXT, scope TEXT, tag TEXT, file TEXT, data BLOB)";
const QString ClvDataAccessor::COMMAND_DROP_TABLE = "DROP TABLE command";
const QString ClvDataAccessor::COMMAND_INSERT_TO_TABLE = "INSERT INTO command (name, decorated_name, scope, tag, file, data) values (:name, :decorated_name, :scope, :tag, :file, :data )";
const QString ClvDataAccessor::COMMAND_UPDATE_GROUP_RAYER = "UPDATE command SET layer_name = \"%1\", group_name = \"%2\" WHERE id = %3";
const QString ClvDataAccessor::COMMAND_SELECT_ID_BY_REGEX = "SELECT id FROM command WHERE name LIKE \"%1\"";
const QString ClvDataAccessor::COMMAND_SELECT_COMMAND_LAYER_GROUP = "SELECT name, layer_name, group_name FROM command";

// table command_group
const QString ClvDataAccessor::GROUP_TABLE = "command_group";
const QString ClvDataAccessor::GROUP_DROP_TABLE = "DROP TABLE command_group";
const QString ClvDataAccessor::GROUP_CHECK_TABLE = "SELECT sql FROM sqlite_master WHERE type='table' AND name='command_group'";
const QString ClvDataAccessor::GROUP_CREATE_TABLE = "CREATE TABLE command_group (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, command_id INT)";
const QString ClvDataAccessor::GROUP_INSERT_TO_TABLE = "INSERT INTO command_group (name, command_id) values (:name, :command_id )";
const QString ClvDataAccessor::GROUP_SELECT_COMMAND_ID_BY_GROUP_NAME = "SELECT command_id FROM command_group WHERE name = \"%1\"";

// table highlight
const QString ClvDataAccessor::HIGHLIGHT_TABLE = "highlight";
const QString ClvDataAccessor::HIGHLIGHT_CHECK_TABLE = "SELECT sql FROM sqlite_master WHERE type='table' AND name='highlight'";
const QString ClvDataAccessor::HIGHLIGHT_DROP_TABLE = "DROP TABLE highlight";
const QString ClvDataAccessor::HIGHLIGHT_CREATE_TABLE = "CREATE TABLE highlight (id INTEGER PRIMARY KEY AUTOINCREMENT, group_name TEXT, regex TEXT, color TEXT)";
const QString ClvDataAccessor::HIGHLIGHT_INSERT_TO_TABLE  = "INSERT INTO highlight (group_name, regex, color) values (:group_name, :regex, :color)";

// table command layer
const QString ClvDataAccessor::LAYER_TABLE = "layer";
const QString ClvDataAccessor::LAYER_DROP_TABLE = "DROP TABLE layer";
const QString ClvDataAccessor::LAYER_CHECK_TABLE = "SELECT sql FROM sqlite_master WHERE type='table' AND name='layer'";
const QString ClvDataAccessor::LAYER_CREATE_TABLE = "CREATE TABLE layer (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, group_name TEXT)";
const QString ClvDataAccessor::LAYER_INSERT_TO_TABLE = "INSERT INTO layer (name, group_name) values (:name, :group_name )";

// table search
const QString ClvDataAccessor::SEARCH1_TABLE = "search1";
const QString ClvDataAccessor::SEARCH1_CHECK_TABLE = "SELECT sql FROM sqlite_master WHERE type='table' AND name='search1'";
const QString ClvDataAccessor::SEARCH1_DROP_TABLE = "DROP TABLE search1";
const QString ClvDataAccessor::SEARCH1_CREATE_TABLE = "CREATE TABLE search1 (id INTEGER PRIMARY KEY AUTOINCREMENT, keyword TEXT, open INTEGER)";
const QString ClvDataAccessor::SEARCH1_INSERT_TO_TABLE = "";

const QString ClvDataAccessor::SEARCH2_TABLE = "search2";
const QString ClvDataAccessor::SEARCH2_CHECK_TABLE = "SELECT sql FROM sqlite_master WHERE type='table' AND name='search2'";
const QString ClvDataAccessor::SEARCH2_DROP_TABLE = "DROP TABLE search2";
const QString ClvDataAccessor::SEARCH2_CREATE_TABLE = "CREATE TABLE search2 (search_id INTEGER , command_id INTEGER)";
const QString ClvDataAccessor::SEARCH2_INSERT_TO_TABLE = "";

// table bookmark
const QString ClvDataAccessor::BOOKMARK_TABLE = "bookmark";
const QString ClvDataAccessor::BOOKMARK_CHECK_TABLE = "SELECT sql FROM sqlite_master WHERE type='table' AND name='bookmark'";
const QString ClvDataAccessor::BOOKMARK_DROP_TABLE = "DROP TABLE bookmark";
const QString ClvDataAccessor::BOOKMARK_CREATE_TABLE = "CREATE TABLE bookmark (id INTEGER PRIMARY KEY AUTOINCREMENT, command_id INTEGER, line_number INTEGER)";
const QString ClvDataAccessor::BOOKMARK_INSERT_TO_TABLE = "";

// table memo
const QString ClvDataAccessor::MEMO_TABLE = "memo";
const QString ClvDataAccessor::MEMO_CHECK_TABLE = "SELECT sql FROM sqlite_master WHERE type='table' AND name='memo'";
const QString ClvDataAccessor::MEMO_DROP_TABLE = "DROP TABLE memo";
const QString ClvDataAccessor::MEMO_CREATE_TABLE = "CREATE TABLE memo (id INTEGER PRIMARY KEY AUTOINCREMENT, content TEXT, update_date TEXT)";
const QString ClvDataAccessor::MEMO_INSERT_TO_TABLE = "";

// sql bind tag
const QString ClvDataAccessor::BIND_MAJOR_VERSION = ":major_version";
const QString ClvDataAccessor::BIND_MINOR_VERSION = ":minor_version";
const QString ClvDataAccessor::BIND_PRODUCT = ":product";
const QString ClvDataAccessor::BIND_CREATE_DATE = ":create_date";
const QString ClvDataAccessor::BIND_UPDATE_DATE = ":update_date";
const QString ClvDataAccessor::BIND_NAME = ":name";
const QString ClvDataAccessor::BIND_DECORATED_NAME = ":decorated_name";
const QString ClvDataAccessor::BIND_SCOPE = ":scope";
const QString ClvDataAccessor::BIND_TAG = ":tag";
const QString ClvDataAccessor::BIND_FILE = ":file";
const QString ClvDataAccessor::BIND_DATA = ":data";
const QString ClvDataAccessor::BIND_COMMAND_ID = ":command_id";
const QString ClvDataAccessor::BIND_GROUP_NAME = ":group_name";
const QString ClvDataAccessor::BIND_REGEX = ":regex";
const QString ClvDataAccessor::BIND_COLOR = ":color";
