#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

enum class CatalogDataType
{
    INT,
    FLOAT,
    BOOL,
    TIMESTAMP,
    VARCHAR
};

struct Column
{
    int ColumnId;
    std::string name;
    CatalogDataType type;
    bool nullable = true;
    bool unique = false;
    bool primaryKey = false;
    bool indexed = false;
};

struct TableData
{
    int tableId;
    std::string name;
    std::vector<Column> columns;
    
    std::optional<Column> getPrimaryKey() const;
    std::optional<Column> getColumn(const std::string& name) const;
    std::vector<Column> getUniqueColumns() const;
    std::vector<Column> getNonNullableColumns() const;
};

struct IndexData
{
    int indexId;
    std::string name;
    int tableId;
    int columnId;
    std::string tableName;
    std::string columnName;
};

class Catalog
{

public:
    // lock operations
    void acquireSharedLock(const std::vector<std::string>& tableNames, const std::string& txnId);
    void acquireExclusiveLock(const std::vector<std::string>& tableNames, const std::string& txnId);
    void releaseLocks(const std::string& txnId);

    // table queries
    int tableExists(const std::string& name) const;
    std::optional<TableData> getTable(const std::string& name) const;
    std::vector<Column> getAllColumns(const std::string& tableName) const;
    std::optional<Column> getColumn(const std::string& table, const std::string& col) const;

    // index queries
    int indexExists(const std::string& name) const;
    std::optional<IndexData> getIndex(const std::string& name) const;
    std::vector<IndexData> getTableIndexes(const std::string& tableName) const;

    int columnExists(const std::string& tableName, const std::string& columnName) const;

    // mutations
    void addTable(const TableData& table);
    void removeTable(const std::string& name);
    void addIndex(const IndexData& index);
    void removeIndex(const std::string& name);
};