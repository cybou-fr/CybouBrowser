#pragma once
#include <QAbstractListModel>
#include <QVariantList>

class CapabilityModel final : public QAbstractListModel {
public:
    using QAbstractListModel::QAbstractListModel;
    int rowCount(const QModelIndex &parent = {}) const override { return parent.isValid() ? 0 : rows.size(); }
    QVariant data(const QModelIndex &index, int role) const override {
        return index.isValid() && index.row() < rows.size() && role == Qt::UserRole + 1 ? rows[index.row()] : QVariant();
    }
    QHash<int, QByteArray> roleNames() const override { return {{Qt::UserRole + 1, "capability"}}; }
    bool update(const QVariantList &value) {
        if (rows == value) return false;
        if (rows.size() != value.size()) { beginResetModel(); rows = value; endResetModel(); return true; }
        for (int i = 0; i < rows.size(); ++i) if (rows[i] != value[i]) { rows[i] = value[i]; emit dataChanged(index(i), index(i)); }
        return true;
    }
    QVariantMap row(const QString &key) const {
        for (const auto &row : rows) if (row.toMap().value("key") == key) return row.toMap();
        return {};
    }
private:
    QVariantList rows;
};
