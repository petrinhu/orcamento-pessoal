/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGDOCUMENTFILTER_H
#define SKGDOCUMENTFILTER_H
/** @file
 * The grantlee's filter to get information from document.
 *
 * @author Stephane MANKOWSKI
 */
#include <KTextTemplate/Filter>
#include <qobject.h>

/**
 * The grantlee's filter to get items of a document table
 */
class SKGDocumentTableFilter final : public KTextTemplate::Filter
{
public:
    Q_DISABLE_COPY(SKGDocumentTableFilter)

    /**
     * Default constructor
     */
    SKGDocumentTableFilter() = default;

    /**
     * Do the filtering
     * @param input the input
     * @param argument the argument
     * @param autoescape the autoescape mode
     * @return the filtered value
     */
    QVariant doFilter(const QVariant &input, const QVariant &argument = QVariant(), bool autoescape = false) const override;

    /**
     * To know if the filter is safe
     * @return true or false
     */
    bool isSafe() const override;
};

/**
 * The grantlee's filter to execute a sql query on the document
 */
class SKGDocumentQueryFilter final : public KTextTemplate::Filter
{
public:
    Q_DISABLE_COPY(SKGDocumentQueryFilter)

    /**
     * Default constructor
     */
    SKGDocumentQueryFilter() = default;

    /**
     * Do the filtering
     * @param input the input
     * @param argument the argument
     * @param autoescape the autoescape mode
     * @return the filtered value
     */
    QVariant doFilter(const QVariant &input, const QVariant &argument = QVariant(), bool autoescape = false) const override;

    /**
     * To know if the filter is safe
     * @return true or false
     */
    bool isSafe() const override;
};

/**
 * The grantlee's filter to get display of an attribute
 */
class SKGDocumentDisplayFilter final : public KTextTemplate::Filter
{
public:
    Q_DISABLE_COPY(SKGDocumentDisplayFilter)

    /**
     * Default constructor
     */
    SKGDocumentDisplayFilter() = default;

    /**
     * Do the filtering
     * @param input the input
     * @param argument the argument
     * @param autoescape the autoescape mode
     * @return the filtered value
     */
    QVariant doFilter(const QVariant &input, const QVariant &argument = QVariant(), bool autoescape = false) const override;

    /**
     * To know if the filter is safe
     * @return true or false
     */
    bool isSafe() const override;
};

/**
 * The grantlee's filter to format percent
 */
class SKGPercentFilter final : public KTextTemplate::Filter
{
public:
    Q_DISABLE_COPY(SKGPercentFilter)

    /**
     * Default constructor
     */
    SKGPercentFilter() = default;

    /**
     * Do the filtering
     * @param input the input
     * @param argument the argument
     * @param autoescape the autoescape mode
     * @return the filtered value
     */
    QVariant doFilter(const QVariant &input, const QVariant &argument = QVariant(), bool autoescape = false) const override;

    /**
     * To know if the filter is safe
     * @return true or false
     */
    bool isSafe() const override;
};

/**
 * The grantlee's filter to format file size
 */
class SKGFileSizeFilter final : public KTextTemplate::Filter
{
public:
    Q_DISABLE_COPY(SKGFileSizeFilter)

    /**
     * Default constructor
     */
    SKGFileSizeFilter() = default;

    /**
     * Do the filtering
     * @param input the input
     * @param argument the argument
     * @param autoescape the autoescape mode
     * @return the filtered value
     */
    QVariant doFilter(const QVariant &input, const QVariant &argument = QVariant(), bool autoescape = false) const override;

    /**
     * To know if the filter is safe
     * @return true or false
     */
    bool isSafe() const override;
};

/**
 * The grantlee's filter to dump objects
 */
class SKGDumpFilter final : public KTextTemplate::Filter
{
public:
    Q_DISABLE_COPY(SKGDumpFilter)

    /**
     * Default constructor
     */
    SKGDumpFilter() = default;

    /**
     * Do the filtering
     * @param input the input
     * @param argument the argument
     * @param autoescape the autoescape mode
     * @return the filtered value
     */
    QVariant doFilter(const QVariant &input, const QVariant &argument = QVariant(), bool autoescape = false) const override;

    /**
     * To know if the filter is safe
     * @return true or false
     */
    bool isSafe() const override;
};

/**
 * The grantlee's filter to format money
 */
class SKGMoneyFilter final : public KTextTemplate::Filter
{
public:
    Q_DISABLE_COPY(SKGMoneyFilter)

    /**
     * Default constructor
     */
    SKGMoneyFilter() = default;

    /**
     * Do the filtering
     * @param input the input
     * @param argument the argument
     * @param autoescape the autoescape mode
     * @return the filtered value
     */
    QVariant doFilter(const QVariant &input, const QVariant &argument = QVariant(), bool autoescape = false) const override;

    /**
     * To know if the filter is safe
     * @return true or false
     */
    bool isSafe() const override;
};

/**
 * The grantlee's filter to encode url
 */
class SKGUrlEncodeFilter final : public KTextTemplate::Filter
{
public:
    Q_DISABLE_COPY(SKGUrlEncodeFilter)

    /**
     * Default constructor
     */
    SKGUrlEncodeFilter() = default;

    /**
     * Do the filtering
     * @param input the input
     * @param argument the argument
     * @param autoescape the autoescape mode
     * @return the filtered value
     */
    QVariant doFilter(const QVariant &input, const QVariant &argument = QVariant(), bool autoescape = false) const override;

    /**
     * To know if the filter is safe
     * @return true or false
     */
    bool isSafe() const override;
};

/**
 * The grantlee's filter to replace in strings
 */
class SKGReplaceFilter final : public KTextTemplate::Filter
{
public:
    Q_DISABLE_COPY(SKGReplaceFilter)

    /**
     * Default constructor
     */
    SKGReplaceFilter() = default;

    /**
     * Do the filtering
     * @param input the input
     * @param argument the argument
     * @param autoescape the autoescape mode
     * @return the filtered value
     */
    QVariant doFilter(const QVariant &input, const QVariant &argument = QVariant(), bool autoescape = false) const override;

    /**
     * To know if the filter is safe
     * @return true or false
     */
    bool isSafe() const override;
};
#endif // SKGDOCUMENTFILTER_H
