#pragma once

#include <QList>
#include <optional>

template<typename T>
class IRepository {
public:
    virtual ~IRepository() = default;

    virtual bool insert(T &entity) = 0;
    virtual bool update(const T &entity) = 0;
    virtual bool remove(int id) = 0;
    virtual std::optional<T> findById(int id) = 0;
    virtual QList<T> findAll() = 0;
};
